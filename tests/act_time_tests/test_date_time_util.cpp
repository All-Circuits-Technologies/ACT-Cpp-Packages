// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_date_time_util.cpp
 * @brief Unit tests for act::time::DateTimeUtil.
 *
 * Covers the ISO 8601 shape of the returned date and time, and that it is the current one: the
 * UTC string is parsed back and must lie between two clock readings taken around the call, and
 * the local string must match the local time of one of those readings.
 */

#include "act_time/date_time_util.hpp"

#include "act_foundation/constants/def_soft.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <regex>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace act::time
{
namespace
{

    /** @brief Length of the YYYY-MM-DDTHH:MM:SS part */
    constexpr std::size_t ISO_DATE_TIME_LENGTH = 19;

    /** @brief Size of the buffer receiving a formatted date and time */
    constexpr std::size_t FORMAT_BUFFER_SIZE = 32;

    /** @brief Format @p time with @p converter as YYYY-MM-DDTHH:MM:SS */
    std::string FormatSeconds(std::time_t time, tm *(*converter)(const std::time_t *, tm *))
    {
        tm parts{};
        if (converter(&time, &parts) == nullptr)
        {
            return {};
        }

        std::array<char, FORMAT_BUFFER_SIZE> buffer{};
        if (std::strftime(buffer.data(), buffer.size(), "%FT%T", &parts) == 0)
        {
            return {};
        }
        return {buffer.data()};
    }

    TEST(DateTimeUtilTest, UtcIsAnIsoDateTimeEndingWithZ)
    {
        const std::string now = DateTimeUtil::GetCurrentIsoDateTimeUtc();

        EXPECT_TRUE(std::regex_match(now, std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)")))
            << now;
    }

    TEST(DateTimeUtilTest, UtcIsTheCurrentTime)
    {
        const std::time_t before = std::time(nullptr);
        const std::string now = DateTimeUtil::GetCurrentIsoDateTimeUtc();
        const std::time_t after = std::time(nullptr);

        const std::string seconds = now.substr(0, ISO_DATE_TIME_LENGTH);
        EXPECT_TRUE(seconds == FormatSeconds(before, gmtime_r) ||
                    seconds == FormatSeconds(after, gmtime_r))
            << now;
    }

    TEST(DateTimeUtilTest, LocalIsAnIsoDateTimeEndingWithItsUtcOffset)
    {
        const std::string now = DateTimeUtil::GetCurrentIsoDateTimeLocal();

        EXPECT_TRUE(
            std::regex_match(now,
                             std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{2}:\d{2})")))
            << now;
    }

    /** @brief Runs a test in a given time zone, and restores the previous one afterwards */
    class DateTimeUtilTimeZoneTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            const char *timeZone = std::getenv("TZ");
            if (timeZone != nullptr)
            {
                m_previousTimeZone = timeZone;
            }
        }

        void TearDown() override
        {
            if (m_previousTimeZone.has_value())
            {
                ::setenv("TZ", m_previousTimeZone->c_str(), 1);
            }
            else
            {
                ::unsetenv("TZ");
            }
            ::tzset();
        }

        /** @brief Switch the process to the POSIX time zone @p timeZone */
        static void UseTimeZone(const char *timeZone)
        {
            ::setenv("TZ", timeZone, 1);
            ::tzset();
        }

      private:
        std::optional<std::string> m_previousTimeZone;
    };

    TEST_F(DateTimeUtilTimeZoneTest, LocalEndsWithTheOffsetOfTheTimeZone)
    {
        // POSIX time zones count the offset westwards: "ACT-02" is two hours ahead of UTC
        UseTimeZone("ACT-02");
        EXPECT_TRUE(DateTimeUtil::GetCurrentIsoDateTimeLocal().ends_with("+02:00"));

        UseTimeZone("ACT+05:30");
        EXPECT_TRUE(DateTimeUtil::GetCurrentIsoDateTimeLocal().ends_with("-05:30"));

        UseTimeZone("UTC0");
        EXPECT_TRUE(DateTimeUtil::GetCurrentIsoDateTimeLocal().ends_with("+00:00"));
    }

    TEST(DateTimeUtilTest, LocalIsTheCurrentLocalTime)
    {
        const std::time_t before = std::time(nullptr);
        const std::string now = DateTimeUtil::GetCurrentIsoDateTimeLocal();
        const std::time_t after = std::time(nullptr);

        const std::string seconds = now.substr(0, ISO_DATE_TIME_LENGTH);
        EXPECT_TRUE(seconds == FormatSeconds(before, localtime_r) ||
                    seconds == FormatSeconds(after, localtime_r))
            << now;
    }

    TEST_F(DateTimeUtilTimeZoneTest, UtcAndLocalCanBeAskedFromConcurrentThreads)
    {
        // Far enough from UTC for a local time read in place of the UTC one to be noticed
        UseTimeZone("ACT-05");
        constexpr int callCount = 20000;
        std::atomic<bool> done{false};
        std::thread localReader([&done]() {
            while (!done.load())
            {
                UNUSED(DateTimeUtil::GetCurrentIsoDateTimeLocal());
            }
        });

        int wrongUtcCount = 0;
        for (int index = 0; index < callCount; ++index)
        {
            const std::time_t before = std::time(nullptr);
            const std::string seconds =
                DateTimeUtil::GetCurrentIsoDateTimeUtc().substr(0, ISO_DATE_TIME_LENGTH);
            const std::time_t after = std::time(nullptr);
            if (seconds != FormatSeconds(before, gmtime_r) &&
                seconds != FormatSeconds(after, gmtime_r))
            {
                ++wrongUtcCount;
            }
        }
        done.store(true);
        localReader.join();

        EXPECT_EQ(wrongUtcCount, 0);
    }

} // namespace
} // namespace act::time
