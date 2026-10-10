// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_reusable_thread.cpp
 * @brief Unit tests for ReusableThread: the function runs with its arguments, isRunning follows
 *        it, a start while it runs is refused or waits for it depending on waitToJoin, the thread
 *        can be started again once done, the precondition value is passed as first argument and
 *        the destructor waits for the running function.
 *
 * The functions are held by a gate the test opens, so no assertion depends on scheduling; waits
 * on the thread state poll with a generous deadline.
 */

#include "act_threading/reusable_thread.hpp"

#include "act_foundation/constants/def_soft.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include "act_logger/printers/abs_external_logger.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace act::threading
{
namespace
{

    /** @brief Generous deadline for anything the tests wait for */
    constexpr std::chrono::seconds WAIT_DEADLINE{5};

    /** @brief Poll the predicate until it holds or the deadline expires */
    bool WaitUntil(const std::function<bool()> &predicate)
    {
        const auto deadline = std::chrono::steady_clock::now() + WAIT_DEADLINE;
        while (!predicate())
        {
            if (std::chrono::steady_clock::now() > deadline)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    /** @brief Gate a thread function waits on until the test opens it */
    class Gate
    {
      public:
        void open()
        {
            m_promise.set_value();
        }

        void wait() const
        {
            m_future.wait();
        }

      private:
        std::promise<void> m_promise;
        std::shared_future<void> m_future{m_promise.get_future().share()};
    };

    class ReusableThreadTest : public ::testing::Test
    {
      protected:
        [[nodiscard]] const act::logger::LoggerHelper &getLogger() const
        {
            return m_logger;
        }

      private:
        // Messages are dropped: the tests only check the thread behavior
        const act::logger::LoggerHelper m_logger{std::shared_ptr<act::logger::AbsExternalLogger>{}};
    };

    TEST_F(ReusableThreadTest, NotRunningBeforeAnyStart)
    {
        const ReusableThread thread;

        EXPECT_FALSE(thread.isRunning());
    }

    TEST_F(ReusableThreadTest, FunctionRunsWithItsArguments)
    {
        ReusableThread thread;
        std::promise<std::string> result;

        const auto status = thread.start(
            getLogger(),
            [](int number, const std::string &text, std::promise<std::string> *output) {
                output->set_value(text + std::to_string(number));
            },
            42,
            std::string("answer "),
            &result);

        ASSERT_EQ(status, ReusableThreadResult::Enum::OK);
        auto future = result.get_future();
        ASSERT_EQ(future.wait_for(WAIT_DEADLINE), std::future_status::ready);
        EXPECT_EQ(future.get(), "answer 42");
    }

    TEST_F(ReusableThreadTest, IsRunningUntilTheFunctionReturns)
    {
        ReusableThread thread;
        Gate gate;

        ASSERT_EQ(thread.start(getLogger(), [&gate]() { gate.wait(); }),
                  ReusableThreadResult::Enum::OK);
        EXPECT_TRUE(thread.isRunning());

        gate.open();
        EXPECT_TRUE(WaitUntil([&thread]() { return !thread.isRunning(); }));
    }

    TEST_F(ReusableThreadTest, StartWithoutWaitIsRefusedWhileRunning)
    {
        ReusableThread thread;
        Gate gate;
        std::atomic<bool> secondRan{false};

        ASSERT_EQ(thread.start(getLogger(), [&gate]() { gate.wait(); }),
                  ReusableThreadResult::Enum::OK);

        EXPECT_EQ(thread.start(getLogger(), [&secondRan]() { secondRan = true; }),
                  ReusableThreadResult::Enum::ALREADY_RUNNING);
        EXPECT_EQ(thread.start(false, getLogger(), [&secondRan]() { secondRan = true; }),
                  ReusableThreadResult::Enum::ALREADY_RUNNING);

        gate.open();
        ASSERT_TRUE(WaitUntil([&thread]() { return !thread.isRunning(); }));
        EXPECT_FALSE(secondRan);
    }

    TEST_F(ReusableThreadTest, StartWithWaitRunsAfterThePreviousFunction)
    {
        ReusableThread thread;
        Gate gate;
        std::atomic<bool> firstDone{false};
        std::promise<bool> secondSawFirstDone;

        ASSERT_EQ(thread.start(getLogger(),
                               [&gate, &firstDone]() {
                                   gate.wait();
                                   firstDone = true;
                               }),
                  ReusableThreadResult::Enum::OK);

        auto secondStart =
            std::async(std::launch::async, [this, &thread, &firstDone, &secondSawFirstDone]() {
                return thread.start(true, getLogger(), [&firstDone, &secondSawFirstDone]() {
                    secondSawFirstDone.set_value(firstDone.load());
                });
            });

        // The second start blocks as long as the first function runs
        EXPECT_EQ(secondStart.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

        gate.open();
        ASSERT_EQ(secondStart.wait_for(WAIT_DEADLINE), std::future_status::ready);
        EXPECT_EQ(secondStart.get(), ReusableThreadResult::Enum::OK);
        auto sawFirstDone = secondSawFirstDone.get_future();
        ASSERT_EQ(sawFirstDone.wait_for(WAIT_DEADLINE), std::future_status::ready);
        EXPECT_TRUE(sawFirstDone.get());
    }

    TEST_F(ReusableThreadTest, CanBeStartedAgainOnceTheFunctionEnded)
    {
        ReusableThread thread;
        std::atomic<int> runs{0};
        const auto increment = [&runs]() { ++runs; };

        ASSERT_EQ(thread.start(getLogger(), increment), ReusableThreadResult::Enum::OK);
        ASSERT_TRUE(WaitUntil([&thread]() { return !thread.isRunning(); }));
        ASSERT_EQ(thread.start(getLogger(), increment), ReusableThreadResult::Enum::OK);
        // Waiting for the previous run is accepted even when it has already ended
        ASSERT_EQ(thread.start(true, getLogger(), increment), ReusableThreadResult::Enum::OK);

        EXPECT_TRUE(WaitUntil([&runs]() { return runs == 3; }));
    }

    TEST_F(ReusableThreadTest, PreconditionValueIsPassedAsFirstArgument)
    {
        ReusableThread thread;
        constexpr int conditionValue = 40;
        const std::function<std::optional<int>()> precondition = []() { return conditionValue; };
        std::promise<int> result;

        const auto status = thread.start<int>(
            getLogger(),
            precondition,
            [](int condition, int offset, std::promise<int> *output) {
                output->set_value(condition + offset);
            },
            2,
            &result);

        ASSERT_EQ(status, ReusableThreadResult::Enum::OK);
        auto future = result.get_future();
        ASSERT_EQ(future.wait_for(WAIT_DEADLINE), std::future_status::ready);
        EXPECT_EQ(future.get(), 42);
    }

    TEST_F(ReusableThreadTest, FailedPreconditionDoesNotStartTheThread)
    {
        ReusableThread thread;
        const std::function<std::optional<int>()> failingPrecondition = []() {
            return std::optional<int>{};
        };
        std::atomic<bool> ran{false};

        EXPECT_EQ(thread.start<int>(getLogger(), failingPrecondition, [&ran](int) { ran = true; }),
                  ReusableThreadResult::Enum::INTERNAL_ERROR);
        EXPECT_EQ(
            thread.start<int>(true, getLogger(), failingPrecondition, [&ran](int) { ran = true; }),
            ReusableThreadResult::Enum::INTERNAL_ERROR);
        EXPECT_FALSE(thread.isRunning());
        EXPECT_FALSE(ran.load());

        // The thread stays usable
        ASSERT_EQ(thread.start(getLogger(), [&ran]() { ran = true; }),
                  ReusableThreadResult::Enum::OK);
        EXPECT_TRUE(WaitUntil([&ran]() { return ran.load(); }));
    }

    TEST_F(ReusableThreadTest, PreconditionStartWithWaitRunsAfterThePreviousFunction)
    {
        ReusableThread thread;
        Gate gate;
        const std::function<std::optional<std::string>()> precondition = []() {
            return std::string("second");
        };
        std::promise<std::string> result;

        ASSERT_EQ(thread.start(getLogger(), [&gate]() { gate.wait(); }),
                  ReusableThreadResult::Enum::OK);
        EXPECT_EQ(thread.start<std::string>(getLogger(), precondition, [](const std::string &) {}),
                  ReusableThreadResult::Enum::ALREADY_RUNNING);

        gate.open();
        EXPECT_EQ(thread.start<std::string>(
                      true,
                      getLogger(),
                      precondition,
                      [](const std::string &text, std::promise<std::string> *output) {
                          output->set_value(text);
                      },
                      &result),
                  ReusableThreadResult::Enum::OK);
        auto future = result.get_future();
        ASSERT_EQ(future.wait_for(WAIT_DEADLINE), std::future_status::ready);
        EXPECT_EQ(future.get(), "second");
    }

    TEST_F(ReusableThreadTest, IsRunningCanBeAskedWhileStarting)
    {
        // isRunning() used to read the working thread that start() deletes and replaces; the race
        // is reported by ThreadSanitizer, or crashes on a use after free
        constexpr int startCount = 2000;
        ReusableThread thread;
        std::atomic<bool> done{false};
        std::thread observer([&thread, &done]() {
            while (!done.load())
            {
                UNUSED(thread.isRunning());
            }
        });

        for (int index = 0; index < startCount; ++index)
        {
            ASSERT_EQ(thread.start(true, getLogger(), []() {}), ReusableThreadResult::Enum::OK);
        }
        done.store(true);
        observer.join();

        EXPECT_TRUE(WaitUntil([&thread]() { return !thread.isRunning(); }));
    }

    TEST_F(ReusableThreadTest, DestructorWaitsForTheRunningFunction)
    {
        std::atomic<bool> finished{false};

        {
            ReusableThread thread;
            ASSERT_EQ(thread.start(getLogger(),
                                   [&finished]() {
                                       std::this_thread::sleep_for(std::chrono::milliseconds(20));
                                       finished = true;
                                   }),
                      ReusableThreadResult::Enum::OK);
        }

        EXPECT_TRUE(finished);
    }

} // namespace
} // namespace act::threading
