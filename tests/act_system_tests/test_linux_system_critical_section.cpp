// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_linux_system_critical_section.cpp
 * @brief Unit tests for act::system::LinuxSystemCriticalSection.
 *
 * Covers the lock file creation, entering and leaving, the exclusion between two critical
 * sections sharing a slug (in the same process and from a child process), the release of the lock
 * on destruction, and the failures reported when the lock file cannot be opened.
 *
 * Every test uses its own slug, and the lock file it creates is removed afterwards.
 */

#include "act_logger/services/logger_manager.hpp"
#include "act_system/linux/linux_system_critical_section.hpp"
#include "act_system/system_critical_section.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

namespace act::system
{
namespace
{

    using std::chrono::milliseconds;

    /** @brief Time left to a blocked enter() to show it does not return */
    constexpr milliseconds BLOCKED_PERIOD{150};

    /** @brief Deadline after which an enter() expected to return is reported as failed */
    constexpr milliseconds WAIT_DEADLINE{5000};

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

    /** @brief Count the file descriptors currently open by this process */
    std::size_t CountOpenFileDescriptors()
    {
        const std::filesystem::directory_iterator descriptors("/proc/self/fd");
        return static_cast<std::size_t>(
            std::distance(std::filesystem::begin(descriptors), std::filesystem::end(descriptors)));
    }

    class LinuxSystemCriticalSectionTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
            m_slug = std::string("act_system_test_") + std::to_string(::getpid()) + "_" +
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

    TEST_F(LinuxSystemCriticalSectionTest, IsThePlatformCriticalSection)
    {
        EXPECT_TRUE((std::is_same_v<SystemCriticalSection, LinuxSystemCriticalSection>));
    }

    TEST_F(LinuxSystemCriticalSectionTest, ConstructionCreatesTheLockFileWithoutLocking)
    {
        const LinuxSystemCriticalSection criticalSection(getSlug(), getLoggerManager());

        EXPECT_TRUE(std::filesystem::exists(getLockFilePath()));
        EXPECT_FALSE(IsLockHeld(getLockFilePath()));
    }

    TEST_F(LinuxSystemCriticalSectionTest, EnterTakesTheLockAndLeaveReleasesIt)
    {
        const LinuxSystemCriticalSection criticalSection(getSlug(), getLoggerManager());

        ASSERT_TRUE(criticalSection.enter());
        EXPECT_TRUE(IsLockHeld(getLockFilePath()));

        ASSERT_TRUE(criticalSection.leave());
        EXPECT_FALSE(IsLockHeld(getLockFilePath()));
    }

    TEST_F(LinuxSystemCriticalSectionTest, LeaveWithoutEnterSucceeds)
    {
        const LinuxSystemCriticalSection criticalSection(getSlug().c_str(), getLoggerManager());

        EXPECT_TRUE(criticalSection.leave());
    }

    TEST_F(LinuxSystemCriticalSectionTest, SecondSectionWithTheSameSlugWaitsForTheFirstToLeave)
    {
        const LinuxSystemCriticalSection first(getSlug(), getLoggerManager());
        const LinuxSystemCriticalSection second(getSlug().c_str(), getLoggerManager());
        ASSERT_TRUE(first.enter());

        std::atomic<bool> secondEntered{false};
        std::thread waiter([&second, &secondEntered]() {
            if (second.enter())
            {
                secondEntered.store(true);
            }
        });

        std::this_thread::sleep_for(BLOCKED_PERIOD);
        EXPECT_FALSE(secondEntered.load());

        ASSERT_TRUE(first.leave());
        waiter.join();
        EXPECT_TRUE(secondEntered.load());
        EXPECT_TRUE(second.leave());
    }

    TEST_F(LinuxSystemCriticalSectionTest, SectionsWithDifferentSlugsDoNotExcludeEachOther)
    {
        const std::string otherSlug = getSlug() + "_other";
        const std::string otherLockFilePath = "/tmp/" + otherSlug + ".lock";

        {
            const LinuxSystemCriticalSection first(getSlug(), getLoggerManager());
            const LinuxSystemCriticalSection second(otherSlug, getLoggerManager());

            ASSERT_TRUE(first.enter());
            EXPECT_FALSE(IsLockHeld(otherLockFilePath));
            EXPECT_TRUE(second.enter());
        }

        std::filesystem::remove(otherLockFilePath);
    }

    TEST_F(LinuxSystemCriticalSectionTest, DestructionReleasesTheLock)
    {
        {
            const LinuxSystemCriticalSection criticalSection(getSlug(), getLoggerManager());
            ASSERT_TRUE(criticalSection.enter());
            ASSERT_TRUE(IsLockHeld(getLockFilePath()));
        }

        EXPECT_FALSE(IsLockHeld(getLockFilePath()));
    }

    TEST_F(LinuxSystemCriticalSectionTest, DestructionClosesTheLockFile)
    {
        constexpr int sectionCount = 10;
        const std::size_t openBefore = CountOpenFileDescriptors();

        for (int index = 0; index < sectionCount; ++index)
        {
            const LinuxSystemCriticalSection criticalSection(getSlug(), getLoggerManager());
            ASSERT_TRUE(criticalSection.enter());
        }

        EXPECT_EQ(CountOpenFileDescriptors(), openBefore);
    }

    TEST_F(LinuxSystemCriticalSectionTest, LockIsSeenFromAnotherProcess)
    {
        const LinuxSystemCriticalSection criticalSection(getSlug(), getLoggerManager());

        const auto childSeesTheLock = [this]() {
            const pid_t pid = ::fork();
            if (pid == 0)
            {
                ::_exit(IsLockHeld(getLockFilePath()) ? 1 : 0);
            }
            int status = 0;
            ::waitpid(pid, &status, 0);
            return WIFEXITED(status) && (WEXITSTATUS(status) == 1);
        };

        ASSERT_TRUE(criticalSection.enter());
        EXPECT_TRUE(childSeesTheLock());

        ASSERT_TRUE(criticalSection.leave());
        EXPECT_FALSE(childSeesTheLock());
    }

    TEST_F(LinuxSystemCriticalSectionTest, EnterAndLeaveFailWhenTheLockFileCannotBeOpened)
    {
        // The slug points into a directory which does not exist, so the lock file cannot be created
        const std::string slug = getSlug() + "_missing_dir/section";
        ASSERT_FALSE(std::filesystem::exists("/tmp/" + getSlug() + "_missing_dir"));

        const LinuxSystemCriticalSection criticalSection(slug, getLoggerManager());

        EXPECT_FALSE(criticalSection.enter());
        EXPECT_FALSE(criticalSection.leave());
    }

} // namespace
} // namespace act::system
