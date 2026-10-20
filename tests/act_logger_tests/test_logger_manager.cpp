// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_logger_manager.cpp
 * @brief Unit tests for LoggerManager: it prints to the console from the INFO level by default,
 *        sends the levels from its standard error threshold to the standard error, and lets the
 *        console minimum level be changed globally and per category once initialized.
 */

#include "act_logger/services/logger_manager.hpp"

#include <string>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    /** @brief Captures both console streams, the manager init message included */
    class LoggerManagerTest : public ::testing::Test
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

    TEST_F(LoggerManagerTest, InitPrintsItsReadyMessageOnTheStandardOutput)
    {
        LoggerManager manager;

        ASSERT_TRUE(manager.init());
        stopCapture();

        EXPECT_EQ(getStdout(), "[INFO] LoggerManager initialized successfully.\n");
        EXPECT_TRUE(getStderr().empty());
    }

    TEST_F(LoggerManagerTest, ConsolePrintsFromInfoByDefault)
    {
        LoggerManager manager;
        ASSERT_TRUE(manager.init());

        EXPECT_FALSE(manager.wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG));
        EXPECT_TRUE(manager.wouldBeLogged(act::foundation::LogsLevel::Enum::INFO));
        manager.debug("dropped");
        manager.warning("warning");
        manager.error("error");
        stopCapture();

        EXPECT_EQ(getStdout(), "[INFO] LoggerManager initialized successfully.\n[WARN] warning\n");
        EXPECT_EQ(getStderr(), "[ERROR] error\n");
    }

    TEST_F(LoggerManagerTest, StdErrThresholdIsTheConstructorOne)
    {
        LoggerManager manager(act::foundation::LogsLevel::Enum::WARNING);
        ASSERT_TRUE(manager.init());

        manager.warning("warning");
        stopCapture();

        EXPECT_EQ(getStderr(), "[WARN] warning\n");
    }

    TEST_F(LoggerManagerTest, ConsoleMinimumLevelCanBeLowered)
    {
        LoggerManager manager;
        ASSERT_TRUE(manager.init());

        manager.setCslMinLogLevel(act::foundation::LogsLevel::Enum::TRACE);
        manager.trace("trace");
        stopCapture();

        EXPECT_EQ(getStdout(), "[INFO] LoggerManager initialized successfully.\n[TRACE] trace\n");
    }

    TEST_F(LoggerManagerTest, ConsoleCategoryLevelAppliesToThatCategoryOnly)
    {
        LoggerManager manager;
        ASSERT_TRUE(manager.init());

        manager.setCslCategoryMinLogLevel("verbose", act::foundation::LogsLevel::Enum::DEBG);
        const auto verbose = manager.createSubLogger("verbose");
        const auto other = manager.createSubLogger("other");
        verbose->debug("kept");
        other->debug("dropped");
        stopCapture();

        EXPECT_EQ(getStdout(),
                  "[INFO] LoggerManager initialized successfully.\n[DEBUG] [verbose] kept\n");
    }

    TEST_F(LoggerManagerTest, ConsoleSettersBeforeInitHaveNoEffect)
    {
        LoggerManager manager;

        manager.setCslMinLogLevel(act::foundation::LogsLevel::Enum::TRACE);
        manager.setCslCategoryMinLogLevel("verbose", act::foundation::LogsLevel::Enum::TRACE);
        ASSERT_TRUE(manager.init());

        EXPECT_FALSE(manager.wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG));
    }

} // namespace
} // namespace act::logger
