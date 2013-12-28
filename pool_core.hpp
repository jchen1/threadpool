#ifndef THREADPOOL_POOLCORE_H
#define THREADPOOL_POOLCORE_H

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "task_wrapper.hpp"
#include "worker_thread.hpp"

namespace threadpool {

class pool_core : public std::enable_shared_from_this<pool_core>
{

public:

  pool_core(unsigned int max_threads = std::thread::hardware_concurrency(),
            bool start_paused = false) :
    m_threads_running(0),
    m_threads_created(0),
    m_stop_requested(false)
  {
    m_max_threads = max_threads;
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
  
  void add_task(std::function<void(void)> const & func,
                unsigned int priority = 1)
  {
    task_wrapper task(func, priority);
    add_task(task);
  }

  void pause()
  {
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
      auto task = m_tasks.top();
      m_tasks.pop();
      m_task_mutex.unlock();
      ++m_threads_running;
      task();
      --m_threads_running;

      idle_ms = 0;
    }
    else
    {
      m_task_mutex.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      ++idle_ms;
    }

    return (!empty() && idle_ms < 1000 &&
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

  std::shared_ptr<pool_core> get_ptr()
  {
    return shared_from_this();
  }


private:

  std::vector<std::shared_ptr<worker_thread<pool_core>>> m_threads;
  std::priority_queue<task_wrapper> m_tasks;

  std::mutex m_task_mutex, m_pause_mutex;

  unsigned int m_max_threads;

  std::atomic_uint m_threads_running;
  std::atomic_uint m_threads_created;
  std::atomic_bool m_stop_requested;
  
  friend class worker_thread<pool_core>;

  void add_task(task_wrapper const & task)
  {
    if (m_stop_requested.load())
    {
      return;
    }
    
    m_task_mutex.lock();
    if (m_threads_created == m_threads_running &&
      m_threads_created != m_max_threads)
    {
      m_threads.emplace_back(
        worker_thread<pool_core>::create_and_attach(get_ptr()));
      ++m_threads_created;        
    }
    m_tasks.push(task);
    m_task_mutex.unlock();
  }

};

}

#endif //THREADPOOL_POOLCORE_H
