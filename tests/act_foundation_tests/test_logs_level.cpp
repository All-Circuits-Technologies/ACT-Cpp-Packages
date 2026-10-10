// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_logs_level.cpp
 * @brief Unit tests for LogsLevel: the level ordering, the names given by ToString and the
 *        integer conversion of FromLevel with its fallback for out of range values.
 */

#include "act_foundation/logger/logs_level.hpp"

#include <gtest/gtest.h>

namespace act::foundation
{
namespace
{

    TEST(LogsLevelTest, LevelsAreOrderedFromTraceToNone)
    {
        EXPECT_LT(LogsLevel::Enum::TRACE, LogsLevel::Enum::DEBG);
        EXPECT_LT(LogsLevel::Enum::DEBG, LogsLevel::Enum::INFO);
        EXPECT_LT(LogsLevel::Enum::INFO, LogsLevel::Enum::WARNING);
        EXPECT_LT(LogsLevel::Enum::WARNING, LogsLevel::Enum::ERR);
        EXPECT_LT(LogsLevel::Enum::ERR, LogsLevel::Enum::FATAL);
        EXPECT_LT(LogsLevel::Enum::FATAL, LogsLevel::Enum::NONE);
    }

    TEST(LogsLevelTest, ToStringNamesEveryLevel)
    {
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::TRACE), "TRACE");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::DEBG), "DEBUG");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::INFO), "INFO");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::WARNING), "WARN");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::ERR), "ERROR");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::FATAL), "FATAL");
        EXPECT_EQ(LogsLevel::ToString(LogsLevel::Enum::NONE), "NONE");
    }

    TEST(LogsLevelTest, FromLevelConvertsEveryValidValue)
    {
        for (int level = LogsLevel::Enum::TRACE; level <= LogsLevel::Enum::NONE; ++level)
        {
            EXPECT_EQ(LogsLevel::FromLevel(level, LogsLevel::Enum::INFO), level);
        }
    }

    TEST(LogsLevelTest, FromLevelReturnsTheDefaultForOutOfRangeValues)
    {
        EXPECT_EQ(LogsLevel::FromLevel(-1), LogsLevel::Enum::NONE);
        EXPECT_EQ(LogsLevel::FromLevel(LogsLevel::Enum::NONE + 1), LogsLevel::Enum::NONE);
        EXPECT_EQ(LogsLevel::FromLevel(-1, LogsLevel::Enum::WARNING), LogsLevel::Enum::WARNING);
        EXPECT_EQ(LogsLevel::FromLevel(100, LogsLevel::Enum::DEBG), LogsLevel::Enum::DEBG);
    }

} // namespace
} // namespace act::foundation
