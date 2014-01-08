#ifndef THREADPOOL_TASK_H
#define THREADPOOL_TASK_H

#include <functional>
#include <future>
#include <memory>

namespace threadpool {

class task_base
{
 public:
  virtual void operator() (void)
  {
    printf("fu");
    return;
  }

  virtual unsigned int get_priority() const
  {
    return 0;
  }
};

template <class T>
class task : public task_base
{
 public:
  task(std::function<T(void)> const & function,
               unsigned int priority)
    : m_function(function), m_priority(priority) {}

  void operator() (void)
  {
    printf("hi\n");
    if (m_function)
    {
      printf("running\n");
      promise.set_value(m_function());
      printf("ran\n");
    }
  }
  
  unsigned int get_priority() const
  {
    return m_priority;
  }

  std::future<T> get_future()
  {
    return promise.get_future();
  }

 private:
  std::function<T(void)> m_function;
  std::promise<T> promise;

  const unsigned int m_priority;
};

class task_comparator
{
 public:
  bool operator() (const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)
  {
    return (lhs->get_priority() < rhs->get_priority());
  }
};

}

#endif //THREADPOOL_TASK_H
