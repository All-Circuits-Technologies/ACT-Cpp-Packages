// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_logger/printers/abs_external_logger.hpp"

namespace act::logger
{

/** @brief This is a simple external logger that logs to the standard console */
class StdConsoleLogger : public AbsExternalLogger
{
  public:
    /**
     * @brief Class constructor
     * @param minLevel The minimum logs level to log
     * @param minLevelToPrintToStdErr The minimum logs level to print to standard error
     * (stderr)
     * @param minLevelByCategory This map defines the minimum logs level to log for each
     * category. This overrides the minLevel parameter for the specified categories. If a
     * category is not in this map, the minLevel parameter is used. If empty, only the
     * minLevel parameter is used. If a log message has multiple categories, the maximum
     * matching level in the map is used.
     */
    explicit StdConsoleLogger(
        act::foundation::LogsLevel::Enum minLevel,
        act::foundation::LogsLevel::Enum minLevelToPrintToStdErr =
            act::foundation::LogsLevel::Enum::ERR,
        const std::map<std::string, act::foundation::LogsLevel::Enum> &minLevelByCategory = {});

    /** @brief Class destructor */
    ~StdConsoleLogger() override = default;

  protected:
    /**
     * @brief Log a message
     * @param level The logs level
     * @param message The message to log
     * @param categories The categories associated to the log message
     */
    void logToExternal(act::foundation::LogsLevel::Enum level,
                       const std::string &message,
                       const std::vector<std::string> &categories) override;

  private:
    /** @brief Format the log message */
    [[nodiscard]] static std::string FormatLogMessage(act::foundation::LogsLevel::Enum level,
                                                      const std::string &message,
                                                      const std::vector<std::string> &categories);

  private:
    /** @brief Separator used between categories */
    static constexpr const char *CATEGORIES_SEPARATOR = "/";

  private:
    /** @brief The minimum logs level to print to standard error (stderr) */
    act::foundation::LogsLevel::Enum m_minLevelToPrintToStdErr;
};

} // namespace act::logger
