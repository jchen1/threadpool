#ifndef THREADPOOL_POOL_H
#define THREADPOOL_POOL_H

#include "pool_core.hpp"

namespace threadpool {

class threadpool
{
public:
	threadpool(int max_threads) :
		m_core(new pool_core(max_threads))
	{

	}

	void add_task(task_ptr task)
	{
		m_core->add_task(std::move(task));
	}

	void add_task(task_func func)
	{
		m_core->add_task(func);
	}

	bool empty()
	{
		return m_core->empty();
	}

	void clear()
	{
		m_core->clear();
	}

	void wait(bool clear_tasks)
	{
		m_core->wait(clear_tasks);
	}

	void wait(long max_wait_ms, bool clear_tasks)
	{
		m_core->wait(max_wait_ms, clear_tasks);
	}

private:
	std::shared_ptr<pool_core> m_core;
};

}

#endif