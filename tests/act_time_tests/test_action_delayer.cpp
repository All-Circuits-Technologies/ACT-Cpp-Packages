// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_action_delayer.cpp
 * @brief Unit tests for act::time::ActionDelayer.
 *
 * Covers starting at construction or later, stopping, the difference between startOrRestart()
 * (which restarts the delay and the maximum delay) and startOrDelay() (which only restarts the
 * delay), the maximum delay capping an action postponed again and again, firing only once per
 * start, and destroying a delayer with a pending action.
 *
 * The action runs on a timer thread, so the tests wait on conditions with generous deadlines and
 * only assert lower bounds on the elapsed times.
 */

#include "act_logger/services/logger_manager.hpp"
#include "act_time/action_delayer.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

#include <gtest/gtest.h>

namespace act::time
{
namespace
{

    using Clock = std::chrono::steady_clock;
    using std::chrono::milliseconds;

    /** @brief Deadline after which a condition expected to become true is reported as failed */
    constexpr milliseconds WAIT_DEADLINE{5000};

    /** @brief Time left to a delayer to show it does not fire when it must not */
    constexpr milliseconds QUIET_PERIOD{150};

    /** @brief Interval between two calls when an action is postponed repeatedly */
    constexpr milliseconds POSTPONE_INTERVAL{20};

    /** @brief Delay expected to expire within a test */
    constexpr unsigned int FAST_MS = 10;

    /** @brief Delay noticeably longer than FAST_MS */
    constexpr unsigned int SLOW_MS = 300;

    /** @brief Delay of an action cancelled before it runs */
    constexpr unsigned int CANCELLED_DELAY_MS = 100;

    /** @brief Maximum delay of an action cancelled before it runs */
    constexpr unsigned int CANCELLED_MAX_DELAY_MS = 150;

    /** @brief Time waited after a cancellation, longer than the cancelled delays */
    constexpr milliseconds SILENCE_PERIOD{300};

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

    /** @brief Delay of the stress test, short so that many restarts land on an expiry */
    constexpr unsigned int STRESS_DELAY_MS = 2;

    /** @brief Number of restarts of the stress test landing around an expiry */
    constexpr int STRESS_ROUND_COUNT = 300;

    /** @brief Shortest wait of a stress round before restarting, below the delay */
    constexpr std::chrono::microseconds STRESS_FIRST_RESTART_WAIT{1500};

    /** @brief Increment of the stress round wait, which sweeps around the expiry */
    constexpr std::chrono::microseconds STRESS_RESTART_WAIT_STEP{100};

    /** @brief Number of different waits swept by the stress rounds */
    constexpr int STRESS_RESTART_WAIT_COUNT = 10;

    /** @brief Poll period of a stress round waiting for the action */
    constexpr std::chrono::microseconds STRESS_POLL_PERIOD{100};

    /** @brief Time a stress round waits for the action following its restart */
    constexpr milliseconds STRESS_ROUND_DEADLINE{200};

