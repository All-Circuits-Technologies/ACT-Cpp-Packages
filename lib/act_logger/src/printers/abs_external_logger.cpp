// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/printers/abs_external_logger.hpp"

#include <algorithm>
#include <optional>

namespace act::logger
{

AbsExternalLogger::AbsExternalLogger(
    LogsLevel::Enum minLevel, const std::map<std::string, LogsLevel::Enum> &minLevelByCategory)
    : m_minLevel{minLevel},
      m_minLevelByCategory{minLevelByCategory}
{
}

bool AbsExternalLogger::isLoggable(LogsLevel::Enum level,
                                   const std::vector<std::string> &categories) const
{
    // The categories found in the map override the global minimum; when several are found, the
    // highest of their levels applies
    std::optional<LogsLevel::Enum> categoryMinLevel;
    for (const auto &category : categories)
    {
        auto it = m_minLevelByCategory.find(category);
        if (it != m_minLevelByCategory.end())
        {
            categoryMinLevel = std::max(categoryMinLevel.value_or(it->second), it->second);
        }
    }

    return level >= categoryMinLevel.value_or(m_minLevel);
}

void AbsExternalLogger::log(LogsLevel::Enum level,
                            const std::string &message,
                            const std::vector<std::string> &categories)
{
    if (!isLoggable(level, categories))
    {
        // Nothing to do
        return;
    }

    logToExternal(level, message, categories);
}

} // namespace act::logger
