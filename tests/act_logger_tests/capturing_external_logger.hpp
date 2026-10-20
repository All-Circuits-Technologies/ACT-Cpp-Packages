// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file capturing_external_logger.hpp
 * @brief External logger test double which records every message it is asked to print.
 */

#pragma once

#include "act_foundation/logger/logs_level.hpp"
#include "act_logger/printers/abs_external_logger.hpp"

#include <map>
#include <string>
#include <vector>

namespace act::logger::test
{

/** @brief One message received by the capturing logger */
struct LogEntry
{
    act::foundation::LogsLevel::Enum level;
    std::string message;
    std::vector<std::string> categories;
};

/** @brief External logger which keeps the messages that pass its filters instead of printing */
class CapturingExternalLogger : public AbsExternalLogger
{
  public:
    explicit CapturingExternalLogger(
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE,
        const std::map<std::string, act::foundation::LogsLevel::Enum> &minLevelByCategory = {})
        : AbsExternalLogger(minLevel, minLevelByCategory)
    {
    }

    ~CapturingExternalLogger() override = default;

  public:
    [[nodiscard]] const std::vector<LogEntry> &getEntries() const
    {
        return m_entries;
    }

  protected:
    void logToExternal(act::foundation::LogsLevel::Enum level,
                       const std::string &message,
                       const std::vector<std::string> &categories) override
    {
        m_entries.push_back(LogEntry{level, message, categories});
    }

  private:
    std::vector<LogEntry> m_entries;
};

} // namespace act::logger::test
