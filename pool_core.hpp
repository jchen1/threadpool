#ifndef THREADPOOL_POOLCORE_H
#define THREADPOOL_POOLCORE_H

#include <algorithm>
#include <condition_variable>
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
    auto promise = std::make_shared<std::promise<T>>();
    std::lock_guard<std::mutex> task_lock(m_task_mutex);
    m_tasks.emplace(new task<T>(func, promise));
    m_task_ready.notify_one();

    return promise->get_future();
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

      auto t = pop_task(m_despawn_time_ms);
      if (t)
      {
        ++m_threads_running;
        (*t)();
        --m_threads_running;
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

  std::unique_ptr<task_base> pop_task(unsigned int max_wait)
  {
    std::unique_ptr<task_base> ret;
    std::unique_lock<std::mutex> task_lock(m_task_mutex);
    while (m_tasks.empty())
    {
      if (m_join_requested || m_task_ready.wait_for(task_lock,
          std::chrono::milliseconds(max_wait)) == std::cv_status::timeout)
      {
        return ret;
      }
    }
    /*
     * This is OK because we immediately pop the task afterwards and because
     * we have already locked m_task_mutex, so the priority queue ordering
     * invariant doesn't get messed up.
     */
    ret = std::move(const_cast<std::unique_ptr<task_base>&>(m_tasks.front()));
    m_tasks.pop();

    if (m_tasks.empty())
    {
      m_task_empty.notify_all();
    }

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
  std::queue<std::unique_ptr<task_base>> m_tasks;

  std::mutex m_task_mutex, m_pause_mutex, m_thread_mutex;
  std::condition_variable m_task_ready, m_task_empty;

  unsigned int m_max_threads, m_despawn_time_ms;

  std::atomic_uint m_threads_created, m_threads_running;
  std::atomic_bool m_join_requested, m_paused;
};

}

#endif //THREADPOOL_POOLCORE_H
