#include "threadpool/ThreadPool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>

TEST(ThreadPool, ExecutesTask)
{
    ThreadPool pool(2);

    std::atomic<bool> executed{false};

    ASSERT_TRUE(
        pool.enqueue([&executed]()
        {
            executed = true;
        })
    );

    // Give the worker a chance to execute the task.
    for (int i = 0; i < 100 && !executed; ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_TRUE(executed);
}

TEST(ThreadPool, ExecutesMultipleTasks)
{
    ThreadPool pool(4);

    constexpr int task_count = 20;

    std::atomic<int> completed{0};

    for (int i = 0; i < task_count; ++i)
    {
        ASSERT_TRUE(
            pool.enqueue([&completed]()
            {
                ++completed;
            })
        );
    }

    for (
        int i = 0;
        i < 100 && completed.load() < task_count;
        ++i
    )
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_EQ(completed.load(), task_count);
}

TEST(ThreadPool, ExecutesTasksConcurrently)
{
    ThreadPool pool(2);

    std::atomic<int> active_tasks{0};
    std::atomic<int> max_active_tasks{0};
    std::atomic<int> started_tasks{0};

    auto task = [&]()
    {
        int current = ++active_tasks;

        int previous = max_active_tasks.load();

        while (
            current > previous &&
            !max_active_tasks.compare_exchange_weak(
                previous,
                current))
        {
        }

        ++started_tasks;

        // Keep both workers busy long enough for
        // the other worker to start.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));

        --active_tasks;
    };

    ASSERT_TRUE(pool.enqueue(task));
    ASSERT_TRUE(pool.enqueue(task));

    // Wait until both tasks have actually started.
    for (int i = 0;
         i < 100 && started_tasks.load() < 2;
         ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(5));
    }

    EXPECT_EQ(started_tasks.load(), 2);
    EXPECT_EQ(max_active_tasks.load(), 2);

    // Wait for both tasks to finish.
    for (int i = 0;
         i < 100 && active_tasks.load() != 0;
         ++i)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(5));
    }

    EXPECT_EQ(active_tasks.load(), 0);
}

TEST(ThreadPool, WorkerSurvivesTaskException)
{
    ThreadPool pool(1);

    std::atomic<bool> second_task_executed{false};

    ASSERT_TRUE(
        pool.enqueue([]()
        {
            throw std::runtime_error("test exception");
        })
    );

    ASSERT_TRUE(
        pool.enqueue([&second_task_executed]()
        {
            second_task_executed = true;
        })
    );

    for (
        int i = 0;
        i < 100 && !second_task_executed.load();
        ++i
    )
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    EXPECT_TRUE(second_task_executed);
}

TEST(ThreadPool, DestructorWaitsForRunningTask)
{
    std::atomic<bool> task_started{false};
    std::atomic<bool> task_finished{false};

    {
        ThreadPool pool(1);

        ASSERT_TRUE(
            pool.enqueue([&]()
            {
                task_started = true;

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100)
                );

                task_finished = true;
            })
        );

        for (
            int i = 0;
            i < 100 && !task_started.load();
            ++i
        )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10)
            );
        }

        ASSERT_TRUE(task_started.load());
    }

    // If the destructor returned before the worker finished,
    // this would be false.
    EXPECT_TRUE(task_finished.load());
}

TEST(ThreadPool, DestructorProcessesQueuedTasks)
{
    std::atomic<bool> first_task_started{false};
    std::atomic<bool> release_first_task{false};

    std::atomic<int> completed_tasks{0};

    {
        ThreadPool pool(1);

        ASSERT_TRUE(
            pool.enqueue([&]()
            {
                first_task_started = true;

                while (!release_first_task.load())
                {
                    std::this_thread::yield();
                }

                ++completed_tasks;
            })
        );

        ASSERT_TRUE(
            pool.enqueue([&]()
            {
                ++completed_tasks;
            })
        );

        // Make sure the first task has started.
        for (
            int i = 0;
            i < 100 && !first_task_started.load();
            ++i
        )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10)
            );
        }

        ASSERT_TRUE(first_task_started.load());

        release_first_task = true;
    }

    EXPECT_EQ(completed_tasks.load(), 2);
}

// TEST(ThreadPool, RejectsTaskAfterShutdown)
// {
//     auto pool = std::make_unique<ThreadPool>(2);

//     pool.reset();

//     EXPECT_FALSE(
//         pool->enqueue([] {})
//     );
// }
