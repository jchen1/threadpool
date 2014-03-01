#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>

namespace threadpool {

class worker_thread
{
 public:
  worker_thread(std::function<void(void)> run_task)
    : running(true),
      thread(std::bind(&worker_thread::run, this, run_task)) {}

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

  bool running;

 private:
  void run(std::function<void(void)> run_task)
  {
    run_task();
    running = false;
  }

  std::thread thread;
};

}

#endif //THREADPOOL_WORKERTHREAD_H
