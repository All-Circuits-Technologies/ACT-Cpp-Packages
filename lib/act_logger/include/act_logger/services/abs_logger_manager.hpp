// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/services/abs_manager.hpp"
#include "act_logger/helpers/logger_helper.hpp"

#include <memory>

namespace act::logger
{

class AbsExternalLogger;
class LoggerHelper;

/** @brief The LoggerManager class handles logging operations */
class AbsLoggerManager : public act::foundation::AbsManager, public act::foundation::AbsLogger
{
  public:
    /**
     * @brief Constructor
     */
    explicit AbsLoggerManager();

    /**
     * @brief Destructor
     */
    ~AbsLoggerManager() override = default;

  public:
    /**
     * @brief Create a sub-logger with an additional category
     * @param category The additional category for the sub-logger
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<LoggerHelper> createSubLogger(
        const std::string &category,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE) const
    {
        return m_loggerHelper->createSubLogger(category, minLevel);
    }

    /**
     * @brief Create a sub-logger with an additional category
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<LoggerHelper> createSubLogger(
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE) const
    {
        return m_loggerHelper->createSubLogger(minLevel);
    }

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param category The additional category for the sub-logger
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        const std::string &category, act::foundation::LogsLevel::Enum minLevel) override
    {
        return m_loggerHelper->createSubLogger(category, minLevel);
    }

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        act::foundation::LogsLevel::Enum minLevel) override
    {
        return m_loggerHelper->createSubLogger(minLevel);
    }

    /**
     * @brief Initialize the manager
     * @return true if initialization was successful, false otherwise
     */
    bool init() override;

    /**
     * @brief Log a message
     * @param level The logs level
     * @param message The message to log
     */
    void log(act::foundation::LogsLevel::Enum level, const std::string &message) const override
    {
        m_loggerHelper->log(level, message);
    }

    /**
     * @brief Get a logger stream to log using stream syntax
     * @param level The logs level
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream logStream(
        act::foundation::LogsLevel::Enum level) const override
    {
        return m_loggerHelper->logStream(level);
    }

    /**
     * @brief Log a trace message
     * @param message The message to log
     */
    void trace(const std::string &message) const override
    {
        m_loggerHelper->trace(message);
    }

    /**
     * @brief Get a trace logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream traceStream() const override
    {
        return m_loggerHelper->traceStream();
    }

    /**
     * @brief Log a debug message
     * @param message The message to log
     */
    void debug(const std::string &message) const override
    {
        m_loggerHelper->debug(message);
    }

    /**
     * @brief Get a debug logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream debugStream() const override
    {
        return m_loggerHelper->debugStream();
    }

    /**
     * @brief Log an info message
     * @param message The message to log
     */
    void info(const std::string &message) const override
    {
        m_loggerHelper->info(message);
    }

    /**
     * @brief Get an info logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream infoStream() const override
    {
        return m_loggerHelper->infoStream();
    }

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void warning(const std::string &message) const override
    {
        m_loggerHelper->warning(message);
    }

    /**
     * @brief Get a warning logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream warningStream() const override
    {
        return m_loggerHelper->warningStream();
    }

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void error(const std::string &message) const override
    {
        m_loggerHelper->error(message);
    }

    /**
     * @brief Get an error logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream errorStream() const override
    {
        return m_loggerHelper->errorStream();
    }

    /**
     * @brief Log a fatal message
     * @param message The message to log
     */
    void fatal(const std::string &message) const override
    {
        m_loggerHelper->fatal(message);
    }

    /**
     * @brief Get a fatal logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] act::foundation::LoggerStream fatalStream() const override
    {
        return m_loggerHelper->fatalStream();
    }

    /**
     * @brief Test if a log message with the given level would be logged
     * @note The method also tests the external logger if set
     * @param level The logs level
     * @return True if the message would be logged, false otherwise
     */
    [[nodiscard]] bool wouldBeLogged(act::foundation::LogsLevel::Enum level) const override
    {
        return m_loggerHelper->wouldBeLogged(level);
    }

  protected:
    /**
     * @brief Get the external logger
     * @return The external logger
     */
    [[nodiscard]] virtual std::shared_ptr<AbsExternalLogger> getExternalLogger() const = 0;

  private:
    /** @brief The logger helper */
    std::shared_ptr<LoggerHelper> m_loggerHelper;
};

} // namespace act::logger
