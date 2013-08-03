#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>
#include <memory>
#include <functional>

#include "task_wrapper.hpp"

template <class threadpool>
class worker_thread
{
public:

    typedef std::shared_ptr<worker_thread<threadpool>> worker_thread_ptr;
    worker_thread(std::shared_ptr<threadpool> pool) :
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

    static worker_thread_ptr create_and_attach(std::shared_ptr<threadpool> pool)
    {
        worker_thread_ptr worker(new worker_thread<threadpool>(pool));
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

    std::shared_ptr<threadpool> m_pool;
    std::shared_ptr<std::thread> m_thread;

};

#endif //THREADPOOL_WORKERTHREAD_H
