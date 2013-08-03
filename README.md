pool_core
==========

Basic pool_core implementation. Just include pool_core.hpp.

Example usage:

std::function<void*(void)> fn;

shared_ptr<pool_core> tp(new pool_core(20));
tp->addTask(fn);
