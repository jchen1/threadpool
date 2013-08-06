#ifndef THREADPOOL_TASKWRAPPER_H
#define THREADPOOL_TASKWRAPPER_H

#include <functional>
#include <memory>

namespace threadpool {

typedef std::function<void(void)> task_func;

class task_wrapper
{
public:

	task_wrapper(task_func const & function, int priority = 1) :
		m_function(function), m_priority(priority) {}

	void operator() (void) const
	{
		if (m_function)
		{
			m_function();
		}
	}
	
	bool operator() (const task_wrapper& lhs, const task_wrapper& rhs)
	{
		return (lhs.get_priority() < rhs.get_priority());
	}
	
	int get_priority() const
	{
		return m_priority;
	}

private:

	task_func m_function;
	int m_priority;
};

typedef std::unique_ptr<task_wrapper> task_ptr;

}

#endif //THREADPOOL_TASKWRAPPER_H
