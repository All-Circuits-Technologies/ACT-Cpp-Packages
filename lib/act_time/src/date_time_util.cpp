// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/date_time_util.hpp"

#include "act_foundation/constants/def_soft.hpp"

#include <ctime>

namespace act::time
{

std::string DateTimeUtil::GetCurrentIsoDateTimeUtc()
{
    return GetCurrentIsoDateTime(std::gmtime, ISO_UTC_TIME_PATTERN);
}

std::string DateTimeUtil::GetCurrentIsoDateTimeLocal()
{
    std::string dateTime = GetCurrentIsoDateTime(std::localtime, ISO_LOCAL_TIME_PATTERN);
    if (dateTime.size() > UTC_OFFSET_MINUTES_LENGTH)
    {
        dateTime.insert(dateTime.size() - UTC_OFFSET_MINUTES_LENGTH, ":");
    }
    return dateTime;
}

std::string DateTimeUtil::GetCurrentIsoDateTime(
    const std::function<tm *(const time_t *)> &timeConverter, const char *pattern)
{
    const auto now = std::time(nullptr);
    char buffer[ISO_TIME_PATTERN_BUFFER_SIZE] = {0};

    // No need to test the return value, we assume the buffer is large enough
    UNUSED(std::strftime(buffer, ISO_TIME_PATTERN_BUFFER_SIZE, pattern, timeConverter(&now)));

    return {buffer};
}

} // namespace act::time
