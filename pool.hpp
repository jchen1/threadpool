#ifndef THREADPOOL_POOL_H
#define THREADPOOL_POOL_H

#include <condition_variable>
#include <future>
#include <list>
#include <queue>

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
   * physical cores the CPU has, start_paused = false, and idle_time = 1000.
   */
  pool(unsigned int max_threads = std::thread::hardware_concurrency(),
       bool start_paused = false,
       unsigned int idle_time = 1000)
    : max_threads(max_threads),
      idle_time(idle_time),
      threads_created(0),
      threads_running(0),
      join_requested(false),
      paused(start_paused)
  {
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
    if (threads_created == threads_running && threads_created != max_threads)
    {
      std::lock_guard<std::mutex> thread_lock(thread_mutex);
      threads.emplace_back(std::bind(&pool::run_task, this));
    }
    auto p_task = std::make_shared<std::packaged_task<R()>>(
      std::bind(std::forward<T>(task), std::forward<Args>(args)...));
    std::lock_guard<std::mutex> task_lock(task_mutex);
    tasks.emplace([p_task](){ (*p_task)(); });
    task_ready.notify_one();

    return p_task->get_future();
  }

  /* 
   * Clears the task queue. Does not stop any running tasks.
   */
  void clear()
  {
    std::lock_guard<std::mutex> task_lock(task_mutex);
    std::queue<std::function<void(void)>>().swap(tasks);
  }

  /*
   * Returns true if the task queue is empty. Note that worker threads may be
   * running, even if empty() returns true.
   */
  bool empty()
  {
    std::lock_guard<std::mutex> task_lock(task_mutex);
    return tasks.empty();
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

    join_requested = true;

    task_ready.notify_all();
    std::lock_guard<std::mutex> thread_lock(thread_mutex);

    for (auto&& thread : threads)
    {
      thread.join();
    }

    join_requested = false;

    threads.clear();
  }

  /*
   * Pauses the thread pool - all currently executing tasks will finish, but any
   * remaining tasks in the task queue will not be executed until unpause() is
   * called. Tasks may still be added to the queue when the pool is paused.
   * Any spawned threads will not despawn.
   */
  void pause()
  {
    paused = true;
  }

  void unpause()
  {
    paused = false;
    unpaused_cv.notify_all();
  }

  /*
   * Waits for the task queue to empty and for all worker threads to complete,
   * without destroying worker threads.
   */
  void wait()
  {
    std::unique_lock<std::mutex> task_lock(task_mutex);
    task_empty.wait(task_lock, [&] {
      return tasks.empty() && !threads_running;
    });
  }

  unsigned int get_threads_running() const
  {
    return threads_running.load();
  }

  unsigned int get_threads_created() const
  {
    return threads_created.load();
  }

  unsigned int get_max_threads() const
  {
    return max_threads;
  }

  void set_max_threads(unsigned int max_threads)
  {
    this->max_threads = max_threads;
  }

  unsigned int get_idle_time() const
  {
    return idle_time.count();
  }

  void set_idle_time(unsigned int idle_time)
  {
    this->idle_time = std::chrono::milliseconds(idle_time);
  }

 private:
  std::function<void(void)> pop_task()
  {
    std::function<void(void)> ret;
    std::unique_lock<std::mutex> task_lock(task_mutex);
    while (tasks.empty() && !join_requested)
    {
      if (task_ready.wait_for(task_lock, idle_time) == std::cv_status::timeout)
      {
        return ret;
      }
    }
    if (join_requested)
    {
      return ret;
    }
    ret = tasks.front();
    tasks.pop();

    if (!threads_running && tasks.empty())
    {
      task_empty.notify_all();
    }

    return ret;
  }

  void run_task()
  {
    ++threads_created;  
    while (threads_created <= max_threads)
    {
      std::unique_lock<std::mutex> lck(pause_mutex);
      while (paused)
      {
        unpaused_cv.wait(lck);
      }
      if (auto t = pop_task())
      {
        ++threads_running;
        t();
        --threads_running;
      }
      else
      {
        break;
      }
    }
    --threads_created;
    
    std::unique_lock<std::mutex> thread_lock(thread_mutex, std::defer_lock);
    if (thread_lock.try_lock())
    {
      threads.remove_if([] (const worker_thread& thread) {
        return thread.should_destroy;
      });
    }
  }

  std::list<worker_thread> threads;
  std::queue<std::function<void(void)>> tasks;

  std::mutex task_mutex, thread_mutex, pause_mutex;
  std::condition_variable_any task_ready, task_empty, unpaused_cv;

  unsigned int max_threads;
  std::chrono::milliseconds idle_time;

  std::atomic_uint threads_created, threads_running;
  std::atomic_bool join_requested, paused;
};

}

#endif //THREADPOOL_POOL_H
