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

class pool_core
{
 public:
  pool_core(unsigned int max_threads,
            bool start_paused,
            unsigned int despawn_time_ms)
    : m_max_threads(max_threads),
      m_despawn_time_ms(despawn_time_ms),
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

  ~pool_core()
  {
    join(false);
  }
  
  template <typename T>
  std::future<T> add_task(std::function<T(void)>&& func)
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
        new worker_thread(std::bind(&pool_core::run_task, this)));
    }
    auto task = std::make_shared<std::packaged_task<T()>>(func);
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    m_tasks.emplace([task](){ (*task)(); });

    return task->get_future();
  }

  void pause()
  {
    m_pause_mutex.unlock();
    m_pause_mutex.lock();
    m_paused = true;
  }

  void unpause()
  {
    if (!m_pause_mutex.try_lock())
    {
      m_pause_mutex.unlock();
    }
    m_paused = false;
  }

  void run_task()
  {
    ++m_threads_created;  
    while (m_threads_created <= m_max_threads)
    {
      m_pause_mutex.lock();
      m_pause_mutex.unlock();
  
      if (auto t = pop_task(m_despawn_time_ms))
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
        --m_threads_created;
        return;
      }
      destroy_finished_threads();
    }
    --m_threads_created;
  }

  void wait()
  {
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    m_task_empty.wait(task_lock, [&] {
      return m_tasks.empty() && !m_threads_running;
    });
  }

  std::function<void(void)> pop_task(unsigned int max_wait)
  {
    std::function<void(void)> ret;
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    while (m_tasks.empty())
    {
      if (m_join_requested || m_task_ready.wait_for(task_lock,
          std::chrono::milliseconds(max_wait)) == std::cv_status::timeout)
      {
        return ret;
      }
    }
    ret = m_tasks.front();
    m_tasks.pop();

    return ret;
  }

  bool empty()
  {
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    return m_tasks.empty();
  }

  void clear()
  {
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    std::queue<std::function<void(void)>>().swap(m_tasks);
  }

  void join(bool clear_tasks)
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

  unsigned int get_max_threads() const
  {
    return m_max_threads;
  }

 private:
  inline void destroy_finished_threads()
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

  std::vector<std::unique_ptr<worker_thread>> m_threads;
  std::queue<std::function<void(void)>> m_tasks;

  std::mutex m_task_mutex, m_pause_mutex, m_thread_mutex;
  std::condition_variable m_task_ready, m_task_empty;

  unsigned int m_max_threads, m_despawn_time_ms;

  std::atomic_uint m_threads_created, m_threads_running;
  std::atomic_bool m_join_requested, m_paused;
};

}

#endif //THREADPOOL_POOLCORE_H
