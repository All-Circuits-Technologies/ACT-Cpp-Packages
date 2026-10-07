// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/logger_stream.hpp"
#include "act_foundation/logger/logs_level.hpp"

#include <memory>
#include <string>

namespace act::foundation
{

class AbsLogger
{
  public:
    explicit AbsLogger() = default;

    /** @brief Destructor */
    virtual ~AbsLogger() = default;

  public:
    virtual void log(LogsLevel::Enum level, const std::string &message) const = 0;

    /**
     * @brief Get a logger stream to log using stream syntax
     * @param level The logs level
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream logStream(LogsLevel::Enum level) const = 0;

    /** @brief Log a trace message
     * @param message The message to log
     */
    virtual void trace(const std::string &message) const = 0;

    /**
     * @brief Get a trace logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream traceStream() const = 0;

    /** @brief Log a debug message
     * @param message The message to log
     */
    virtual void debug(const std::string &message) const = 0;

    /**
     * @brief Get a debug logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream debugStream() const = 0;

    /** @brief Log an info message
     * @param message The message to log
     */
    virtual void info(const std::string &message) const = 0;

    /**
     * @brief Get an info logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream infoStream() const = 0;

    /** @brief Log a warning message
     * @param message The message to log
     */
    virtual void warning(const std::string &message) const = 0;

    /**
     * @brief Get a warning logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream warningStream() const = 0;

    /** @brief Log an error message
     * @param message The message to log
     */
    virtual void error(const std::string &message) const = 0;

    /**
     * @brief Get an error logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream errorStream() const = 0;

    /** @brief Log a fatal message
     * @param message The message to log
     */
    virtual void fatal(const std::string &message) const = 0;

    /**
     * @brief Get a fatal logger stream to log using stream syntax
     * @return The logger stream
     */
    [[nodiscard]] virtual LoggerStream fatalStream() const = 0;

    /**
     * @brief Test if a log message with the given level would be logged
     * @note The method also tests the external logger; without one, nothing would be logged
     * @param level The logs level
     * @return True if the message would be logged, false otherwise
     */
    [[nodiscard]] virtual bool wouldBeLogged(LogsLevel::Enum level) const = 0;

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param category The additional category for the sub-logger
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] virtual std::shared_ptr<AbsLogger> createAbsSubLogger(
        const std::string &category, LogsLevel::Enum minLevel) = 0;

    /**
     * @brief Create an abstract sub-logger with an additional category
     * @param minLevel The minimum logs level for the sub-logger
     * @return The sub-logger
     */
    [[nodiscard]] virtual std::shared_ptr<AbsLogger> createAbsSubLogger(
        LogsLevel::Enum minLevel) = 0;
};

} // namespace act::foundation
