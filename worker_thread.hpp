#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

/*
 * worker_thread is templated to avoid cyclical includes - otherwise,
 * worker_thread.hpp would have to include pool_core.hpp and vice versa.
 */
template <typename pool_core>
class worker_thread
{
 public:
  worker_thread(std::shared_ptr<pool_core> pool)
    : m_pool(pool),
      m_thread(std::thread(std::bind(&worker_thread::run, this))),
      m_should_destroy(false) {}

  ~worker_thread()
  {
    if (m_thread.joinable())
    {
      m_thread.join();
    }
  }

  void join()
  {
    m_thread.join();
  }

  bool should_destroy() const
  {
    return m_should_destroy;
  }

 private:
  void run()
  {
    ++m_pool->m_threads_created;
    m_pool->run_task();
    --m_pool->m_threads_created;
    m_should_destroy = true;
  }

  std::shared_ptr<pool_core> m_pool;
  std::thread m_thread;

  bool m_should_destroy;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
