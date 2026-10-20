// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/printers/std_console_logger.hpp"

#include "act_logger/printers/abs_external_logger.hpp"
#include "act_text/vector_string_util.hpp"

#include <iostream>

namespace act::logger
{

StdConsoleLogger::StdConsoleLogger(
    act::foundation::LogsLevel::Enum minLevel,
    act::foundation::LogsLevel::Enum minLevelToPrintToStdErr,
    const std::map<std::string, act::foundation::LogsLevel::Enum> &minLevelByCategory)
    : AbsExternalLogger(minLevel, minLevelByCategory),
      m_minLevelToPrintToStdErr{minLevelToPrintToStdErr}
{
}

void StdConsoleLogger::logToExternal(act::foundation::LogsLevel::Enum level,
                                     const std::string &message,
                                     const std::vector<std::string> &categories)
{
    auto formattedMessage = FormatLogMessage(level, message, categories);

    // Output to standard console. Each line is flushed on purpose: stdout is fully buffered when it
    // is not a terminal, and a crash must not lose the lines before it.
    if (level < m_minLevelToPrintToStdErr)
    {
        std::cout << formattedMessage << '\n' << std::flush;
    }
    else
    {
        std::cerr << formattedMessage << '\n' << std::flush;
    }
}

std::string StdConsoleLogger::FormatLogMessage(act::foundation::LogsLevel::Enum level,
                                               const std::string &message,
                                               const std::vector<std::string> &categories)
{
    std::string formattedMessage;

    // Add level
    formattedMessage += "[" + act::foundation::LogsLevel::ToString(level) + "] ";

    // Add categories if any
    if (!categories.empty())
    {
        formattedMessage += "[";
        formattedMessage += act::text::VectorStringUtil::join(categories, CATEGORIES_SEPARATOR);
        formattedMessage += "] ";
    }

    // Add the actual message
    formattedMessage += message;

    return formattedMessage;
}

} // namespace act::logger
