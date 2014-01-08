#ifndef THREADPOOL_POOL_H
#define THREADPOOL_POOL_H

#include "pool_core.hpp"

namespace threadpool {

/*
 * Thread pool that does not need a master thread to manage load. Tasks must
 * have no return value or arguments, of the form void task(void). Tasks are
 * added to a max-priority queue. Threads are created only when there are no
 * idle threads available and the total thread count does not exceed the
 * maximum thread count. Threads are despawned if they are idle for more than
 * a specified time (MAX_IDLE_MS_BEFORE_DESPAWN in pool_core.hpp, defaulted
 * to 1000 ms).
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
             bool start_paused = false)
    : m_core(new pool_core(max_threads, start_paused)) {}

  /*
   * Adds a new task to the task queue, with an optional priority. The task
   * must be a zero-argument function object with no return value. For a
   * function that takes arguments, use std::bind() on the function before
   * adding it to the task queue.
   */
  template <class T>
  inline std::future<T> add_task(std::function<T(void)> const & func,
                       unsigned int priority = 0)
  {
    return m_core->add_task(func, priority);
  }

  /*
   * Pauses the thread pool - all currently executing tasks will finish, but any
   * remaining tasks in the task queue will not be executed until unpause() is
   * called. Tasks may still be added to the queue when the pool is paused.
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
   * Waits for all threads to finish executing. wait(true) will clear any
   * remaining tasks in the task queue, thus exiting once any running workers
   * finish. wait(false), on the other hand, will wait until the task queue
   * is empty and the running workers finish.
   */
  inline void wait(bool clear_tasks)
  {
    m_core->wait(clear_tasks);
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
  std::shared_ptr<pool_core> m_core;
};

}

#endif //THREADPOOL_POOL_H
