// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_mutexed.cpp
 * @brief Unit tests for Mutexed: construction forwarded to the value, the unlocked accessors, the
 *        lock held by lock() until the returned holder is released, withLock and its return
 *        value, mutual exclusion between threads and the locked comparison operators.
 */

#include "act_threading/mutexed.hpp"

#include <future>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace act::threading
{
namespace
{

    /** @brief Try to take the mutex from another thread, so the result does not depend on the
     * caller already owning it */
    template <typename T>
    bool CanBeLockedFromAnotherThread(const Mutexed<T> &mutexed)
    {
        return std::async(std::launch::async,
                          [&mutexed]() {
                              const bool locked = mutexed.mtx.try_lock();
                              if (locked)
                              {
                                  mutexed.mtx.unlock();
                              }
                              return locked;
                          })
            .get();
    }

    TEST(MutexedTest, ConstructorArgumentsAreForwardedToTheValue)
    {
        const Mutexed<std::string> repeated(3, 'a');
        const Mutexed<std::vector<int>> list(std::vector<int>{1, 2, 3});
        const Mutexed<int> defaulted;

        EXPECT_EQ(repeated.value, "aaa");
        EXPECT_EQ(list.value, (std::vector<int>{1, 2, 3}));
        EXPECT_EQ(defaulted.value, 0);
    }

    TEST(MutexedTest, AccessorsReachTheValueWithoutLocking)
    {
        Mutexed<std::string> mutexed("abc");
        const auto holder = mutexed.lock();

        // operator-> and operator* do not lock, so they work while the mutex is held
        EXPECT_EQ(mutexed->size(), 3U);
        (*mutexed).append("d");
        EXPECT_EQ(mutexed.value, "abcd");
    }

    TEST(MutexedTest, LockHoldsTheMutexUntilTheHolderIsReleased)
    {
        const Mutexed<int> mutexed(1);

        {
            const auto holder = mutexed.lock();
            EXPECT_TRUE(holder.owns_lock());
            EXPECT_FALSE(CanBeLockedFromAnotherThread(mutexed));
        }

        EXPECT_TRUE(CanBeLockedFromAnotherThread(mutexed));
    }

    TEST(MutexedTest, WithLockGivesTheValueAndReturnsTheResult)
    {
        constexpr int initialValue = 20;
        Mutexed<int> mutexed(initialValue);

        const bool lockedDuringCall = mutexed.withLock([&mutexed](int &value) {
            value += 1;
            return !CanBeLockedFromAnotherThread(mutexed);
        });
        const int doubled = mutexed.withLock([](const int &value) { return value * 2; });

        EXPECT_TRUE(lockedDuringCall);
        EXPECT_EQ(doubled, 42);
        EXPECT_EQ(mutexed.value, 21);
        EXPECT_TRUE(CanBeLockedFromAnotherThread(mutexed));
    }

    TEST(MutexedTest, WithLockSerializesConcurrentUpdates)
    {
        constexpr int threadsNb = 8;
        constexpr int incrementsNb = 10000;
        Mutexed<int> counter(0);

        std::vector<std::thread> threads;
        threads.reserve(threadsNb);
        for (int idx = 0; idx < threadsNb; ++idx)
        {
            threads.emplace_back([&counter]() {
                for (int count = 0; count < incrementsNb; ++count)
                {
                    counter.withLock([](int &value) { ++value; });
                }
            });
        }
        for (auto &thread : threads)
        {
            thread.join();
        }

        EXPECT_EQ(counter.value, threadsNb * incrementsNb);
    }

    TEST(MutexedTest, ComparisonOperatorsCompareTheValues)
    {
        const Mutexed<std::string> first("value");
        const Mutexed<std::string> same("value");
        const Mutexed<std::string> other("other");

        EXPECT_TRUE(first == same);
        EXPECT_FALSE(first != same);
        EXPECT_FALSE(first == other);
        EXPECT_TRUE(first != other);
        EXPECT_TRUE(CanBeLockedFromAnotherThread(first));
        EXPECT_TRUE(CanBeLockedFromAnotherThread(same));
    }

} // namespace
} // namespace act::threading
