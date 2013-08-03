pool_core
==========

Basic pool_core implementation. Just include pool.hpp.

Example usage:

std::function<void(void)> fn;

threadpool p(10);
p.add_task(fn);
