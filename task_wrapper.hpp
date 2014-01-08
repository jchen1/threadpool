#ifndef THREADPOOL_TASKWRAPPER_H
#define THREADPOOL_TASKWRAPPER_H

#include <functional>
#include <future>
#include <memory>

namespace threadpool {

class task_wrapper_base
{
 public:
  void operator() (void) const;
  bool operator() (const task_wrapper_base& lhs, const task_wrapper_base& rhs);
  unsigned int get_priority() const;
};

template <class T>
class task_wrapper : public task_wrapper_base
{
 public:
  task_wrapper(std::function<T(void)> const & function,
               unsigned int priority)
    : m_function(function), m_priority(priority) {}

  void operator() (void) const
  {
    if (m_function)
    {
      promise.set_value(m_function());
    }
  }
  
  bool operator() (const task_wrapper_base& lhs, const task_wrapper_base& rhs)
  {
    return (lhs.get_priority() < rhs.get_priority());
  }
  
  unsigned int get_priority() const
  {
    return m_priority;
  }

  std::future<T> get_future()
  {
    return std::move(promise.get_future());
  }

 private:
  std::function<T(void)> m_function;
  std::promise<T> promise;

  const unsigned int m_priority;
};

}

#endif //THREADPOOL_TASKWRAPPER_H
