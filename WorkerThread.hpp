#pragma once

#include <thread>
#include <chrono>

namespace threadpool {
	
	using namespace std;
	class ThreadPool;

	template <typename Task>
	class WorkerThread
	{
	public:
		WorkerThread(shared_ptr<ThreadPool> pool) :
			p_pool(pool)
		{
			//create thread
			m_thread(run);
		}

		~WorkerThread() {}

		void join()
		{
			m_thread.join();
		}

	private:
		thread m_thread;
		shared_ptr<ThreadPool> p_pool;
		unique_ptr<Task> p_currentTask;
		
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
				
				this_thread::sleep_for(duration<20, milli>);
			}
		}

	};

}