    class ActionDelayerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());
        }

        /** @brief Action counting its calls and remembering when the first one happened */
        std::function<void()> countingAction()
        {
            return [this]() {
                if (m_fireCount.load() == 0)
                {
                    m_firstFireTime.store(Clock::now());
                }
                ++m_fireCount;
            };
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] int getFireCount() const
        {
            return m_fireCount.load();
        }

        [[nodiscard]] Clock::time_point getFirstFireTime() const
        {
            return m_firstFireTime.load();
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::atomic<int> m_fireCount{0};
        std::atomic<Clock::time_point> m_firstFireTime{};
    };

    TEST_F(ActionDelayerTest, DoesNotFireUntilStarted)
    {
        ActionDelayer delayer(getLoggerManager(), FAST_MS, countingAction());

        std::this_thread::sleep_for(QUIET_PERIOD);

        EXPECT_FALSE(delayer.isRunning());
        EXPECT_EQ(getFireCount(), 0);
    }

    TEST_F(ActionDelayerTest, StartImmediatelyRunsTheActionOnce)
    {
        ActionDelayer delayer(getLoggerManager(), FAST_MS, countingAction(), std::nullopt, true);

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        EXPECT_FALSE(delayer.isRunning());

        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(ActionDelayerTest, RunsTheActionOnceAfterTheDelay)
    {
        constexpr unsigned int delayMs = 50;
        ActionDelayer delayer(getLoggerManager(), delayMs, countingAction());

        const auto startTime = Clock::now();
        delayer.startOrRestart();
        EXPECT_TRUE(delayer.isRunning());

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        EXPECT_GE(getFirstFireTime() - startTime, milliseconds(delayMs));
        EXPECT_FALSE(delayer.isRunning());

        std::this_thread::sleep_for(QUIET_PERIOD);
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(ActionDelayerTest, StopCancelsThePendingAction)
    {
        ActionDelayer delayer(getLoggerManager(),
                              CANCELLED_DELAY_MS,
                              countingAction(),
                              CANCELLED_MAX_DELAY_MS);

        delayer.startOrRestart();
        delayer.stop();
        EXPECT_FALSE(delayer.isRunning());

        std::this_thread::sleep_for(SILENCE_PERIOD);
        EXPECT_EQ(getFireCount(), 0);
    }

    TEST_F(ActionDelayerTest, StopOnANeverStartedDelayerDoesNothing)
    {
        ActionDelayer delayer(getLoggerManager(), FAST_MS, countingAction(), SLOW_MS);

        delayer.stop();

        EXPECT_FALSE(delayer.isRunning());
    }

    TEST_F(ActionDelayerTest, StartOrRestartPostponesTheAction)
    {
        constexpr unsigned int delayMs = 200;
        ActionDelayer delayer(getLoggerManager(), delayMs, countingAction());

        delayer.startOrRestart();
        std::this_thread::sleep_for(milliseconds(delayMs / 2));
        const auto restartTime = Clock::now();
        delayer.startOrRestart();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 1; }));
        EXPECT_GE(getFirstFireTime() - restartTime, milliseconds(delayMs));
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(ActionDelayerTest, RestartLandingOnAnExpiryStillRunsTheActionAfterTheDelay)
    {
        // A restart made while the timer expired used to let the stale expiry run the action at
        // once, and the restarted timer then found nothing to do: no action followed the restart
        std::atomic<Clock::time_point> lastActionTime{};
        ActionDelayer delayer(getLoggerManager(), STRESS_DELAY_MS, [&lastActionTime]() {
            lastActionTime.store(Clock::now());
        });

        int lostRestarts = 0;
        for (int round = 0; round < STRESS_ROUND_COUNT; ++round)
        {
            delayer.startOrRestart();
            // Sweep around the expiry so that some restarts land on it
            std::this_thread::sleep_for(
                STRESS_FIRST_RESTART_WAIT +
                ((round % STRESS_RESTART_WAIT_COUNT) * STRESS_RESTART_WAIT_STEP));

            const auto restartTime = Clock::now();
            delayer.startOrRestart();

            const auto roundDeadline = Clock::now() + STRESS_ROUND_DEADLINE;
            while (lastActionTime.load() - restartTime < milliseconds(STRESS_DELAY_MS))
            {
                if (Clock::now() > roundDeadline)
                {
                    ++lostRestarts;
                    break;
                }
                std::this_thread::sleep_for(STRESS_POLL_PERIOD);
            }
        }

        EXPECT_EQ(lostRestarts, 0);
    }

    TEST_F(ActionDelayerTest, StartOrDelayPostponesTheAction)
    {
        constexpr unsigned int delayMs = 200;
        ActionDelayer delayer(getLoggerManager(), delayMs, countingAction());

        delayer.startOrDelay();
        std::this_thread::sleep_for(milliseconds(delayMs / 2));
        const auto delayTime = Clock::now();
        delayer.startOrDelay();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 1; }));
        EXPECT_GE(getFirstFireTime() - delayTime, milliseconds(delayMs));
        EXPECT_EQ(getFireCount(), 1);
    }

    TEST_F(ActionDelayerTest, MaxDelayCapsAnActionPostponedWithStartOrDelay)
    {
        constexpr unsigned int delayMs = 50;
        constexpr unsigned int maxDelayMs = 100;
        ActionDelayer delayer(getLoggerManager(), delayMs, countingAction(), maxDelayMs);

        // Postponing more often than the delay never lets the delay expire, only the maximum delay
        const auto startTime = Clock::now();
        const auto deadline = startTime + WAIT_DEADLINE;
        while (getFireCount() == 0 && Clock::now() < deadline)
        {
            delayer.startOrDelay();
            std::this_thread::sleep_for(POSTPONE_INTERVAL);
        }
        delayer.stop();

        ASSERT_GE(getFireCount(), 1);
        EXPECT_GE(getFirstFireTime() - startTime, milliseconds(maxDelayMs));
    }

    TEST_F(ActionDelayerTest, StartOrRestartAlsoRestartsTheMaxDelay)
    {
        constexpr unsigned int delayMs = 300;
        constexpr unsigned int maxDelayMs = 400;
        ActionDelayer delayer(getLoggerManager(), delayMs, countingAction(), maxDelayMs);

        // Postponing for longer than the maximum delay: with startOrDelay() the maximum delay would
        // have run the action, startOrRestart() restarts it as well
        const auto postponeEnd = Clock::now() + milliseconds(maxDelayMs) + 4 * POSTPONE_INTERVAL;
        auto lastRestartTime = Clock::now();
        while (Clock::now() < postponeEnd)
        {
            lastRestartTime = Clock::now();
            delayer.startOrRestart();
            std::this_thread::sleep_for(POSTPONE_INTERVAL);
        }
        EXPECT_EQ(getFireCount(), 0);

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() >= 1; }));
        EXPECT_GE(getFirstFireTime() - lastRestartTime, milliseconds(delayMs));
    }

    TEST_F(ActionDelayerTest, ActionRunsOnceWhenTheDelayExpiresBeforeTheMaxDelay)
    {
        ActionDelayer delayer(getLoggerManager(), FAST_MS, countingAction(), SLOW_MS);

        delayer.startOrDelay();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        std::this_thread::sleep_for(2 * milliseconds(SLOW_MS));
        EXPECT_EQ(getFireCount(), 1);
        EXPECT_FALSE(delayer.isRunning());
    }

    TEST_F(ActionDelayerTest, ActionRunsOnceWhenTheMaxDelayExpiresBeforeTheDelay)
    {
        ActionDelayer delayer(getLoggerManager(), SLOW_MS, countingAction(), FAST_MS);

        delayer.startOrDelay();

        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));
        std::this_thread::sleep_for(2 * milliseconds(SLOW_MS));
        EXPECT_EQ(getFireCount(), 1);
        EXPECT_FALSE(delayer.isRunning());
    }

    TEST_F(ActionDelayerTest, CanBeStartedAgainAfterTheActionRan)
    {
        ActionDelayer delayer(getLoggerManager(), FAST_MS, countingAction(), SLOW_MS);

        delayer.startOrRestart();
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 1; }));

        delayer.startOrDelay();
        EXPECT_TRUE(delayer.isRunning());
        ASSERT_TRUE(WaitUntil([this]() { return getFireCount() == 2; }));
    }

    TEST_F(ActionDelayerTest, DestroyingADelayerCancelsThePendingAction)
    {
        {
            ActionDelayer delayer(getLoggerManager(),
                                  CANCELLED_DELAY_MS,
                                  countingAction(),
                                  CANCELLED_MAX_DELAY_MS,
                                  true);
        }

        std::this_thread::sleep_for(SILENCE_PERIOD);
        EXPECT_EQ(getFireCount(), 0);
    }

} // namespace
} // namespace act::time
