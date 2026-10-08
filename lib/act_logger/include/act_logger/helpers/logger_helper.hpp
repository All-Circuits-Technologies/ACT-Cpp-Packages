// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/logger/logger_stream.hpp"
#include "act_foundation/logger/logs_level.hpp"

#include <memory>
#include <string>
#include <vector>

namespace act::logger
{

class AbsExternalLogger;

/** @brief This is a helper class to log messages with categories and minimum level */
class LoggerHelper : public act::foundation::AbsLogger
{
  public:
    /**
     * @brief Class constructor
     * @param externalLogger The external logger to use
     * @param category The main category for this logger
     * @param minLevel The minimum logs level for this logger
     */
    explicit LoggerHelper(
        const std::shared_ptr<AbsExternalLogger> &externalLogger,
        const std::string &category,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /**
     * @brief Class constructor
     * @note This constructor does not set any category for the logger
     * @param externalLogger The external logger to use
     * @param minLevel The minimum logs level for this logger
     */
    explicit LoggerHelper(
        const std::shared_ptr<AbsExternalLogger> &externalLogger,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /** @brief Class destructor */
    ~LoggerHelper() override = default;

  protected:
    /**
     * @brief Class constructor
     * @note This constructor is used to create a sub-logger, the external logger is not set
     * here, we will use the parent logger's external logger.
     * @param categories The categories for this logger
     * @param minLevel The minimum logs level for this logger
     */
    explicit LoggerHelper(const std::vector<std::string> &categories,
                          act::foundation::LogsLevel::Enum minLevel);

  public:
    /**
     * @brief Get the external logger
     * @return The external logger
     */
    [[nodiscard]] virtual const std::shared_ptr<AbsExternalLogger> &getLogger() const
    {
        return m_externalLogger;
    }

    /**
     * @brief Update the external logger
     * @param externalLogger The new external logger
     */
    virtual void updateLogger(const std::shared_ptr<AbsExternalLogger> &externalLogger)
    {
        m_externalLogger = externalLogger;
    }

  public:
    /**
     * @brief Get the list of categories for this logger
     * @return The list of categories
     */
    [[nodiscard]] const std::vector<std::string> &getCategories() const
    {
        return m_categories;
    }

    /**
     * @brief Create a sub-logger with an additional category
     * @param category The additional category for the sub-logger
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<LoggerHelper> createSubLogger(
        const std::string &category,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /**
     * @brief Create a sub-logger with an additional category
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<LoggerHelper> createSubLogger(
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param category The additional category for the sub-logger
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        const std::string &category, act::foundation::LogsLevel::Enum minLevel) override
    {
        return createSubLogger(category, minLevel);
    }

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        act::foundation::LogsLevel::Enum minLevel) override
    {
        return createSubLogger(minLevel);
    }

    /**
     * @brief Test if a log message with the given level would be logged
     * @note The method also tests the external logger if set
     * @param level The logs level
     * @return True if the message would be logged, false otherwise
     */
    [[nodiscard]] bool wouldBeLogged(act::foundation::LogsLevel::Enum level) const override;

    /**
     * @brief Log a message
     * @param level The logs level
     * @param message The message to log
     */
    void log(act::foundation::LogsLevel::Enum level, const std::string &message) const override;

    /**
     * @brief Get a logger stream to log using stream syntax
     * @param level The logs level
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream logStream(
        act::foundation::LogsLevel::Enum level) const override
    {
        return act::foundation::LoggerStream(level, *this);
    }

    /**
     * @brief Log a trace message
     * @param message The message to log
     */
    void trace(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::TRACE, message);
    }

    /**
     * @brief Get a trace logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream traceStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::TRACE);
    }

    /**
     * @brief Log a debug message
     * @param message The message to log
     */
    void debug(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::DEBG, message);
    }

    /**
     * @brief Get a debug logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream debugStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::DEBG);
    }

    /**
     * @brief Log an info message
     * @param message The message to log
     */
    void info(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::INFO, message);
    }

    /**
     * @brief Get an info logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream infoStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::INFO);
    }

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void warning(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::WARNING, message);
    }

    /**
     * @brief Get a warning logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream warningStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::WARNING);
    }

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void error(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::ERR, message);
    }

    /**
     * @brief Get an error logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream errorStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::ERR);
    }

    /**
     * @brief Log a fatal message
     * @param message The message to log
     */
    void fatal(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::Enum::FATAL, message);
    }

    /**
     * @brief Get a fatal logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream fatalStream() const override
    {
        return logStream(act::foundation::LogsLevel::Enum::FATAL);
    }

  private:
    /**
     * @brief Test if a log message with the given level should be logged
     * @param level The logs level
     * @return True if the message should be logged, false otherwise
     */
    [[nodiscard]] bool testIfLoggable(act::foundation::LogsLevel::Enum level) const
    {
        return level >= m_minLevel;
    }

  private:
    /**
     * @brief This is the list of categories for this logger.
     * @note It can be used to filter logs by categories or to add more context to the logs.
     * @note The first category is the main category of the logger and the last one is the
     * most specific.
     */
    std::vector<std::string> m_categories;

    /**
     * @brief This is the minimum logs level for this logger.
     * @note If a log message has a level lower than this, it will not be logged.
     */
    act::foundation::LogsLevel::Enum m_minLevel;

    /**
     * @brief This is the external logger used to log messages.
     * @note If this is not set, the logs will not be logged.
     */
    std::shared_ptr<AbsExternalLogger> m_externalLogger;
};

} // namespace act::logger
