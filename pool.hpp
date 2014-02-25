#ifndef THREADPOOL_POOLCORE_H
#define THREADPOOL_POOLCORE_H

#include <algorithm>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <vector>

#include "worker_thread.hpp"

namespace threadpool {

/*
 * Thread pool that does not need a master thread to manage load. A task is of
 * the form std::function<T(void)>, and the add_task() function will return a
 * std::future<T> which will contain the return value of the function. Tasks are
 * placed in a queue. Threads are created only when there are no idle threads
 * available and the total thread count does not exceed the maximum thread
 * count. Threads are despawned if they are idle for more than despawn_file_ms,
 * the third argument in the constructor of the threadpool.
 */
class pool
{
 public:
  /* 
   * Creates a new thread pool, with max_threads threads allowed. Starts paused
   * if start_paused = true. Default values are max_threads = 
   * std::thread::hardware_concurrency(), which should return the number of
   * physical cores the CPU has, and start_paused = false.
   */
  pool(unsigned int max_threads = std::thread::hardware_concurrency(),
       bool start_paused = false,
       unsigned int despawn_time = 1000)
    : m_max_threads(max_threads),
      m_despawn_time(despawn_time),
      m_threads_created(0),
      m_threads_running(0),
      m_join_requested(false)
  {
    m_threads.reserve(m_max_threads);
    if (start_paused)
    {
      pause();
    }
  }

  /*
   * When the pool is destructed, it will first stop all worker threads.
   */
  ~pool()
  {
    join(false);
  }

  /*
   * Adds a new task to the task queue. The task must be a function object,
   * and the remaining passed arguments must be parameters for task, which will
   * be bound using std::bind().
   */
  template <typename T, typename... Args,
            typename R = typename std::result_of<T(Args...)>::type>
  std::future<R> add_task(T&& task, Args&&... args)
  {
    /*
     * If all created threads are executing tasks and we have not spawned the
     * maximum number of allowed threads, create a new thread.
     */
    if ((m_threads_created == m_threads_running || m_paused) &&
        m_threads_created != m_max_threads)
    {
      std::lock_guard<std::mutex> thread_lock(m_thread_mutex);
      m_threads.emplace_back(
        new worker_thread(std::bind(&pool::run_task, this)));
    }
    auto p_task = std::make_shared<std::packaged_task<R()>>(
      std::bind(std::forward<T>(task), std::forward<Args>(args)...));
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    m_tasks.emplace([p_task](){ (*p_task)(); });

    return p_task->get_future();
  }

  /*
   * Pauses the thread pool - all currently executing tasks will finish, but any
   * remaining tasks in the task queue will not be executed until unpause() is
   * called. Tasks may still be added to the queue when the pool is paused.
   * Any spawned threads will not despawn.
   */
  void pause()
  {
    m_paused = true;
  }

  /*
   * Unpauses the thread pool.
   */
  void unpause()
  {
    m_paused = false;
    m_unpaused_cv.notify_all();
  }

  /*
   * Waits for the task queue to empty and for all worker threads to complete,
   * without destroying worker threads.
   */
  void wait()
  {
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    m_task_empty.wait(task_lock, [&] {
      return m_tasks.empty() && !m_threads_running;
    });
  }

  /*
   * Returns true if the task queue is empty. Note that worker threads may be
   * running, even if empty() returns true.
   */
  bool empty()
  {
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    return m_tasks.empty();
  }

  /* 
   * Clears the task queue. Does not stop any running tasks.
   */
  void clear()
  {
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    std::queue<std::function<void(void)>>().swap(m_tasks);
  }

  /*
   * Waits for all threads to finish executing. join(true) will clear any
   * remaining tasks in the task queue, thus exiting once any running workers
   * finish. join(false), on the other hand, will wait until the task queue
   * is empty and the running workers finish. Spawned threads will exit.
   */
  void join(bool clear_tasks = false)
  {
    if (clear_tasks)
    {
      clear();
    }

    m_join_requested = true;

    std::lock_guard<std::mutex> thread_lock(m_thread_mutex);

    for (auto&& thread : m_threads)
    {
      thread->join();
    }

    m_join_requested = false;

    m_threads.clear();
  }

  /*
   * Returns how many worker threads are currently executing a task.
   */
  unsigned int get_threads_running() const
  {
    return m_threads_running.load();
  }

  /*
   * Returns how many worker threads have been created.
   */
  unsigned int get_threads_created() const
  {
    return m_threads_created.load();
  }

  /*
   * Sets the maximum number of worker threads the thread pool can spawn.
   */
  void set_max_threads(unsigned int max_threads)
  {
    m_max_threads = max_threads;
  }

  /*
   * Returns the maximum number of worker threads the pool can spawn.
   */
  unsigned int get_max_threads() const
  {
    return m_max_threads;
  }

 private:
  void destroy_finished_threads()
  {
    std::unique_lock<std::mutex> thread_lock(m_thread_mutex, std::defer_lock);
    if (thread_lock.try_lock())
    {
      auto to_erase = std::remove_if(begin(m_threads), end(m_threads),
        [] (const decltype(m_threads)::value_type& thread) {
          return thread->should_destroy();
        });
      m_threads.erase(to_erase, end(m_threads));
    }
  }

  std::function<void(void)> pop_task()
  {
    std::function<void(void)> ret;
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    while (m_tasks.empty())
    {
      if (m_join_requested || m_task_ready.wait_for(task_lock,
          std::chrono::milliseconds(m_despawn_time)) == std::cv_status::timeout)
      {
        return ret;
      }
    }
    ret = m_tasks.front();
    m_tasks.pop();

    return ret;
  }

  void run_task()
  {
    ++m_threads_created;  
    while (m_threads_created <= m_max_threads)
    {
      std::unique_lock<std::mutex> lck(m_pause_mutex);
      while (m_paused)
      {
        m_unpaused_cv.wait(lck);
      }
      lck.unlock();
  
      if (auto t = pop_task())
      {
        ++m_threads_running;
        t();
        --m_threads_running;
        if (empty() && !m_threads_running)
        {
          m_task_empty.notify_all();
        }
      }
      else if (m_join_requested.load())
      {
        break;
      }
    }
    --m_threads_created;
    destroy_finished_threads();
  }

  std::vector<std::unique_ptr<worker_thread>> m_threads;
  std::queue<std::function<void(void)>> m_tasks;

  std::mutex m_task_mutex, m_thread_mutex, m_pause_mutex;
  std::condition_variable m_task_ready, m_task_empty, m_unpaused_cv;

  unsigned int m_max_threads, m_despawn_time;

  std::atomic_uint m_threads_created, m_threads_running;
  std::atomic_bool m_join_requested, m_paused;
};

}

#endif //THREADPOOL_POOLCORE_H
