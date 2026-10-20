// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_restartable_timer.cpp
 * @brief Unit tests for act::time::RestartableTimer.
 *
 * Covers the one-shot and periodic modes, starting at construction or later, restarting (which
 * postpones the expiry), the start identifiers given to the callback, stopping, the running state
 * following each call at once, changing the duration and the periodicity, restarting from the
 * callback, and destroying a timer which is still armed.
 *
 * The expiries happen in the timer thread, so the tests wait for them on conditions with
 * generous deadlines and only assert lower bounds on the elapsed times.
 */

#include "act_foundation/constants/def_soft.hpp"
#include "act_logger/services/logger_manager.hpp"
#include "act_time/restartable_timer.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace act::time
{
namespace
{

    using Clock = std::chrono::steady_clock;
    using std::chrono::milliseconds;

    /** @brief Deadline after which a condition expected to become true is reported as failed */
    constexpr milliseconds WAIT_DEADLINE{5000};

    /** @brief Time left to a timer to show it does not fire when it must not */
    constexpr milliseconds QUIET_PERIOD{150};

    /** @brief Duration of a timer expected to fire within a test */
    constexpr unsigned int FAST_MS = 10;

    /** @brief Duration a timer never reaches within a test */
    constexpr unsigned int LONG_MS = 60000;

    /** @brief Duration of a timer cancelled before it fires */
    constexpr unsigned int CANCELLED_MS = 100;

    /** @brief Time waited after a cancellation, longer than CANCELLED_MS */
    constexpr milliseconds SILENCE_PERIOD{300};

    /** @brief Number of timers of the stress tests, enough to hit the narrow races they look for */
    constexpr int STRESS_TIMER_COUNT = 20000;

    /** @brief Number of timers armed at once by the stress test of stop() */
    constexpr int STRESS_ARMED_TIMER_COUNT = 500;

    /** @brief Deadline of a stress test, far beyond the time it takes */
    constexpr std::chrono::seconds STRESS_DEADLINE{30};

    /**
     * @brief Run @p work in a detached thread and wait for it until the stress deadline
     * @return True if @p work returned in time; a blocked @p work is abandoned, and stays alive
     *         with what it captured, so the test fails instead of hanging
     */
    bool RunsBeforeTheStressDeadline(const std::function<void()> &work)
    {
        auto done = std::make_shared<std::promise<void>>();
        auto finished = done->get_future();
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks): the detached thread owns it
        std::thread([work, done]() {
            work();
            done->set_value();
        }).detach();
        return finished.wait_for(STRESS_DEADLINE) == std::future_status::ready;
    }

    /**
     * @brief Poll @p condition until it is true or the deadline expires
     * @return The last value of @p condition
     */
    bool WaitUntil(const std::function<bool()> &condition)
    {
        const auto deadline = Clock::now() + WAIT_DEADLINE;
        while (!condition())
        {
            if (Clock::now() > deadline)
            {
                return false;
            }
            std::this_thread::sleep_for(milliseconds(1));
        }
        return true;
    }

    class RestartableTimerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());
        }

        /** @brief Callback counting its calls and remembering when the last one happened */
        std::function<void(RestartableTimer::StartId)> countingCallback()
        {
            return [this](RestartableTimer::StartId /*start*/) {
                m_lastFireTime.store(Clock::now());
                ++m_fireCount;
            };
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        /** @brief Count one more fire, for callbacks which do more than counting */
        int recordFire()
        {
            return ++m_fireCount;
        }

        [[nodiscard]] int getFireCount() const
        {
            return m_fireCount.load();
        }

        [[nodiscard]] Clock::time_point getLastFireTime() const
        {
            return m_lastFireTime.load();
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::atomic<int> m_fireCount{0};
        std::atomic<Clock::time_point> m_lastFireTime{};
    };

    TEST_F(RestartableTimerTest, ExposesTheConstructionSettings)
    {
        RestartableTimer oneShot(getLoggerManager(), LONG_MS, countingCallback());
        EXPECT_EQ(oneShot.getDurationMs(), LONG_MS);
        EXPECT_FALSE(oneShot.isPeriodic());
        EXPECT_FALSE(oneShot.isRunning());

        RestartableTimer periodic(getLoggerManager(), FAST_MS, countingCallback(), true);
        EXPECT_EQ(periodic.getDurationMs(), FAST_MS);
        EXPECT_TRUE(periodic.isPeriodic());
    }

    TEST_F(RestartableTimerTest, SettersUpdateTheSettings)
    {
        RestartableTimer timer(getLoggerManager(), LONG_MS, countingCallback());

        timer.setDurationMs(FAST_MS);
        timer.setPeriodic(true);

        EXPECT_EQ(timer.getDurationMs(), FAST_MS);
        EXPECT_TRUE(timer.isPeriodic());
    }

    TEST_F(RestartableTimerTest, DoesNotFireUntilStarted)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback());

        std::this_thread::sleep_for(QUIET_PERIOD);

        EXPECT_EQ(getFireCount(), 0);
        EXPECT_FALSE(timer.isRunning());
    }

    TEST_F(RestartableTimerTest, StartImmediatelyFiresOnce)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback(), false, true);

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));

        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(RestartableTimerTest, OneShotFiresOnceAfterItsDuration)
    {
        constexpr unsigned int durationMs = 50;
        RestartableTimer timer(getLoggerManager(), durationMs, countingCallback());

        const auto startTime = Clock::now();
        timer.startOrRestart();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        EXPECT_GE(getLastFireTime() - startTime, milliseconds(durationMs));

        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));
        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(RestartableTimerTest, IsRunningFollowsStartAndStopAtOnce)
    {
        RestartableTimer timer(getLoggerManager(), LONG_MS, countingCallback());

        timer.startOrRestart();
        EXPECT_TRUE(timer.isRunning());

        timer.stop();
        EXPECT_FALSE(timer.isRunning());
        EXPECT_EQ(getFireCount(), 0);
    }

    TEST_F(RestartableTimerTest, StopPreventsTheExpiry)
    {
        RestartableTimer timer(getLoggerManager(), CANCELLED_MS, countingCallback());

        timer.startOrRestart();
        timer.stop();
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));

        std::this_thread::sleep_for(SILENCE_PERIOD);
        EXPECT_EQ(getFireCount(), 0);
    }

    TEST_F(RestartableTimerTest, StopOnANeverStartedTimerDoesNothing)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback());

        timer.stop();

        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_FALSE(timer.isRunning());
        EXPECT_EQ(getFireCount(), 0);
    }

    TEST_F(RestartableTimerTest, EachStartGetsANewIdentifier)
    {
        RestartableTimer timer(getLoggerManager(), LONG_MS, countingCallback());

        const RestartableTimer::StartId first = timer.startOrRestart();
        const RestartableTimer::StartId second = timer.startOrRestart(LONG_MS);
        timer.stop();
        const RestartableTimer::StartId third = timer.startOrRestart();

        EXPECT_NE(first, second);
        EXPECT_NE(second, third);
        EXPECT_NE(first, third);
    }

    TEST_F(RestartableTimerTest, CallbackReceivesTheIdentifierOfTheLastStart)
    {
        std::atomic<RestartableTimer::StartId> expiredStart{0};
        std::atomic<int> expiryCount{0};
        RestartableTimer timer(getLoggerManager(),
                               LONG_MS,
                               [&expiredStart, &expiryCount](RestartableTimer::StartId start) {
                                   expiredStart.store(start);
                                   ++expiryCount;
                               });

        UNUSED(timer.startOrRestart());
        const RestartableTimer::StartId lastStart = timer.startOrRestart(FAST_MS);

        ASSERT_TRUE(WaitUntil([&expiryCount]() { return expiryCount.load() == 1; }));
        EXPECT_EQ(expiredStart.load(), lastStart);

        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(expiryCount.load(), 1);
    }

    TEST_F(RestartableTimerTest, PeriodicTimerGivesItsStartAtEveryPeriod)
    {
        constexpr int wantedPeriods = 3;
        std::atomic<int> periodsOfTheStart{0};
        std::atomic<RestartableTimer::StartId> start{0};
        RestartableTimer timer(
            getLoggerManager(),
            FAST_MS,
            [&start, &periodsOfTheStart](RestartableTimer::StartId expiredStart) {
                if (expiredStart == start.load())
                {
                    ++periodsOfTheStart;
                }
            },
            true);

        // A period expiring before the start is recorded is not counted, the next ones are
        start.store(timer.startOrRestart());

        EXPECT_TRUE(WaitUntil(
            [&periodsOfTheStart]() { return periodsOfTheStart.load() >= wantedPeriods; }));
        timer.stop();
    }

    TEST_F(RestartableTimerTest, RestartPostponesTheExpiry)
    {
        constexpr unsigned int durationMs = 200;
        RestartableTimer timer(getLoggerManager(), durationMs, countingCallback());

        timer.startOrRestart();
        std::this_thread::sleep_for(milliseconds(durationMs / 2));
        const auto restartTime = Clock::now();
        timer.startOrRestart();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 1; }));
        EXPECT_GE(getLastFireTime() - restartTime, milliseconds(durationMs));

        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(RestartableTimerTest, StartWithADurationReplacesTheDuration)
    {
        RestartableTimer timer(getLoggerManager(), LONG_MS, countingCallback());

        timer.startOrRestart(FAST_MS);

        EXPECT_EQ(timer.getDurationMs(), FAST_MS);
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
    }

    TEST_F(RestartableTimerTest, NewDurationAppliesOnTheNextStart)
    {
        RestartableTimer timer(getLoggerManager(), LONG_MS, countingCallback());

        timer.setDurationMs(FAST_MS);
        timer.startOrRestart();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
    }

    TEST_F(RestartableTimerTest, CanBeStartedAgainAfterExpiring)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback());

        timer.startOrRestart();
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));

        timer.startOrRestart();
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 2; }));
    }

    TEST_F(RestartableTimerTest, PeriodicFiresUntilStopped)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback(), true);

        timer.startOrRestart();
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 3; }));
        EXPECT_TRUE(timer.isRunning());

        timer.stop();
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));

        const int countAfterStop = getFireCount();
        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), countAfterStop);
    }

    TEST_F(RestartableTimerTest, ClearingPeriodicStopsAfterTheCurrentPeriod)
    {
        RestartableTimer timer(getLoggerManager(), FAST_MS, countingCallback(), true);

        timer.startOrRestart();
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 1; }));

        timer.setPeriodic(false);
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));

        const int countAfterLastPeriod = getFireCount();
        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), countAfterLastPeriod);
    }

    TEST_F(RestartableTimerTest, CallbackCanRestartTheTimer)
    {
        constexpr int wantedFires = 3;
        RestartableTimer *timerPtr = nullptr;
        RestartableTimer timer(getLoggerManager(),
                               FAST_MS,
                               [this, &timerPtr](RestartableTimer::StartId) {
                                   if (recordFire() < wantedFires)
                                   {
                                       timerPtr->startOrRestart();
                                   }
                               });
        timerPtr = &timer;

        timer.startOrRestart();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == wantedFires; }));
        ASSERT_TRUE(WaitUntil([&timer]() { return !timer.isRunning(); }));
        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), wantedFires);
    }

    TEST_F(RestartableTimerTest, DestroyingAStoppedTimerAlwaysReturns)
    {
        // A request made while the timer thread handles the previous one used to be erased, so
        // the termination asked by the destructor could be lost and the destructor blocked forever
        auto loggerManager = std::make_shared<act::logger::LoggerManager>();
        ASSERT_TRUE(loggerManager->init());

        EXPECT_TRUE(RunsBeforeTheStressDeadline([loggerManager]() {
            for (int index = 0; index < STRESS_TIMER_COUNT; ++index)
            {
                RestartableTimer timer(*loggerManager, LONG_MS, [](RestartableTimer::StartId) {});
                timer.startOrRestart();
                timer.stop();
            }
        }));
    }

    TEST_F(RestartableTimerTest, StopRightAfterStartIsNeverLost)
    {
        // Same race as above, with a stop() erased by the handling of the start it follows
        auto loggerManager = std::make_shared<act::logger::LoggerManager>();
        ASSERT_TRUE(loggerManager->init());
        auto fireCount = std::make_shared<std::atomic<int>>(0);

        EXPECT_TRUE(RunsBeforeTheStressDeadline([loggerManager, fireCount]() {
            std::vector<std::unique_ptr<RestartableTimer>> timers;
            for (int index = 0; index < STRESS_ARMED_TIMER_COUNT; ++index)
            {
                timers.push_back(std::make_unique<RestartableTimer>(
                    *loggerManager,
                    FAST_MS,
                    [fireCount](RestartableTimer::StartId) { ++(*fireCount); }));
                timers.back()->startOrRestart();
                timers.back()->stop();
            }
            std::this_thread::sleep_for(QUIET_PERIOD);
        }));

        EXPECT_EQ(fireCount->load(), 0);
    }

    TEST_F(RestartableTimerTest, DestroyingAnArmedTimerCancelsTheExpiry)
    {
        {
            RestartableTimer timer(getLoggerManager(), CANCELLED_MS, countingCallback(), true);
            timer.startOrRestart();
        }

        std::this_thread::sleep_for(SILENCE_PERIOD);
        EXPECT_EQ(getFireCount(), 0);
    }

} // namespace
} // namespace act::time
