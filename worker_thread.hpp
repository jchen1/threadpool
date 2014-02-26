#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

class worker_thread
{
 public:
  worker_thread(std::function<void(void)>&& run_task)
    : thread(&worker_thread::run, this, run_task),
      thread_completed(false) {}

  ~worker_thread()
  {
    join();
  }

  void join()
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }

  bool should_destroy() const
  {
    return thread_completed;
  }

 private:
  void run(std::function<void(void)>&& run_task)
  {
    run_task();
    thread_completed = true;
  }

  std::thread thread;
  bool thread_completed;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
