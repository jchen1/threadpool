threadpool
==========

Basic threadpool implementation without a master thread to manage load. Tasks can be submitted with their parameters as extra arguments to threadpool::add_task(), as in std::bind(). Tasks are placed in a queue. Threads are automatically created and destroyed to accomodate the load placed on the threadpool. This library uses C++11 features, so make sure to use a compiler that supports it.

Note that threadpool itself is not thread-safe: don't use a single threadpool object across multiple threads.

To use, just include `pool.hpp` and create an object of type `threadpool::pool`.

Example usage:

```c++
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>

#include "pool.hpp"

std::mutex mtx;

void func(int i)
{
    // lock the mutex for thread-safe cout
    std::unique_lock<std::mutex> lck(mtx);
    std::cout << i << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int future_func(int ret)
{
    std::unique_lock<std::mutex> lck(mtx);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return ret;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "usage: example [max_num] [num_threads]" << std::endl;
        return 0;
    }
    
    int max_num = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    // start the threadpool with num_threads maximum threads and threads
    // that despawn automatically after 1000 idle milliseconds
    threadpool::pool tp(num_threads, 1000);
    
    for (int i = 0; i < max_num; i++)
    {
        tp.add_task(func, i);
    }
    
    tp.add_task([]{
        std::unique_lock<std::mutex> lck(mtx);
        std::cout << 1000 << std::endl;
    });

    auto future = tp.add_task(future_func, 50);
    
    tp.join(false);

    std::cout << future.get() << std::endl; //prints 50
}
```

Sample output:

```
./sample 5 5

0
1
2
3
4
1000
50
```
