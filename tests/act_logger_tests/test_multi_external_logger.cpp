// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_multi_external_logger.cpp
 * @brief Unit tests for MultiExternalLogger: every message is forwarded to each wrapped logger,
 *        which applies its own filters, and null entries are skipped.
 */

#include "act_logger/helpers/multi_external_logger.hpp"

#include "capturing_external_logger.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    TEST(MultiExternalLoggerTest, MessageIsForwardedToEveryLogger)
    {
        auto first = std::make_shared<test::CapturingExternalLogger>();
        auto second = std::make_shared<test::CapturingExternalLogger>();
        MultiExternalLogger multi({first, second});

        multi.log(act::foundation::LogsLevel::Enum::INFO, "message", {"cat"});

        for (const auto &logger : {first, second})
        {
            ASSERT_EQ(logger->getEntries().size(), 1U);
            EXPECT_EQ(logger->getEntries()[0].level, act::foundation::LogsLevel::Enum::INFO);
            EXPECT_EQ(logger->getEntries()[0].message, "message");
            EXPECT_EQ(logger->getEntries()[0].categories, std::vector<std::string>{"cat"});
        }
    }

    TEST(MultiExternalLoggerTest, EachLoggerAppliesItsOwnFilters)
    {
        auto verbose = std::make_shared<test::CapturingExternalLogger>(
            act::foundation::LogsLevel::Enum::TRACE);
        auto quiet =
            std::make_shared<test::CapturingExternalLogger>(act::foundation::LogsLevel::Enum::ERR);
        MultiExternalLogger multi({verbose, quiet});

        multi.debug("debug");
        multi.error("error");

        EXPECT_EQ(verbose->getEntries().size(), 2U);
        ASSERT_EQ(quiet->getEntries().size(), 1U);
        EXPECT_EQ(quiet->getEntries()[0].message, "error");
    }

    TEST(MultiExternalLoggerTest, NullLoggersAreSkipped)
    {
        auto logger = std::make_shared<test::CapturingExternalLogger>();
        MultiExternalLogger multi({nullptr, logger, nullptr});

        multi.warning("message");

        EXPECT_EQ(logger->getEntries().size(), 1U);
    }

    TEST(MultiExternalLoggerTest, EmptyListAcceptsMessages)
    {
        MultiExternalLogger multi({});

        EXPECT_TRUE(multi.isLoggable(act::foundation::LogsLevel::Enum::TRACE, {}));
        multi.fatal("nobody listens");
    }

} // namespace
} // namespace act::logger
