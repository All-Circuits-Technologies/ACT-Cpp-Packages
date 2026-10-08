// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/helpers/logger_helper.hpp"

#include "act_logger/helpers/sub_logger_helper.hpp"
#include "act_logger/printers/abs_external_logger.hpp"

namespace act::logger
{

LoggerHelper::LoggerHelper(const std::shared_ptr<AbsExternalLogger> &externalLogger,
                           const std::string &category,
                           act::foundation::LogsLevel::Enum minLevel)
    : m_categories{category},
      m_minLevel{minLevel},
      m_externalLogger{externalLogger}
{
}

LoggerHelper::LoggerHelper(const std::shared_ptr<AbsExternalLogger> &externalLogger,
                           act::foundation::LogsLevel::Enum minLevel)
    : m_minLevel{minLevel},
      m_externalLogger{externalLogger}
{
}

LoggerHelper::LoggerHelper(const std::vector<std::string> &categories,
                           act::foundation::LogsLevel::Enum minLevel)
    : m_categories{categories},
      m_minLevel{minLevel},
      m_externalLogger{nullptr}
{
}

std::shared_ptr<LoggerHelper> LoggerHelper::createSubLogger(
    const std::string &category, act::foundation::LogsLevel::Enum minLevel)
{
    return std::shared_ptr<LoggerHelper>(new SubLoggerHelper(category, *this, minLevel));
}

std::shared_ptr<LoggerHelper> LoggerHelper::createSubLogger(
    act::foundation::LogsLevel::Enum minLevel)
{
    return std::shared_ptr<LoggerHelper>(new SubLoggerHelper(*this, minLevel));
}

bool LoggerHelper::wouldBeLogged(act::foundation::LogsLevel::Enum level) const
{
    if (!testIfLoggable(level))
    {
        // We skip log because the level is lower than the minLevel
        return false;
    }

    // We prefer to get the logger at each log call to support sub-loggers
    const auto &logger = getLogger();

    if (logger)
    {
        return logger->isLoggable(level, m_categories);
    }

    // Without an external logger, log() drops every message
    return false;
}

void LoggerHelper::log(act::foundation::LogsLevel::Enum level, const std::string &message) const
{
    if (!testIfLoggable(level))
    {
        // We skip log because the level is lower than the minLevel
        return;
    }

    // We prefer to get the logger at each log call to support sub-loggers
    const auto &logger = getLogger();

    if (logger)
    {
        logger->log(level, message, m_categories);
    }
}

} // namespace act::logger
