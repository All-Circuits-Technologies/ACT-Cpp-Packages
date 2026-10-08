// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_foundation/logger/logger_stream.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/logger/logs_level.hpp"

namespace act::foundation
{

LoggerStream::LoggerStream(LogsLevel::Enum level, const AbsLogger &logger)
    : m_logger{logger},
      m_level{level}
{
}

LoggerStream::~LoggerStream()
{
    auto tmpLog = m_stream.str();
    if (tmpLog.empty())
    {
        // No log to perform
        return;
    }

    m_logger.log(m_level, tmpLog);
}

} // namespace act::foundation
