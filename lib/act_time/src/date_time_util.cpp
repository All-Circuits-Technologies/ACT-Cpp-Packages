// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/date_time_util.hpp"

#include <array>
#include <ctime>
#include <tuple>

namespace act::time
{

namespace
{

    /**
     * @brief Break @p time down into its calendar parts, as UTC or as local time
     * @note std::gmtime and std::localtime share a static result, so concurrent calls overwrite
     * each other's; the reentrant variants of each platform write into @p parts instead.
     * @param time The time to break down
     * @param utc True to break it down as UTC, false as local time
     * @param parts Receives the calendar parts
     * @return True on success, false if the time cannot be converted
     */
    bool BreakDownTime(std::time_t time, bool utc, std::tm &parts)
    {
#ifdef _WIN32
        return (utc ? gmtime_s(&parts, &time) : localtime_s(&parts, &time)) == 0;
#else
        return (utc ? gmtime_r(&time, &parts) : localtime_r(&time, &parts)) != nullptr;
#endif
    }

} // namespace

std::string DateTimeUtil::GetCurrentIsoDateTimeUtc()
{
    return GetCurrentIsoDateTime(true, ISO_UTC_TIME_PATTERN);
}

std::string DateTimeUtil::GetCurrentIsoDateTimeLocal()
{
    std::string dateTime = GetCurrentIsoDateTime(false, ISO_LOCAL_TIME_PATTERN);
    if (dateTime.size() > UTC_OFFSET_MINUTES_LENGTH)
    {
        dateTime.insert(dateTime.size() - UTC_OFFSET_MINUTES_LENGTH, ":");
    }
    return dateTime;
}

std::string DateTimeUtil::GetCurrentIsoDateTime(bool utc, const char *pattern)
{
    const auto now = std::time(nullptr);
    std::tm parts{};
    if (!BreakDownTime(now, utc, parts))
    {
        return {};
    }

    std::array<char, ISO_TIME_PATTERN_BUFFER_SIZE> buffer{};

    // No need to test the return value, we assume the buffer is large enough
    std::ignore = std::strftime(buffer.data(), buffer.size(), pattern, &parts);

    return {buffer.data()};
}

} // namespace act::time
