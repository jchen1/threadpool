threadpool
==========

Basic threadpool implementation. Just include threadpool.hpp.

Example usage:

std::function<void*(void)> fn;

shared_ptr<ThreadPool> tp(new ThreadPool(20));
tp->addTask(fn);
