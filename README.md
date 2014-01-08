threadpool
==========

Basic threadpool implementation without a master thread to manage load. Tasks submitted to the threadpool must be of type std::function<void(void)>. Tasks are added to a max-priority queue, with a default priority of 0. Threads are automatically created and destroyed to accomodate the load placed on the threadpool.

To use, just include `pool.hpp` and create an object of type `threadpool::pool`.

Example usage:

```c++
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "pool.hpp"

void func(int i)
{
    std::cout << i << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "usage: example [max_num] [num_threads]" << std::endl;
        return -1;
    }
    
    int max_num = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    threadpool::pool tp(num_threads, false);
    
    for (int i = 0; i < max_num; i++)
    {
        tp.add_task(std::bind(func, i));    //default priority is 0
    }
    
    tp.pause();

    tp.add_task(std::bind(func, 50), 5);
    tp.add_task(std::bind(func, 100), 100); 	//executed before previous line
    
    tp.unpause();
    
    tp.wait();
}
```
