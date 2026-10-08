// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/helpers/multi_external_logger.hpp"

#include "act_foundation/logger/logs_level.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include <memory>
#include <string>
#include <vector>

namespace act::logger
{

MultiExternalLogger::MultiExternalLogger(
    const std::vector<std::shared_ptr<AbsExternalLogger>> &loggers)
    : AbsExternalLogger(act::foundation::LogsLevel::Enum::TRACE),
      m_loggers{loggers}
{
}

void MultiExternalLogger::log(act::foundation::LogsLevel::Enum level,
                              const std::string &message,
                              const std::vector<std::string> &categories)
{
    for (const auto &logger : m_loggers)
    {
        if (logger)
        {
            logger->log(level, message, categories);
        }
    }
}

} // namespace act::logger
