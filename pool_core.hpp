#ifndef THREADPOOL_POOLCORE_H
#define THREADPOOL_POOLCORE_H

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "task_wrapper.hpp"
#include "worker_thread.hpp"

namespace threadpool {

class pool_core : public std::enable_shared_from_this<pool_core>
{

public:

    pool_core(int max_threads) :
        m_max_threads(max_threads),
        m_threads_running(0),
        m_threads_created(0),
        m_stop_requested(false)
    {
        m_threads.reserve(m_max_threads);
    }

    ~pool_core()
    {
        wait(-1);
    }

    void add_task(task_ptr task)
    {
        if (m_stop_requested)   //if stop is requested, don't add more ;_;
        {
            return;
        }

        m_task_mutex.lock();
        if (m_threads_created == m_threads_running &&
            m_threads_created != m_max_threads)
        {
            m_threads.emplace_back(
                worker_thread<pool_core>::create_and_attach(get_ptr()));
            ++m_threads_created;
        }
        m_tasks.push(std::move(task));
        m_task_mutex.unlock();
    }

    void add_task(task_func func)
    {
        add_task(task_wrapper::make_task_ptr(func));
    }

    bool run_task()
    {
        task_func task;

        m_task_mutex.lock();
        if (!m_tasks.empty())
        {
            task = *(std::move(m_tasks.front()));
            m_tasks.pop();
            m_task_mutex.unlock();
            task();
        }
        else
        {
            m_task_mutex.unlock();
        }

        return (!empty());
    }

    bool empty()
    {
        bool ret;
        m_task_mutex.lock();
        ret = m_tasks.empty();
        m_task_mutex.unlock();
        return ret;
    }

    void clear()
    {
        m_task_mutex.lock();
        while (!m_tasks.empty())
        {
            m_tasks.pop();
        }
        m_task_mutex.unlock();
    }

    void wait(bool clear_tasks)
    {
        if (clear_tasks)
        {
            clear();
        }

        m_stop_requested = true;

        std::for_each(begin(m_threads), end(m_threads),
            [] (std::shared_ptr<worker_thread<pool_core>> wtp)
            {
                wtp->join();
            });
    }

    std::shared_ptr<pool_core> get_ptr()
    {
        return shared_from_this();
    }


private:

    std::vector<std::shared_ptr<worker_thread<pool_core>>> m_threads;
    std::queue<task_ptr> m_tasks;

    std::mutex m_task_mutex;

    int m_max_threads;

    volatile int m_threads_running;
    volatile int m_threads_created;
    volatile bool m_stop_requested;

    friend class worker_thread<pool_core>;

};

}

#endif //THREADPOOL_POOLCORE_H