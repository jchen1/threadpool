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
    cout << i << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cout << "usage: example [max_num] [num_threads]" << endl;
    }
    
    int max_num = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    threadpool::threadpool tp(num_threads);
    
    for (int i = 0; i < max_num; i++)
    {
        tp.add_task(std::bind(func, i), 1);
    }

    tp.add_task(std::bind(func, 50), 5);
    tp.add_task(std::bind(func, 100), 100); 	//executed before previous line
    
    tp.wait();
}
```
