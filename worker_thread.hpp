#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

class worker_thread
{
 public:
  worker_thread(std::function<void(void)> run_task)
    : should_destroy(false),
      thread(run_task) {}

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

  bool should_destroy;

 private:
  void run(std::function<void(void)> run_task)
  {
    run_task();
    should_destroy = true;
  }

  std::thread thread;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
