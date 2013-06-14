#pragma once

#include <thread>

namespace threadpool {
	
	using namespace std;

	template <typename Task, typename ThreadPool>
	class WorkerThread
	{
	public:
		WorkerThread()
		{

		}

		~WorkerThread() {}

		void join()
		{
			m_thread.join();
		}

	private:
		thread m_thread;

	};

}