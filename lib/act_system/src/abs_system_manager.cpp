// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/abs_system_manager.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/logger/logs_level.hpp"
#include "act_text/vector_string_util.hpp"
#include "act_threading/reusable_thread.hpp"
#include "act_threading/types/reusable_thread_result.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <exception>
#include <iostream>
#include <thread>
#include <vector>

namespace act::system
{

AbsSystemManager::AbsSystemManager(act::foundation::AbsLogger &parentLogger)
    : m_logger{parentLogger.createAbsSubLogger(LOGGER_CATEGORY, act::foundation::LogsLevel::TRACE)},
      m_rebootThread{new act::threading::ReusableThread()}
{
}

AbsSystemManager::~AbsSystemManager()
{
    if (m_rebootThread != nullptr)
    {
        delete m_rebootThread;
        m_rebootThread = nullptr;
    }
}

bool AbsSystemManager::init()
{
    return true;
}

act::threading::ReusableThreadResult::Enum AbsSystemManager::askReboot(int delayInSec)
{
    return m_rebootThread->start(*m_logger, RebootThreadFunction, delayInSec, this);
}

int AbsSystemManager::callCommand(const std::string &cmd,
                                  std::ostream &output,
                                  const act::foundation::AbsLogger &logger)
{
    FILE *pipe = nullptr;
    try
    {
        pipe = openPipe(cmd);
    }
    catch (const std::exception &e)
    {
        logger.errorStream() << "Exception caught: " << e.what();
    }

    if (pipe == nullptr)
    {
        logger.errorStream() << "Failed to open pipe for command: " << cmd;
        return -1;
    }

    std::array<char, CMD_OUTPUT_BUFFER_SIZE> buffer{};
    while (fgets(buffer.data(), CMD_OUTPUT_BUFFER_SIZE, pipe) != nullptr)
    {
        output << buffer.data();
    }

    return closePipe(pipe);
}

std::string AbsSystemManager::EscapeCmdArgument(const std::string &arg, char escapeChar)
{
    std::string escaped(1, escapeChar);
    for (const char character : arg)
    {
        if (escapeChar == SINGLE_QUOTE_CHAR && character == SINGLE_QUOTE_CHAR)
        {
            // Nothing can be escaped between single quotes
            escaped += SINGLE_QUOTE_IN_SINGLE_QUOTES;
            continue;
        }

        if (escapeChar == DOUBLE_QUOTE_CHAR &&
            DOUBLE_QUOTED_SPECIAL_CHARS.find(character) != std::string_view::npos)
        {
            escaped += BACKSLASH_CHAR;
        }

        escaped += character;
    }
    escaped += escapeChar;
    return escaped;
}

int AbsSystemManager::callCommand(const std::vector<std::string> &cmdParts,
                                  std::ostream &output,
                                  const act::foundation::AbsLogger &logger)
{
    auto cmd = act::text::VectorStringUtil::join(cmdParts, CMD_PART_SEPARATOR);
    return callCommand(cmd, output, logger);
}

void AbsSystemManager::RebootThreadFunction(int delayInSec, AbsSystemManager *systemManager)
{
    auto logger = systemManager->m_logger;

    systemManager->syncBeforeReboot();

    if (delayInSec > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(delayInSec));
    }

    int cmdReturn = -1;
    {
        logger->infoStream() << "Executing reboot command...";
        auto traceStream = logger->traceStream();
        cmdReturn = systemManager->callCommand(REBOOT_CMD_NAME, traceStream.getStream(), *logger);
    }

    if (cmdReturn != 0)
    {
        // For now, we don't manage reboot failures further than logging an error, because we
        // expect the system to reboot properly when the command is called. If it appears that
        // some systems may fail to reboot properly, we may need to implement a more robust
        // mechanism to handle such failures, such as events notifications
        logger->errorStream() << "Reboot command failed with return code: " << cmdReturn;
        return;
    }

    systemManager->m_rebootAsked = true;
}

} // namespace act::system
