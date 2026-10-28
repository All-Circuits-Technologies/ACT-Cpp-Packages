// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_std_console_logger.cpp
 * @brief Unit tests for StdConsoleLogger: the "[LEVEL] [cat/sub] message" line format, the split
 *        between the standard output and the standard error by level, and the level filters.
 */

#include "act_logger/printers/std_console_logger.hpp"

#include <string>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    /** @brief Captures both console streams around one call */
    class StdConsoleLoggerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ::testing::internal::CaptureStdout();
            ::testing::internal::CaptureStderr();
        }

        void TearDown() override
        {
            if (!m_captured)
            {
                stopCapture();
            }
        }

        /** @brief Stop capturing and keep what each stream received */
        void stopCapture()
        {
            m_stdout = ::testing::internal::GetCapturedStdout();
            m_stderr = ::testing::internal::GetCapturedStderr();
            m_captured = true;
        }

        /** @brief Get what the standard output received, once the capture is stopped */
        [[nodiscard]] const std::string &getStdout() const
        {
            return m_stdout;
        }

        /** @brief Get what the standard error received, once the capture is stopped */
        [[nodiscard]] const std::string &getStderr() const
        {
            return m_stderr;
        }

      private:
        std::string m_stdout;
        std::string m_stderr;
        bool m_captured{false};
    };

    TEST_F(StdConsoleLoggerTest, MessageWithoutCategoryShowsTheLevelAndTheMessage)
    {
        StdConsoleLogger logger(act::foundation::LogsLevel::Enum::TRACE);

        logger.info("hello");
        stopCapture();

        EXPECT_EQ(getStdout(), "[INFO] hello\n");
        EXPECT_TRUE(getStderr().empty());
    }

    TEST_F(StdConsoleLoggerTest, CategoriesAreJoinedWithASlash)
    {
        StdConsoleLogger logger(act::foundation::LogsLevel::Enum::TRACE);

        logger.warning("message", {"main", "sub", "leaf"});
        stopCapture();

        EXPECT_EQ(getStdout(), "[WARN] [main/sub/leaf] message\n");
    }

    TEST_F(StdConsoleLoggerTest, LevelsFromTheStdErrThresholdGoToStandardError)
    {
        StdConsoleLogger logger(act::foundation::LogsLevel::Enum::TRACE,
                                act::foundation::LogsLevel::Enum::WARNING);

        logger.info("out");
        logger.warning("err1");
        logger.fatal("err2");
        stopCapture();

        EXPECT_EQ(getStdout(), "[INFO] out\n");
        EXPECT_EQ(getStderr(), "[WARN] err1\n[FATAL] err2\n");
    }

    TEST_F(StdConsoleLoggerTest, DefaultStdErrThresholdIsError)
    {
        StdConsoleLogger logger(act::foundation::LogsLevel::Enum::TRACE);

        logger.warning("out");
        logger.error("err");
        stopCapture();

        EXPECT_EQ(getStdout(), "[WARN] out\n");
        EXPECT_EQ(getStderr(), "[ERROR] err\n");
    }

    TEST_F(StdConsoleLoggerTest, FilteredMessagesAreNotPrinted)
    {
        StdConsoleLogger logger(act::foundation::LogsLevel::Enum::INFO,
                                act::foundation::LogsLevel::Enum::ERR,
                                {{"verbose", act::foundation::LogsLevel::Enum::TRACE}});

        logger.debug("dropped");
        logger.debug("kept", {"verbose"});
        stopCapture();

        EXPECT_EQ(getStdout(), "[DEBUG] [verbose] kept\n");
        EXPECT_TRUE(getStderr().empty());
    }

} // namespace
} // namespace act::logger
