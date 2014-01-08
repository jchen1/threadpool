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
  static const unsigned int MAX_IDLE_MS_BEFORE_DESPAWN = 1000;

  pool_core(unsigned int max_threads, bool start_paused)
    : m_threads_running(0),
      m_threads_created(0),
      m_stop_requested(false),
      m_max_threads(max_threads)
  {
    m_threads.reserve(m_max_threads);
    if (start_paused)
    {
      pause();
    }
  }

  ~pool_core()
  {
    wait(false);
  }
  
  template <class T>
  std::future<T> add_task(std::function<T(void)> const & func,
                          unsigned int priority)
  {
    auto promise = std::make_shared<std::promise<T>>();
    std::shared_ptr<task_base> t =
            std::make_shared<task<T>>(func, priority, promise);
    add_task_wrapper(t);
    return promise->get_future();
  }

  void add_task(std::function<void(void)> const & func,
                unsigned int priority)
  {
    std::shared_ptr<task_base> t = std::make_shared<task<void>>(func, priority);
    add_task_wrapper(t);
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

  bool run_task(unsigned int &idle_ms)
  {  
    m_pause_mutex.lock();
    m_pause_mutex.unlock();

    m_task_mutex.lock();
    if (!m_tasks.empty())
    {
      std::shared_ptr<task_base> t = m_tasks.top();
      m_tasks.pop();
      m_task_mutex.unlock();
      ++m_threads_running;
      (*t)();
      --m_threads_running;

      idle_ms = 0;
    }
    else
    {
      m_task_mutex.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      ++idle_ms;
    }

    return (!empty() && idle_ms < MAX_IDLE_MS_BEFORE_DESPAWN &&
      m_threads_created <= m_max_threads);
  }

  bool empty()
  {
    m_task_mutex.lock();
    bool ret = m_tasks.empty();
    m_task_mutex.unlock();
    return ret;
  }

  void clear()
  {
    m_task_mutex.lock();
    while (!m_tasks.empty())
    {
      m_tasks.pop();
    }
    m_task_mutex.unlock();
  }

  void wait(bool clear_tasks)
  {
    if (clear_tasks)
    {
      clear();
    }

    m_stop_requested = true;

    for (auto thread : m_threads)
    {
      thread->join();
    }

    m_stop_requested = false;
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
  void add_task_wrapper(std::shared_ptr<task_base> const & t)
  {
    if (m_stop_requested.load())
    {
      return;
    }
    
    m_task_mutex.lock();
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
    m_tasks.push(t);
    m_task_mutex.unlock();
  }

  std::vector<std::shared_ptr<worker_thread<pool_core>>> m_threads;
  std::priority_queue<std::shared_ptr<task_base>,
      std::vector<std::shared_ptr<task_base>>, task_comparator> m_tasks;

  std::mutex m_task_mutex, m_pause_mutex;

  unsigned int m_max_threads;

  std::atomic_uint m_threads_running;
  std::atomic_uint m_threads_created;
  std::atomic_bool m_stop_requested;

  friend class worker_thread<pool_core>;
};

}

#endif //THREADPOOL_POOLCORE_H
