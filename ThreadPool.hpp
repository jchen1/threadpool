#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <mutex>

#include "WorkerThread.hpp"

namespace threadpool {

	using namespace std;
	using std::placeholders::_1;

	class ThreadPool : public enable_shared_from_this<ThreadPool>
	{
	public:
	
		typedef std::function<void*(void)> Task;
		typedef WorkerThread<Task> Thread;

		ThreadPool(int maxThreads) :
			m_maxThreads(maxThreads),
			m_threadsCreated(0),
			m_bRunning(true),
			m_bTasksLeft(false),
			m_threadsRunning(0),
			m_threadsPending(0)
		{
			m_threads.reserve(m_maxThreads);
		}


		~ThreadPool()
		{
			joinAll();
		}
		
		void addTask(Task t)
		{
			if (m_threadsPending == 0 && m_threadsCreated < m_maxThreads)
			{
				m_threads.emplace_back(new Thread(shared_from_this()));
				++m_threadsCreated;
			}
			m_taskQueuMutex.lock();
			p_taskQueue.push(unique_ptr<Task>(new Task(t)));
			m_taskQueueMutex.unlock();
			m_bTasksLeft = true;
		}
		
		void* getCompletedTask()
		{
			if (p_tasksCompleted.empty())
			{
				return nullptr;
			}
			
			void* task = p_tasksCompleted.front();
			p_tasksCompleted.pop();
			
			return task;
		}
		
		inline bool tasksLeft() { return m_bTasksLeft; }
		inline bool isRunning() { return m_bRunning; }

		void joinAll()
		{
			while (tasksLeft())
			{
				//sleep(100);
			}
			for_each(begin(p_threads), end(p_threads), bind(Thread::join, _1));
		}

	private:
		int m_maxThreads;

		vector<unique_ptr<Thread>> p_threads;
		queue<unique_ptr<Task>> p_taskQueue;
		queue<void*> p_tasksCompleted;
		mutex m_taskQueueMutex;
		bool m_bTasksLeft, m_bRunning;
		int m_maxThreads, m_ThreadsCreated;
		
		volatile int m_threadsRunning, m_threadsPending;
		
		unique_ptr<Task> getNextTask()
		{
			unique_ptr<Task> task;
			m_taskQueueMutex.lock();
			if (tasksLeft())
			{
				task = move(p_taskQueue.front());
				p_taskQueue.pop();
				if (p_taskQueue.empty())
				{
					m_bTasksLeft = false;
				}
			}
			else
			{
				m_bTasksLeft = false;
			}
			m_taskQueueMutex.unlock();
			return task;
		}


	};
}
