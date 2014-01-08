threadpool
==========

Basic threadpool implementation. Just include `pool.hpp`.

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
        tp.add_task(std::bind(func, i), 1);
    }
    
    tp.pause();

    tp.add_task(std::bind(func, 50), 5);
    tp.add_task(std::bind(func, 100), 100); 	//executed before previous line
    
    tp.unpause();
    
    tp.wait(false);
}
```
