#ifndef THREADPOOL_TASKWRAPPER_H
#define THREADPOOL_TASKWRAPPER_H

#include <functional>
#include <memory>

namespace threadpool {

class task_wrapper
{
 public:
  task_wrapper(std::function<void(void)> const & function,
               unsigned int priority)
    : m_function(function), m_priority(priority) {}

  void operator() (void) const
  {
    if (m_function)
    {
      m_function();
    }
  }
  
  unsigned int get_priority() const
  {
    return m_priority;
  }

 private:
  std::function<void(void)> m_function;
  unsigned int m_priority;
};

class task_wrapper_comparator
{
 public:
  bool operator() (const task_wrapper& lhs, const task_wrapper& rhs) const
  {
    return (lhs.get_priority() < rhs.get_priority());
  }
};

}

#endif //THREADPOOL_TASKWRAPPER_H
