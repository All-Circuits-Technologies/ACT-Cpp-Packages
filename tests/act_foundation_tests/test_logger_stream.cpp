// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_logger_stream.cpp
 * @brief Unit tests for LoggerStream: the streamed values are joined into one message, logged
 *        once with the stream level when the stream is destroyed, and an empty stream logs
 *        nothing.
 */

#include "act_foundation/logger/logger_stream.hpp"
#include "act_test_common/recording_logger.hpp"

#include <gtest/gtest.h>

namespace act::foundation
{
namespace
{

    using act::tests::RecordingLogger;

    TEST(LoggerStreamTest, StreamedValuesAreLoggedAsOneMessageOnDestruction)
    {
        constexpr int integer = 42;
        constexpr double real = 1.5;
        const RecordingLogger logger;

        {
            LoggerStream stream(LogsLevel::Enum::WARNING, logger);
            stream << "value " << integer << ' ' << real;

            // Nothing is logged while the stream is alive
            EXPECT_TRUE(logger.getEntries().empty());
        }

        ASSERT_EQ(logger.getEntries().size(), 1U);
        EXPECT_EQ(logger.getEntries()[0].level, LogsLevel::Enum::WARNING);
        EXPECT_EQ(logger.getEntries()[0].message, "value 42 1.5");
    }

    TEST(LoggerStreamTest, EmptyStreamLogsNothing)
    {
        const RecordingLogger logger;

        {
            const LoggerStream stream(LogsLevel::Enum::ERR, logger);
        }

        EXPECT_TRUE(logger.getEntries().empty());
    }

    TEST(LoggerStreamTest, UnderlyingStreamContributesToTheMessage)
    {
        const RecordingLogger logger;

        {
            LoggerStream stream(LogsLevel::Enum::INFO, logger);
            stream.getStream() << "direct";
            stream << " and operator";
        }

        ASSERT_EQ(logger.getEntries().size(), 1U);
        EXPECT_EQ(logger.getEntries()[0].message, "direct and operator");
    }

    TEST(LoggerStreamTest, TemporaryStreamLogsAtTheEndOfTheStatement)
    {
        const RecordingLogger logger;

        logger.errorStream() << "first";
        logger.infoStream() << "second";

        ASSERT_EQ(logger.getEntries().size(), 2U);
        EXPECT_EQ(logger.getEntries()[0].level, LogsLevel::Enum::ERR);
        EXPECT_EQ(logger.getEntries()[0].message, "first");
        EXPECT_EQ(logger.getEntries()[1].level, LogsLevel::Enum::INFO);
        EXPECT_EQ(logger.getEntries()[1].message, "second");
    }

} // namespace
} // namespace act::foundation
