threadpool
==========

Basic threadpool implementation. Just include pool.hpp.

Example usage:

std::function<void(void)> fn;

threadpool tp(10);
tp.add_task(fn);
