#ifndef THREADPOOL_POOL_H
#define THREADPOOL_POOL_H

#include "pool_core.hpp"

namespace threadpool {

class threadpool
{
public:
	threadpool(int max_threads) : m_core(new pool_core(max_threads)) {}

	inline void add_task(task_wrapper const & task)
	{
		m_core->add_task(task);
	}

	inline void add_task(task_func const & func)
	{
		m_core->add_task(func);
	}

	inline bool empty()
	{
		return m_core->empty();
	}

	inline void clear()
	{
		m_core->clear();
	}

	inline void wait(bool clear_tasks)
	{
		m_core->wait(clear_tasks);
	}

private:
	std::shared_ptr<pool_core> m_core;
};

}

#endif
