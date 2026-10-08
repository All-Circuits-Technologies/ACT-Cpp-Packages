// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/logs_level.hpp"

#include <map>
#include <string>
#include <vector>

namespace act::logger
{

/** @brief This defines the interface to be implemented for the printers */
class AbsExternalLogger
{
  public:
    /**
     * @brief Class constructor
     * @param minLevel The minimum logs level to log
     * @param minLevelByCategory This map defines the minimum logs level to log for each
     * category. This overrides the minLevel parameter for the specified categories. If a
     * category is not in this map, the minLevel parameter is used. If empty, only the
     * minLevel parameter is used. If a log message has multiple categories, the maximum
     * matching level in the map is used.
     */
    explicit AbsExternalLogger(
        act::foundation::LogsLevel::Enum minLevel,
        const std::map<std::string, act::foundation::LogsLevel::Enum> &minLevelByCategory = {});

    /** @brief Class destructor */
    virtual ~AbsExternalLogger() = default;

  public:
    /**
     * @brief Set the log min level
     * @param minLevel The new minimum logs level to log
     */
    void setMinLevel(act::foundation::LogsLevel::Enum minLevel)
    {
        m_minLevel = minLevel;
    }

    /**
     * @brief Set (or override) the minimum log level for a specific category.
     * @param category The category name.
     * @param minLevel The minimum log level for this category.
     */
    void setCategoryMinLevel(const std::string &category, act::foundation::LogsLevel::Enum minLevel)
    {
        m_minLevelByCategory[category] = minLevel;
    }

    /**
     * @brief Test if a log message with the given level and categories should be logged
     * @param level The logs level
     * @param categories The categories associated to the log message
     * @return True if the message should be logged, false otherwise
     */
    [[nodiscard]] bool isLoggable(act::foundation::LogsLevel::Enum level,
                                  const std::vector<std::string> &categories) const;

    /**
     * @brief Log a message
     * @param level The logs level
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    virtual void log(act::foundation::LogsLevel::Enum level,
                     const std::string &message,
                     const std::vector<std::string> &categories);

    /**
     * @brief Log a trace message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void trace(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::TRACE, message, categories);
    }

    /**
     * @brief Log a debug message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void debug(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::DEBG, message, categories);
    }

    /**
     * @brief Log an info message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void info(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::INFO, message, categories);
    }

    /**
     * @brief Log a warning message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void warning(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::WARNING, message, categories);
    }

    /**
     * @brief Log an error message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void error(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::ERR, message, categories);
    }

    /**
     * @brief Log a fatal message
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void fatal(const std::string &message, const std::vector<std::string> &categories = {})
    {
        log(act::foundation::LogsLevel::Enum::FATAL, message, categories);
    }

  protected:
    /**
     * @brief Pure virtual method to log a message, to be implemented by derived classes
     * @param level The logs level
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    virtual void logToExternal(act::foundation::LogsLevel::Enum level,
                               const std::string &message,
                               const std::vector<std::string> &categories) = 0;

  private:
    /** @brief The minimum logs level to log */
    act::foundation::LogsLevel::Enum m_minLevel;

    /**
     * @brief The minimum logs level to log for each category
     * @note The key is the category name, the value is the minimum logs level to log for
     * that category.
     * @note This overrides the minLevel parameter for the specified categories.
     * @note If a category is not in this map, the minLevel parameter is used.
     * @note If empty, only the minLevel parameter is used.
     * @note If a log message has multiple categories, the maximum matching level in the map
     * is used.
     */
    std::map<std::string, act::foundation::LogsLevel::Enum> m_minLevelByCategory;
};

} // namespace act::logger
