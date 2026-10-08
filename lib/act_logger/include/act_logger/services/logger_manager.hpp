// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_logger/services/abs_logger_manager.hpp"

#include <memory>
#include <string>

namespace act::logger
{

class StdConsoleLogger;

/** @brief The LoggerManager class handles logging operations */
class LoggerManager : public AbsLoggerManager
{
  public:
    /**
     * @brief Constructor
     */
    explicit LoggerManager(act::foundation::LogsLevel::Enum minLevelToPrintToStdErr =
                               act::foundation::LogsLevel::Enum::ERR);

    /**
     * @brief Destructor
     */
    ~LoggerManager() override = default;

  public:
    /**
     * @brief Initialize the manager
     * @return true if initialization was successful, false otherwise
     */
    bool init() override;

    /**
     * @brief Set the console logger minimum log level
     * @param minLevel The new minimum log level for the console logger
     */
    void setCslMinLogLevel(act::foundation::LogsLevel::Enum minLevel);

    /**
     * @brief Set the console logger minimum log level for a specific category.
     * @param category The category name
     * @param minLevel The minimum log level for this category
     */
    void setCslCategoryMinLogLevel(const std::string &category,
                                   act::foundation::LogsLevel::Enum minLevel);

  protected:
    /**
     * @brief Get the external logger
     * @return The external logger
     */
    [[nodiscard]] std::shared_ptr<AbsExternalLogger> getExternalLogger() const override;

  private:
    /** @brief The console logger */
    std::shared_ptr<StdConsoleLogger> m_consoleLogger;

    /** @brief The minimum logs level to print to standard error (stderr) */
    act::foundation::LogsLevel::Enum m_minLevelToPrintToStdErr;
};

} // namespace act::logger
