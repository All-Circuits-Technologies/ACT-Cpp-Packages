// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_system_manager.cpp
 * @brief Unit tests for act::system::AbsSystemManager.
 *
 * The platform primitives are replaced by a fake whose pipe reads from an in-memory buffer, so the
 * tests cover the command handling of the base class alone: the command given to the pipe, the
 * copy of the whole output, the exit code returned verbatim, the joining of command parts, the
 * failures to open the pipe (null pipe or exception), and the argument escaping.
 *
 * The reboot is never asked: only its initial state is checked.
 */

#include "act_foundation/constants/def_soft.hpp"
#include "act_logger/services/logger_manager.hpp"
#include "act_system/abs_system_manager.hpp"

#include <cstdio>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace act::system
{
namespace
{

    /** @brief System manager whose pipe reads a configured output instead of running a command */
    class FakeSystemManager : public AbsSystemManager
    {
      public:
        /** @brief How openPipe() behaves */
        enum class PipeBehavior
        {
            READ_OUTPUT,
            RETURN_NULL,
            THROW,
        };

        using AbsSystemManager::AbsSystemManager;

        /** @brief Choose how the next openPipe() calls behave */
        void setPipeBehavior(PipeBehavior behavior)
        {
            m_pipeBehavior = behavior;
        }

        /** @brief Set the output read through the pipe and the exit code returned on close */
        void setCommandResult(const std::string &output, int exitCode)
        {
            m_output = output;
            m_exitCode = exitCode;
        }

        /** @brief Get the command given to the last openPipe() call */
        [[nodiscard]] const std::string &getLastCommand() const
        {
            return m_lastCommand;
        }

        /** @brief Get the number of pipes closed */
        [[nodiscard]] int getClosedPipeCount() const
        {
            return m_closedPipeCount;
        }

      protected:
        FILE *openPipe(const std::string &cmd) override
        {
            m_lastCommand = cmd;
            switch (m_pipeBehavior)
            {
                case PipeBehavior::RETURN_NULL:
                    return nullptr;
                case PipeBehavior::THROW:
                    throw std::runtime_error("cannot open the pipe");
                case PipeBehavior::READ_OUTPUT:
                    break;
            }

            FILE *pipe = std::tmpfile();
            if (pipe == nullptr)
            {
                return nullptr;
            }

            if (std::fputs(m_output.c_str(), pipe) < 0)
            {
                UNUSED(std::fclose(pipe));
                return nullptr;
            }
            std::rewind(pipe);
            return pipe;
        }

        int closePipe(FILE *pipe) override
        {
            UNUSED(std::fclose(pipe));
            ++m_closedPipeCount;
            return m_exitCode;
        }

        void syncBeforeReboot() override
        {
        }

      private:
        PipeBehavior m_pipeBehavior{PipeBehavior::READ_OUTPUT};
        std::string m_output;
        int m_exitCode{0};
        std::string m_lastCommand;
        int m_closedPipeCount{0};
    };

    class AbsSystemManagerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            // The manager creates its sub-logger at construction, which needs an initialized parent
            ASSERT_TRUE(m_loggerManager.init());
            m_manager = std::make_unique<FakeSystemManager>(m_loggerManager);
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] FakeSystemManager &getManager() const
        {
            return *m_manager;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::unique_ptr<FakeSystemManager> m_manager;
    };

    TEST_F(AbsSystemManagerTest, InitSucceeds)
    {
        EXPECT_TRUE(getManager().init());
    }

    TEST_F(AbsSystemManagerTest, RebootIsNotAskedInitially)
    {
        EXPECT_FALSE(getManager().isRebootAsked());
    }

    TEST_F(AbsSystemManagerTest, CallCommandRunsTheGivenCommand)
    {
        std::ostringstream output;

        getManager().callCommand("do something", output, getLoggerManager());

        EXPECT_EQ(getManager().getLastCommand(), "do something");
    }

    TEST_F(AbsSystemManagerTest, CallCommandCopiesTheOutputAndReturnsTheExitCode)
    {
        constexpr int exitCode = 7;
        getManager().setCommandResult("first line\nsecond line\n", exitCode);
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("cmd", output, getLoggerManager()), exitCode);
        EXPECT_EQ(output.str(), "first line\nsecond line\n");
        EXPECT_EQ(getManager().getClosedPipeCount(), 1);
    }

    TEST_F(AbsSystemManagerTest, CallCommandCopiesAnOutputLongerThanItsReadBuffer)
    {
        constexpr std::size_t longLineLength = 1000;
        const std::string longLine(longLineLength, 'x');
        const std::string expectedOutput = longLine + "\n" + longLine;
        getManager().setCommandResult(expectedOutput, 0);
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("cmd", output, getLoggerManager()), 0);
        EXPECT_EQ(output.str(), expectedOutput);
    }

    TEST_F(AbsSystemManagerTest, CallCommandHandlesAnEmptyOutput)
    {
        getManager().setCommandResult("", 0);
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("cmd", output, getLoggerManager()), 0);
        EXPECT_TRUE(output.str().empty());
    }

    TEST_F(AbsSystemManagerTest, CallCommandJoinsThePartsWithSpaces)
    {
        std::ostringstream output;
        const std::vector<std::string> cmdParts{"cmd", "--flag", "value"};

        getManager().callCommand(cmdParts, output, getLoggerManager());

        EXPECT_EQ(getManager().getLastCommand(), "cmd --flag value");
    }

    TEST_F(AbsSystemManagerTest, CallCommandFailsWhenThePipeCannotBeOpened)
    {
        getManager().setPipeBehavior(FakeSystemManager::PipeBehavior::RETURN_NULL);
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("cmd", output, getLoggerManager()), -1);
        EXPECT_TRUE(output.str().empty());
        EXPECT_EQ(getManager().getClosedPipeCount(), 0);
    }

    TEST_F(AbsSystemManagerTest, CallCommandFailsWhenOpeningThePipeThrows)
    {
        getManager().setPipeBehavior(FakeSystemManager::PipeBehavior::THROW);
        std::ostringstream output;

        EXPECT_EQ(getManager().callCommand("cmd", output, getLoggerManager()), -1);
        EXPECT_EQ(getManager().getClosedPipeCount(), 0);
    }

    TEST_F(AbsSystemManagerTest, EscapeCmdArgumentWrapsInSingleQuotesByDefault)
    {
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument("a b"), "'a b'");
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument(""), "''");
    }

    TEST_F(AbsSystemManagerTest, EscapeCmdArgumentWrapsInTheGivenChar)
    {
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument("a b", AbsSystemManager::DOUBLE_QUOTE_CHAR),
                  "\"a b\"");
    }

    TEST_F(AbsSystemManagerTest, EscapeCmdArgumentEscapesSingleQuotes)
    {
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument("it's"), R"('it'\''s')");
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument("$HOME \"`"), R"('$HOME "`')");
    }

    TEST_F(AbsSystemManagerTest, EscapeCmdArgumentEscapesWhatDoubleQuotesInterpret)
    {
        EXPECT_EQ(AbsSystemManager::EscapeCmdArgument(R"(a"b\c$d`e'f)",
                                                      AbsSystemManager::DOUBLE_QUOTE_CHAR),
                  R"("a\"b\\c\$d\`e'f")");
    }

} // namespace
} // namespace act::system
