#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

class worker_thread
{
 public:
  worker_thread(std::function<void(void)>&& run_task)
    : m_thread(&worker_thread::run, this, run_task),
      m_should_destroy(false) {}

  ~worker_thread()
  {
    join();
  }

  void join()
  {
    if (m_thread.joinable())
    {
      m_thread.join();
    }
  }

  bool should_destroy() const
  {
    return m_should_destroy;
  }

 private:
  void run(std::function<void(void)>&& run_task)
  {
    run_task();
    m_should_destroy = true;
  }

  std::thread m_thread;
  bool m_should_destroy;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
