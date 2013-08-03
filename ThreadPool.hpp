#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H

#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <algorithm>
#include <thread>

#include "WorkerThread.hpp"

class ThreadPool
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
		p_threads.reserve(m_maxThreads);
	}


	~ThreadPool()
	{
		joinAll();
	}
	
	void addTask(Task t)
	{
		if (m_threadsPending == 0 && m_threadsCreated < m_maxThreads)
		{
			p_threads.emplace_back(new Thread(this));
			++m_threadsCreated;
		}
		m_taskQueueMutex.lock();
		p_taskQueue.push(std::unique_ptr<Task>(new Task(t)));
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
			std::this_thread::sleep_for(chrono::milliseconds(100));
		}
		for_each(std::begin(p_threads), std::end(p_threads), std::bind(Thread::join, std::placeholders::_1));
	}

private:
	
	std::vector<std::unique_ptr<Thread>> p_threads;
	std::queue<std::unique_ptr<Task>> p_taskQueue;
	std::queue<void*> p_tasksCompleted;
	std::mutex m_taskQueueMutex;
	bool m_bTasksLeft, m_bRunning;
	int m_maxThreads, m_threadsCreated;
	
	volatile int m_threadsRunning, m_threadsPending;
	
	std::unique_ptr<Task> getNextTask()
	{
		std::unique_ptr<Task> task;
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

#endif //THREADPOOL_THREADPOOL_H