#include "threadpool/ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads){
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool(){
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        running = false;
    }

    cv.notify_all();

    for (std::thread& worker : workers)
    {
        worker.join();
    }
}

void ThreadPool::worker(){
    
    while(true){
        std::function<void()> task;{
            std::unique_lock<std::mutex> lock(queue_mutex);

            cv.wait(lock,[this]{
                return !tasks.empty() || !running;
            });

            if(!running && tasks.empty()){
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        try
        {
            task();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Task failed: " << e.what() << '\n';
        }
        catch (...)
        {
            std::cerr << "Task failed with unknown exception\n";
        }
    }
}

bool ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        if (!running)
            return false;

        tasks.push(std::move(task));
    }

    cv.notify_one();
    return true;
}

