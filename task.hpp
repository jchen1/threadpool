#ifndef THREADPOOL_TASK_H
#define THREADPOOL_TASK_H

#include <functional>
#include <memory>

namespace threadpool {

class task
{
 public:
  task(std::function<void(void)> const & function,
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

class task_comparator
{
 public:
  bool operator() (const task& lhs, const task& rhs) const
  {
    return (lhs.get_priority() < rhs.get_priority());
  }
};

}

#endif //THREADPOOL_TASK_H
