#ifndef THREADPOOL_WORKERTHREAD_H
#define THREADPOOL_WORKERTHREAD_H

#include <thread>
#include <chrono>
#include "ThreadPool.hpp"

template <typename Task>
class WorkerThread
{
public:
	WorkerThread<Task>(ThreadPool* pool) :
		p_pool(pool)
	{
		//create thread
		m_thread(std::bind(&WorkerThread::run, this));
	}

	~WorkerThread() {}

	void join()
	{
		m_thread.join();
	}

private:
	std::thread m_thread;
	ThreadPool* p_pool;
	std::unique_ptr<Task> p_currentTask;
	
	void run()
	{
		++p_pool->m_threadsPending;
		while (p_pool->isRunning())
		{
			if (p_pool->tasksLeft())
			{
				p_currentTask = p_pool->getNextTask();
				
				if (p_currentTask.get() != nullptr)
				{
					--p_pool->m_threadsPending;
					++p_pool->m_threadsRunning;
					p_pool->m_tasksCompleted.push((*p_currentTask)());
					--p_pool->m_threadsRunning;
					++p_pool->m_threadsPending;
				}
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}

};

#endif //THREADPOOL_WORKERTHREAD_H
