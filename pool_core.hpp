#ifndef THREADPOOL_POOLCORE_H
#define THREADPOOL_POOLCORE_H

#include <algorithm>
#include <mutex>
#include <queue>
#include <vector>

#include "task.hpp"
#include "worker_thread.hpp"

namespace threadpool {

class pool_core : public std::enable_shared_from_this<pool_core>
{
 public:

  pool_core(unsigned int max_threads,
            bool start_paused,
            unsigned int despawn_time_ms)
    : m_max_threads(max_threads),
      m_despawn_time_ms(despawn_time_ms),
      m_threads_created(0),
      m_threads_running(0),
      m_join_requested(false),
      m_stop_requested(false)      
  {
    m_threads.reserve(m_max_threads);
    if (start_paused)
    {
      pause();
    }
  }

  ~pool_core()
  {
    join(false);
  }
  
  template <class T>
  std::future<T> add_task(std::function<T(void)> const & func,
                          unsigned int priority)
  {
    auto promise = std::make_shared<std::promise<T>>();
    std::shared_ptr<task_base> task_ptr =
            std::make_shared<task<T>>(func, priority, promise);
    add_task_wrapper(task_ptr);
    return promise->get_future();
  }

  std::future<void> add_task(std::function<void(void)> const & func,
                             unsigned int priority)
  {
    auto promise = std::make_shared<std::promise<void>>();
    std::shared_ptr<task_base> task_ptr = 
            std::make_shared<task<void>>(func, priority, promise);
    add_task_wrapper(task_ptr);
    return promise->get_future();
  }

  void pause()
  {
    /*
     * First unlocks the pause mutex before locking it, ensuring that if the
     * current thread already owns the lock (i.e. pause() has been called prior
     * to the current call without a corresponding unpause()), no deadlock will
     * occur.
     */
    m_pause_mutex.unlock();
    m_pause_mutex.lock();
  }

  void unpause()
  {
    m_pause_mutex.unlock();
  }

  void run_task()
  {  
    unsigned int idle_ms(0);
    std::unique_lock<std::mutex> task_lock(m_task_mutex, std::defer_lock);

    while (idle_ms < m_despawn_time_ms)
    {
      m_pause_mutex.lock();
      m_pause_mutex.unlock();

      task_lock.lock();
      if (!m_tasks.empty())
      {
        std::shared_ptr<task_base> t = m_tasks.top();
        m_tasks.pop();
        task_lock.unlock();
        ++m_threads_running;
        (*t)();
        --m_threads_running;

        idle_ms = 0;
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ++idle_ms;
        task_lock.unlock();

        if (m_join_requested.load())
        {
          return;
        }
      }
    }
  }

  bool empty()
  {
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    return m_tasks.empty();
  }

  void clear()
  {
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    while (!m_tasks.empty())
    {
      m_tasks.pop();
    }
  }

  void join(bool clear_tasks)
  {
    if (clear_tasks)
    {
      clear();
    }

    m_join_requested = true;

    for (auto thread : m_threads)
    {
      thread->join();
    }

    m_join_requested = false;

  }

  unsigned int get_threads_running() const
  {
    return m_threads_running.load();
  }

  unsigned int get_threads_created() const
  {
    return m_threads_created.load();
  }

  void set_max_threads(unsigned int max_threads)
  {
    m_max_threads = max_threads;
  }

  int get_max_threads() const
  {
    return m_max_threads;
  }

 private: 
  void add_task_wrapper(std::shared_ptr<task_base> const & task_ptr)
  {
    if (m_stop_requested.load())
    {
      return;
    }

    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    
    /*
     * If all created threads are executing tasks and we have not spawned the
     * maximum number of allowed threads, create a new thread.
     */
    if (m_threads_created == m_threads_running &&
        m_threads_created != m_max_threads)
    {
      m_threads.emplace_back(
        worker_thread<pool_core>::create_and_attach(shared_from_this()));
      ++m_threads_created;
    }
    m_tasks.push(task_ptr);
  }

  std::vector<std::shared_ptr<worker_thread<pool_core>>> m_threads;
  std::priority_queue<std::shared_ptr<task_base>,
      std::vector<std::shared_ptr<task_base>>, task_comparator> m_tasks;

  std::mutex m_task_mutex, m_pause_mutex;

  unsigned int m_max_threads, m_despawn_time_ms;

  std::atomic_uint m_threads_created, m_threads_running;
  std::atomic_bool m_join_requested, m_stop_requested;

  friend class worker_thread<pool_core>;
};

}

#endif //THREADPOOL_POOLCORE_H
