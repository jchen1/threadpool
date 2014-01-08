#ifndef THREADPOOL_TASK_H
#define THREADPOOL_TASK_H

#include <functional>
#include <future>
#include <memory>

namespace threadpool {

class task_base
{
 public:
  void operator() (void) const;
  unsigned int get_priority() const;
};

template <class T>
class task : public task_base
{
 public:
  task(std::function<T(void)> const & function,
               unsigned int priority)
    : m_function(function), m_priority(priority) {}

  void operator() (void) const
  {
    if (m_function)
    {
      promise.set_value(m_function());
    }
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

class task_comparator
{
 public:
  bool operator() (const std::unique_ptr<task_base>& lhs, const std::unique_ptr<task_base>& rhs)
  {
    return (lhs->get_priority() < rhs->get_priority());
  }
};

}

#endif //THREADPOOL_TASK_H
