// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_finally.cpp
 * @brief Unit tests of Finally.
 *
 * Covers the scope exit contract: the function runs exactly once when the object goes out of
 * scope, never after cancel(), and an exception it throws is logged as an error instead of
 * escaping the destructor.
 */

#include "act_foundation/finally.hpp"
#include "act_test_common/recording_logger.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace act::foundation
{
namespace
{

    TEST(FinallyTest, RunsTheFunctionOnceWhenGoingOutOfScope)
    {
        act::tests::RecordingLogger logger;
        int calls = 0;

        {
            Finally finally([&calls]() { ++calls; }, logger);
            EXPECT_EQ(calls, 0);
        }

        EXPECT_EQ(calls, 1);
        EXPECT_TRUE(logger.getEntries().empty());
    }

    TEST(FinallyTest, CancelPreventsTheFunctionFromRunning)
    {
        act::tests::RecordingLogger logger;
        int calls = 0;

        {
            Finally finally([&calls]() { ++calls; }, logger);
            finally.cancel();
        }

        EXPECT_EQ(calls, 0);
    }

    TEST(FinallyTest, CancellingTwiceKeepsTheFunctionDisabled)
    {
        act::tests::RecordingLogger logger;
        int calls = 0;

        {
            Finally finally([&calls]() { ++calls; }, logger);
            finally.cancel();
            finally.cancel();
        }

        EXPECT_EQ(calls, 0);
    }

    TEST(FinallyTest, ExceptionThrownByTheFunctionIsLoggedAsAnError)
    {
        act::tests::RecordingLogger logger;

        EXPECT_NO_THROW(
            { Finally finally([]() { throw std::runtime_error("cleanup failed"); }, logger); });

        EXPECT_EQ(logger.count(act::foundation::LogsLevel::ERR), 1U);
        EXPECT_TRUE(logger.contains(act::foundation::LogsLevel::ERR, "cleanup failed"));
    }

    TEST(FinallyTest, NestedObjectsRunInReverseOrderOfCreation)
    {
        act::tests::RecordingLogger logger;
        std::string order;

        {
            Finally outer([&order]() { order += "outer;"; }, logger);
            Finally inner([&order]() { order += "inner;"; }, logger);
        }

        EXPECT_EQ(order, "inner;outer;");
    }

} // namespace
} // namespace act::foundation
