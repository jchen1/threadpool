#pragma once

#include <vector>
#include <queue>
#include "WorkerThread.hpp"

namespace threadpool {

	using namespace std;
	using namespace std::placeholders;

	template <typename Task>
	class ThreadPool
	{
	public:
		typedef WorkerThread<Task, ThreadPool> Thread;

		ThreadPool(int maxThreads) :
			m_maxThreads(maxThreads)
		{

		}

		void addTask(Task t)
		{
			p_taskQueue.push(t);
		}

		unique_ptr<Task> getTask()
		{
			unique_ptr<Task> task;
			//lock
			if (!p_taskQueue.empty())
			{
				task = move(p_taskQueue.front());

				if (p_taskQueue.empty())
				{
					//set empty event
				}
			}
			else
			{
				//set empty event
			}
		}

		void joinAll()
		{
			//while not empty event
			for_each(begin(p_threads), end(p_threads), bind(Thread::joinAll, _1));
		}

	private:
		int m_maxThreads;

		vector<unique_ptr<Thread>> p_threads;
		queue<unique_ptr<Task>> p_taskQueue; 


	};
}