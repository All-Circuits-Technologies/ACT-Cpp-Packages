// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_logger/helpers/logger_helper.hpp"

namespace act::logger
{

class SubLoggerHelper : public LoggerHelper
{
  public:
    /**
     * @brief Class constructor
     * @param category The additional category for this sub-logger
     * @param parentLogger The parent logger
     * @param minLevel The minimum logs level for this sub-logger
     */
    explicit SubLoggerHelper(
        const std::string &category,
        LoggerHelper &parentLogger,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /**
     * @brief Class constructor
     * @param parentLogger The parent logger
     * @param minLevel The minimum logs level for this sub-logger
     */
    explicit SubLoggerHelper(
        LoggerHelper &parentLogger,
        act::foundation::LogsLevel::Enum minLevel = act::foundation::LogsLevel::Enum::TRACE);

    /** @brief Class destructor */
    ~SubLoggerHelper() override = default;

  protected:
    /**
     * @brief Constructor for subclasses that supply their own category list but
     *        still want to delegate getLogger() to a parent.
     * @param categories Pre-built category list.
     * @param parentLogger The parent logger to delegate getLogger() to.
     * @param minLevel Minimum log level.
     */
    explicit SubLoggerHelper(const std::vector<std::string> &categories,
                             LoggerHelper &parentLogger,
                             act::foundation::LogsLevel::Enum minLevel);

  public:
    /**
     * @brief Get the external logger
     * @return The external logger
     */
    [[nodiscard]] const std::shared_ptr<AbsExternalLogger> &getLogger() const override
    {
        return m_parentLogger.getLogger();
    }

    /**
     * @brief Update the external logger
     * @param externalLogger The new external logger
     */
    void updateLogger(const std::shared_ptr<AbsExternalLogger> &externalLogger) override
    {
        m_parentLogger.updateLogger(externalLogger);
    }

  private:
    /**
     * @brief Concatenate the categories of the parent logger with the new category
     * @param category The new category
     * @param parentLogger The parent logger
     * @return The concatenated categories
     */
    static std::vector<std::string> ConcatenateCategories(const std::string &category,
                                                          const LoggerHelper &parentLogger);

  private:
    /** @brief The parent logger */
    LoggerHelper &m_parentLogger;
};

} // namespace act::logger
