#ifndef THREADPOOL_TASK_H
#define THREADPOOL_TASK_H

#include <functional>
#include <future>
#include <memory>

namespace threadpool {

class task_base
{
 public:
  task_base() {}
  virtual void operator() (void) {}
};

template <typename T>
class task : public task_base
{
 public:
  task(std::function<T(void)> const & function,
       std::shared_ptr<std::promise<T>> p)
    : task_base(), m_function(function), promise(p) {}

  void operator() (void)
  {
    if (m_function)
    {
      promise->set_value(m_function());
      promise.reset();
      m_function = nullptr;
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
       std::shared_ptr<std::promise<void>> p)
    : task_base(), m_function(function), promise(p) {}

  void operator() (void)
  {
    if (m_function)
    {
      m_function();
      promise->set_value();
      promise.reset();
      m_function = nullptr;
    }
  }

 private:
  std::function<void(void)> m_function;
  std::shared_ptr<std::promise<void>> promise;
};

}

#endif //THREADPOOL_TASK_H
