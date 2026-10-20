// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_linux_system_manager.cpp
 * @brief Unit tests for act::system::LinuxSystemManager.
 *
 * Runs harmless shell commands (echo, printf, exit) through the real pipe primitives to cover the
 * output capture, the success and failure exit codes, and that an escaped argument reaches the
 * command as a single argument.
 *
 * The reboot is never asked.
 */

#include "act_logger/services/logger_manager.hpp"
#include "act_system/linux/linux_system_manager.hpp"
#include "act_system/system_manager.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace act::system
{
namespace
{

    class LinuxSystemManagerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            // The manager creates its sub-logger at construction, which needs an initialized parent
            ASSERT_TRUE(m_loggerManager.init());
            m_manager = std::make_unique<LinuxSystemManager>(m_loggerManager);
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] LinuxSystemManager &getManager() const
        {
            return *m_manager;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::unique_ptr<LinuxSystemManager> m_manager;
    };

    TEST_F(LinuxSystemManagerTest, IsThePlatformSystemManager)
    {
        EXPECT_TRUE((std::is_same_v<SystemManager, LinuxSystemManager>));
    }

    TEST_F(LinuxSystemManagerTest, CallCommandCapturesTheOutputOfASucceedingCommand)
    {
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("echo hello", output, getLoggerManager()), 0);
        EXPECT_EQ(output.str(), "hello\n");
    }

    TEST_F(LinuxSystemManagerTest, CallCommandCapturesAMultiLineOutput)
    {
        constexpr std::size_t longLineLength = 300;
        const std::string longLine(longLineLength, 'y');
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("printf 'a\\nb\\n" + longLine + "\\n'",
                                           output,
                                           getLoggerManager()),
                  0);
        EXPECT_EQ(output.str(), "a\nb\n" + longLine + "\n");
    }

    TEST_F(LinuxSystemManagerTest, CallCommandReportsAFailingCommand)
    {
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("exit 3", output, getLoggerManager()), 3);
    }

    TEST_F(LinuxSystemManagerTest, CallCommandGivesNoExitCodeForAKilledCommand)
    {
        std::ostringstream output;

        // The shell kills itself, so it never exits with a code
        EXPECT_EQ(getManager().callCommand("kill -KILL $$", output, getLoggerManager()), -1);
    }

    TEST_F(LinuxSystemManagerTest, CallCommandReportsAnUnknownCommand)
    {
        std::ostringstream output;

        // A POSIX shell exits with 127 when it cannot find the command
        constexpr int commandNotFound = 127;
        EXPECT_EQ(getManager().callCommand("act_system_test_unknown_command 2>/dev/null",
                                           output,
                                           getLoggerManager()),
                  commandNotFound);
        EXPECT_TRUE(output.str().empty());
    }

    TEST_F(LinuxSystemManagerTest, CallCommandRunsTheJoinedParts)
    {
        std::ostringstream output;
        const std::vector<std::string> cmdParts{"echo", "a", "b"};

        EXPECT_EQ(getManager().callCommand(cmdParts, output, getLoggerManager()), 0);
        EXPECT_EQ(output.str(), "a b\n");
    }

    TEST_F(LinuxSystemManagerTest, EscapedArgumentIsPassedAsOneArgument)
    {
        std::ostringstream output;
        const std::vector<std::string> cmdParts{
            "printf",
            "'[%s]'",
            AbsSystemManager::EscapeCmdArgument("two  spaced words")};

        EXPECT_EQ(getManager().callCommand(cmdParts, output, getLoggerManager()), 0);
        EXPECT_EQ(output.str(), "[two  spaced words]");
    }

    TEST_F(LinuxSystemManagerTest, EscapedArgumentReachesTheCommandUnchanged)
    {
        // Every character the shell would otherwise interpret, quotes included
        const std::string argument = R"(it's "$HOME" `id` \n; exit 1)";

        for (const char quote :
             {AbsSystemManager::SINGLE_QUOTE_CHAR, AbsSystemManager::DOUBLE_QUOTE_CHAR})
        {
            std::ostringstream output;
            const std::vector<std::string> cmdParts{
                "printf",
                "'[%s]'",
                AbsSystemManager::EscapeCmdArgument(argument, quote)};

            EXPECT_EQ(getManager().callCommand(cmdParts, output, getLoggerManager()), 0) << quote;
            EXPECT_EQ(output.str(), "[" + argument + "]") << quote;
        }
    }

} // namespace
} // namespace act::system
