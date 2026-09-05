#include "threadpool/ThreadPool.h"
#include "logging/Logger.h"

#include <stdexcept>

ThreadPool::ThreadPool(size_t num_threads){

    if (num_threads == 0)
    {
        throw std::invalid_argument(
            "ThreadPool must have at least one worker"
        );
    }

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
            Logger::instance().error(
                std::string("Task failed: ") + e.what()
            );
        }
        catch (...)
        {
            Logger::instance().error(
                "Task failed with unknown exception"
            );
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

