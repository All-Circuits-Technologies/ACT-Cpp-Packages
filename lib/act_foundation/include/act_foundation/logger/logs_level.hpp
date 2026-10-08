// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <string>

namespace act::foundation
{

/** @brief This is the status of the Firmware update */
class LogsLevel : private NotCopiableNotMovable
{
  public:
    /** @brief Enum representing the logs level types */
    enum Enum : int
    {
        TRACE = 0,
        // DEBUG is keyword already used in some environments, so we use DEBG instead
        DEBG,
        INFO,
        WARNING,
        // ERROR is a macro defined in some environments (e.g. wingdi.h), so we use ERR instead
        ERR,
        FATAL,
        NONE
    };

  public:
    /**
     * @brief Convert LogsLevel::Enum to string
     * @param level The logs level
     * @return The string representation of the logs level
     */
    static std::string ToString(LogsLevel::Enum level)
    {
        switch (level)
        {
            case LogsLevel::Enum::TRACE:
                return TRACE_STR;
            case LogsLevel::Enum::DEBG:
                return DEBUG_STR;
            case LogsLevel::Enum::INFO:
                return INFO_STR;
            case LogsLevel::Enum::WARNING:
                return WARNING_STR;
            case LogsLevel::Enum::ERR:
                return ERROR_STR;
            case LogsLevel::Enum::FATAL:
                return FATAL_STR;
            case LogsLevel::Enum::NONE:
                return NONE_STR;
        }

        // Should not happen
        return "";
    }

    /**
     * @brief Convert integer to LogsLevel::Enum
     * @param level The integer logs level
     * @param defaultLevel The default logs level to return if the input level is invalid
     * @return The LogsLevel::Enum representation of the logs level
     */
    static LogsLevel::Enum FromLevel(int level,
                                     LogsLevel::Enum defaultLevel = LogsLevel::Enum::NONE)
    {
        if (level < LogsLevel::Enum::TRACE || level > LogsLevel::Enum::NONE)
        {
            return defaultLevel;
        }

        return static_cast<LogsLevel::Enum>(level);
    }

  private:
    /** @brief String representation of the trace level */
    static constexpr const char *TRACE_STR = "TRACE";

    /** @brief String representation of the debug level */
    static constexpr const char *DEBUG_STR = "DEBUG";

    /** @brief String representation of the info level */
    static constexpr const char *INFO_STR = "INFO";

    /** @brief String representation of the warning level */
    static constexpr const char *WARNING_STR = "WARN";

    /** @brief String representation of the error level */
    static constexpr const char *ERROR_STR = "ERROR";

    /** @brief String representation of the fatal level */
    static constexpr const char *FATAL_STR = "FATAL";

    /** @brief String representation of the none level */
    static constexpr const char *NONE_STR = "NONE";
};

} // namespace act::foundation
