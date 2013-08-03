#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>
#include <memory>
#include <functional>

#include "task_wrapper.hpp"

namespace threadpool {

template <class pool_core>
class worker_thread
{
public:

    typedef std::shared_ptr<worker_thread<pool_core>> worker_thread_ptr;
    worker_thread(std::shared_ptr<pool_core> pool) :
        m_pool(pool)
    {

    }

    ~worker_thread()
    {
        join();
    }

    void join()
    {
        m_thread->join();
    }

    static worker_thread_ptr create_and_attach(std::shared_ptr<pool_core> pool)
    {
        worker_thread_ptr worker(new worker_thread<pool_core>(pool));
        if (worker)
        {
            worker->m_thread.reset(new std::thread(
                std::bind(&worker_thread::run, worker)));
        }

        return worker;
    }


private:

    void run()
    {
        ++m_pool->m_threads_running;
        while (m_pool && m_pool->run_task());
        --m_pool->m_threads_running;
        --m_pool->m_threads_created;
    }

    std::shared_ptr<pool_core> m_pool;
    std::shared_ptr<std::thread> m_thread;

};

}

#endif //THREADPOOL_WORKERTHREAD_H