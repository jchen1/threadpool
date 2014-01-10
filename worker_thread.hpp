#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

/*
 * worker_thread is templated to avoid cyclical includes - otherwise,
 * worker_thread.hpp would have to include pool_core.hpp and vice versa.
 */
template <class pool_core>
class worker_thread
{
 public:
  worker_thread(std::shared_ptr<pool_core> pool)
    : m_pool(pool),
      m_thread(std::thread(std::bind(&worker_thread::run, this))) {}

  ~worker_thread()
  {
    join();
  }

  void join()
  {
    m_thread.join();
  }

 private:
  void run()
  {
    ++m_pool->m_threads_created;
    if (m_pool)
    {
      m_pool->run_task();
    }
    --m_pool->m_threads_created;
  }

  std::shared_ptr<pool_core> m_pool;
  std::thread m_thread;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
