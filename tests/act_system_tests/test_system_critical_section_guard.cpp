// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_system_critical_section_guard.cpp
 * @brief Unit tests for act::system::SystemCriticalSectionGuard.
 *
 * Covers that the guard holds the critical section for its whole lifetime and releases it on
 * destruction, so that another critical section with the same slug can then be entered.
 */

#include "act_logger/services/logger_manager.hpp"
#include "act_system/system_critical_section.hpp"
#include "act_system/system_critical_section_guard.hpp"

#include <filesystem>
#include <string>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <gtest/gtest.h>

namespace act::system
{
namespace
{

    /**
     * @brief Test if the lock of @p lockFilePath is held by another open file description
     * @note Tries a non-blocking exclusive lock on a fresh descriptor, and releases it on success.
     */
    bool IsLockHeld(const std::string &lockFilePath)
    {
        const int fd = ::open(lockFilePath.c_str(), O_RDWR);
        if (fd < 0)
        {
            return false;
        }

        const bool held = (::flock(fd, LOCK_EX | LOCK_NB) != 0);
        ::close(fd);
        return held;
    }

    class SystemCriticalSectionGuardTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
            m_slug = std::string("act_system_guard_test_") + std::to_string(::getpid()) + "_" +
                     testInfo->name();
            m_lockFilePath = "/tmp/" + m_slug + ".lock";
            std::filesystem::remove(m_lockFilePath);
        }

        void TearDown() override
        {
            std::filesystem::remove(m_lockFilePath);
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] const std::string &getSlug() const
        {
            return m_slug;
        }

        [[nodiscard]] const std::string &getLockFilePath() const
        {
            return m_lockFilePath;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::string m_slug;
        std::string m_lockFilePath;
    };

    TEST_F(SystemCriticalSectionGuardTest, HoldsTheSectionForItsLifetime)
    {
        SystemCriticalSection criticalSection(getSlug(), getLoggerManager());
        ASSERT_FALSE(IsLockHeld(getLockFilePath()));

        {
            const SystemCriticalSectionGuard guard(criticalSection);
            EXPECT_TRUE(IsLockHeld(getLockFilePath()));
        }

        EXPECT_FALSE(IsLockHeld(getLockFilePath()));
    }

    TEST_F(SystemCriticalSectionGuardTest, SectionCanBeGuardedAgainAfterRelease)
    {
        SystemCriticalSection criticalSection(getSlug(), getLoggerManager());
        SystemCriticalSection otherSection(getSlug(), getLoggerManager());

        {
            const SystemCriticalSectionGuard guard(criticalSection);
        }

        {
            const SystemCriticalSectionGuard guard(otherSection);
            EXPECT_TRUE(IsLockHeld(getLockFilePath()));
        }

        EXPECT_FALSE(IsLockHeld(getLockFilePath()));
    }

} // namespace
} // namespace act::system
