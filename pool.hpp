#ifndef THREADPOOL_POOL_H
#define THREADPOOL_POOL_H

#include "pool_core.hpp"

namespace threadpool {

/*
 * Thread pool that does not need a master thread to manage load. A task is of
 * the form std::function<T(void)>, and the add_task() function will return a
 * std::future<T> which will contain the return value of the function. Tasks are
 * placed in a queue. Threads are created only when there are no idle threads
 * available and the total thread count does not exceed the maximum thread
 * count. Threads are despawned if they are idle for more than despawn_file_ms,
 * the third argument in the constructor of the threadpool.
 */
class pool
{
 public:
  /* 
   * Creates a new thread pool, with max_threads threads allowed. Starts paused
   * if start_paused = true. Default values are max_threads = 
   * std::thread::hardware_concurrency(), which should return the number of
   * physical cores the CPU has, and start_paused = false.
   */
  pool(unsigned int max_threads = std::thread::hardware_concurrency(),
       bool start_paused = false,
       unsigned int despawn_time = 1000)
    : m_core(new pool_core(max_threads, start_paused, despawn_time)) {}

  /*
   * Adds a new task to the task queue. The task must be a function object,
   * and the remaining passed arguments must be parameters for task, which will
   * be bound using std::bind().
   */
  template <typename T, typename... Args, 
            typename R = typename std::result_of<T(Args...)>::type>
  inline std::future<R> add_task(T&& task, Args&&... args)
  {
    return m_core->add_task(
      (std::function<R(void)>)std::bind(std::forward<T>(task),
                                        std::forward<Args>(args)...));
  }

  /*
   * Pauses the thread pool - all currently executing tasks will finish, but any
   * remaining tasks in the task queue will not be executed until unpause() is
   * called. Tasks may still be added to the queue when the pool is paused.
   * Any spawned threads will not despawn.
   */
  inline void pause()
  {
    m_core->pause();
  }

  /*
   * Unpauses the thread pool.
   */
  inline void unpause()
  {
    m_core->unpause();
  }

  /*
   * Returns true if the task queue is empty. Note that worker threads may be
   * running, even if empty() returns true.
   */
  inline bool empty()
  {
    return m_core->empty();
  }

  /* 
   * Clears the task queue. Does not stop any running tasks.
   */
  inline void clear()
  {
    m_core->clear();
  }

  /*
   * Waits for all threads to finish executing. join(true) will clear any
   * remaining tasks in the task queue, thus exiting once any running workers
   * finish. join(false), on the other hand, will wait until the task queue
   * is empty and the running workers finish. Spawned threads will exit.
   */
  inline void join(bool clear_tasks = false)
  {
    m_core->join(clear_tasks);
  }

  /*
   * Waits for the task queue to empty and for all worker threads to complete,
   * without destroying worker threads.
   */
  inline void wait()
  {
    m_core->wait();
  }

  /*
   * Returns how many worker threads are currently executing a task.
   */
  inline unsigned int get_threads_running() const
  {
    return m_core->get_threads_running();
  }

  /*
   * Returns how many worker threads have been created.
   */
  inline unsigned int get_threads_created() const
  {
    return m_core->get_threads_created();
  }

  /*
   * Sets the maximum number of worker threads the thread pool can spawn.
   */
  inline void set_max_threads(unsigned int max_threads)
  {
    m_core->set_max_threads(max_threads);
  }

  /*
   * Returns the maximum number of worker threads the pool can spawn.
   */
  inline unsigned int get_max_threads() const
  {
    return m_core->get_max_threads();
  }

 private:
  std::unique_ptr<pool_core> m_core;
};

}

#endif //THREADPOOL_POOL_H
