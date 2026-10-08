// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/helpers/sub_logger_helper.hpp"
#include "act_foundation/logger/logs_level.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include <string>
#include <vector>

namespace act::logger
{

SubLoggerHelper::SubLoggerHelper(const std::vector<std::string> &categories,
                                 LoggerHelper &parentLogger,
                                 act::foundation::LogsLevel::Enum minLevel)
    : LoggerHelper(categories, minLevel),
      m_parentLogger(parentLogger)
{
}

SubLoggerHelper::SubLoggerHelper(const std::string &category,
                                 LoggerHelper &parentLogger,
                                 act::foundation::LogsLevel::Enum minLevel)
    : LoggerHelper(SubLoggerHelper::ConcatenateCategories(category, parentLogger), minLevel),
      m_parentLogger{parentLogger}
{
}

SubLoggerHelper::SubLoggerHelper(LoggerHelper &parentLogger,
                                 act::foundation::LogsLevel::Enum minLevel)
    : LoggerHelper(parentLogger.getCategories(), minLevel),
      m_parentLogger{parentLogger}
{
}

std::vector<std::string> SubLoggerHelper::ConcatenateCategories(const std::string &category,
                                                                const LoggerHelper &parentLogger)
{
    std::vector<std::string> categories = parentLogger.getCategories();
    categories.push_back(category);
    return categories;
}

} // namespace act::logger
