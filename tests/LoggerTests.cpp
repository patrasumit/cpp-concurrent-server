#include "logging/Logger.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <string>

TEST(Logger, LogsAllLevels)
{
    Logger& logger = Logger::instance();

    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
}

TEST(Logger, HandlesConcurrentLogging)
{
    constexpr int thread_count = 10;

    std::vector<std::thread> threads;

    for (int i = 0; i < thread_count; ++i)
    {
        threads.emplace_back([i]()
        {
            Logger::instance().info(
                "Message from thread " +
                std::to_string(i)
            );
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}
