#ifndef THREADPOOL_TASK_H
#define THREADPOOL_TASK_H

#include <functional>
#include <future>
#include <memory>

namespace threadpool {

class task_base
{
 public:
  task_base(unsigned int priority) : m_priority(priority) {}

  virtual void operator() (void) {}

  unsigned int get_priority() const
  {
    return m_priority;
  }

 protected:
  unsigned int m_priority;
};

template <class T>
class task : public task_base
{
 public:
  task(std::function<T(void)> const & function,
       unsigned int priority, std::shared_ptr<std::promise<T>> p)
    : task_base(priority), m_function(function), promise(p) {}

  void operator() (void)
  {
    if (m_function)
    {
      promise->set_value(m_function());
    }
  }

 private:
  std::function<T(void)> m_function;
  std::shared_ptr<std::promise<T>> promise;
};

template <>
class task<void> : public task_base
{
 public:
  task(std::function<void(void)> const & function,
       unsigned int priority, std::shared_ptr<std::promise<void>> p)
    : task_base(priority), m_function(function), promise(p) {}

  void operator() (void)
  {
    if (m_function)
    {
      m_function();
      promise->set_value();
    }
  }

 private:
  std::function<void(void)> m_function;
  std::shared_ptr<std::promise<void>> promise;
};

class task_comparator
{
 public:
  bool operator() (const std::shared_ptr<task_base>& lhs,
                   const std::shared_ptr<task_base>& rhs)
  {
    return (lhs->get_priority() < rhs->get_priority());
  }
};

}

#endif //THREADPOOL_TASK_H
