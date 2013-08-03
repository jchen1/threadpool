#ifndef THREADPOOL_TASKWRAPPER_H
#define THREADPOOL_TASKWRAPPER_H

#include <functional>
#include <memory>

typedef std::function<void(void)> task_func;

class task_wrapper
{
public:

	task_wrapper(task_func function) :
		m_function(function)
	{

	}

	void operator() (void) const
	{
		if (m_function)
		{
			m_function();
		}
	}

	static std::unique_ptr<task_wrapper>  make_task_ptr(task_func function)
	{
		return std::unique_ptr<task_wrapper>(new task_wrapper(function));
	}

private:

	task_func m_function;
};

typedef std::unique_ptr<task_wrapper> task_ptr;

#endif //THREADPOOL_TASKWRAPPER_H