// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <functional>
#include <string>

class tm;

namespace act::time
{

/** @brief Contains the date and time utility functions */
class DateTimeUtil
{
  public:
    /**
     * @brief Get the current date and time in ISO 8601 format (UTC).
     * @return A string representing the current date and time in ISO 8601 format.
     */
    static std::string GetCurrentIsoDateTimeUtc();

    /**
     * @brief Get the current date and time in ISO 8601 format (Local Time).
     * @return A string representing the current date and time in ISO 8601 format, ending with
     *         the offset of the local time from UTC (for instance 2026-10-07T11:51:03+02:00).
     */
    static std::string GetCurrentIsoDateTimeLocal();

  private:
    /**
     * @brief Get the current date and time in ISO 8601 format using the specified time
     * converter.
     * @param timeConverter A function that converts a time_t pointer to a tm pointer.
     * @param pattern The strftime pattern to format the date and time with.
     * @return A string representing the current date and time in ISO 8601 format.
     */
    static std::string GetCurrentIsoDateTime(
        const std::function<tm *(const time_t *)> &timeConverter, const char *pattern);

  private:
    /** @brief ISO time pattern for UTC date-time formatting, with the UTC designator */
    static const constexpr char *ISO_UTC_TIME_PATTERN = "%FT%TZ";

    /**
     * @brief ISO time pattern for local date-time formatting, with the offset from UTC
     * @note strftime writes the offset as +hhmm, the colon of +hh:mm is inserted afterwards
     */
    static const constexpr char *ISO_LOCAL_TIME_PATTERN = "%FT%T%z";

    /** @brief Number of digits of the minutes of a UTC offset */
    static const constexpr size_t UTC_OFFSET_MINUTES_LENGTH = 2;

    /** @brief Buffer size for ISO time pattern */
    static const constexpr size_t ISO_TIME_PATTERN_BUFFER_SIZE = 32;
};

} // namespace act::time
