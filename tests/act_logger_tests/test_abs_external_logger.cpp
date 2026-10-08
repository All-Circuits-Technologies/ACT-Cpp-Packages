// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_external_logger.cpp
 * @brief Unit tests for AbsExternalLogger: the global minimum level, a per category minimum level
 *        lowering it for the messages of that category, the level helpers and the dispatch to
 *        logToExternal of the messages which pass the filters only.
 */

#include "act_logger/printers/abs_external_logger.hpp"

#include "capturing_external_logger.hpp"

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    TEST(AbsExternalLoggerTest, LevelsBelowTheMinimumAreNotLoggable)
    {
        const test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::WARNING);

        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::TRACE, {}));
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {}));
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::WARNING, {}));
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::FATAL, {"any"}));
    }

    TEST(AbsExternalLoggerTest, SetMinLevelChangesTheFilter)
    {
        test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::ERR);
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {}));

        logger.setMinLevel(act::foundation::LogsLevel::Enum::DEBG);

        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::TRACE, {}));
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::DEBG, {}));
    }

    TEST(AbsExternalLoggerTest, CategoryMinimumLowersTheFilterForThatCategory)
    {
        const test::CapturingExternalLogger logger(
            act::foundation::LogsLevel::Enum::ERR,
            {{"verbose", act::foundation::LogsLevel::Enum::DEBG}});

        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::DEBG, {"verbose"}));
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::TRACE, {"verbose"}));
        // Other categories keep the global minimum
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::DEBG, {"other"}));
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::DEBG, {}));
    }

    TEST(AbsExternalLoggerTest, CategoryMinimumRaisesTheFilterForThatCategory)
    {
        const test::CapturingExternalLogger logger(
            act::foundation::LogsLevel::Enum::TRACE,
            {{"quiet", act::foundation::LogsLevel::Enum::ERR}});

        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"quiet"}));
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::ERR, {"quiet"}));
        // Other categories keep the global minimum
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"other"}));
    }

    TEST(AbsExternalLoggerTest, HighestMatchingCategoryMinimumApplies)
    {
        const test::CapturingExternalLogger logger(
            act::foundation::LogsLevel::Enum::TRACE,
            {{"verbose", act::foundation::LogsLevel::Enum::DEBG},
             {"quiet", act::foundation::LogsLevel::Enum::ERR}});

        EXPECT_FALSE(
            logger.isLoggable(act::foundation::LogsLevel::Enum::WARNING, {"verbose", "quiet"}));
        EXPECT_FALSE(
            logger.isLoggable(act::foundation::LogsLevel::Enum::WARNING, {"quiet", "verbose"}));
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::ERR, {"verbose", "quiet"}));
    }

    TEST(AbsExternalLoggerTest, AnyCategoryOfTheMessageCanMatch)
    {
        const test::CapturingExternalLogger logger(
            act::foundation::LogsLevel::Enum::ERR,
            {{"verbose", act::foundation::LogsLevel::Enum::INFO}});

        EXPECT_TRUE(
            logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"parent", "verbose"}));
        EXPECT_TRUE(
            logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"verbose", "child"}));
    }

    TEST(AbsExternalLoggerTest, SetCategoryMinLevelAddsAndOverridesACategory)
    {
        test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::ERR);
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"cat"}));

        logger.setCategoryMinLevel("cat", act::foundation::LogsLevel::Enum::INFO);
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::INFO, {"cat"}));
        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::DEBG, {"cat"}));

        logger.setCategoryMinLevel("cat", act::foundation::LogsLevel::Enum::TRACE);
        EXPECT_TRUE(logger.isLoggable(act::foundation::LogsLevel::Enum::TRACE, {"cat"}));
    }

    TEST(AbsExternalLoggerTest, LogForwardsOnlyLoggableMessages)
    {
        test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::INFO);

        logger.log(act::foundation::LogsLevel::Enum::DEBG, "dropped", {"cat"});
        logger.log(act::foundation::LogsLevel::Enum::WARNING, "kept", {"cat", "sub"});

        ASSERT_EQ(logger.getEntries().size(), 1U);
        EXPECT_EQ(logger.getEntries()[0].level, act::foundation::LogsLevel::Enum::WARNING);
        EXPECT_EQ(logger.getEntries()[0].message, "kept");
        EXPECT_EQ(logger.getEntries()[0].categories, (std::vector<std::string>{"cat", "sub"}));
    }

    TEST(AbsExternalLoggerTest, LevelHelpersLogWithTheirLevel)
    {
        test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::TRACE);

        logger.trace("t");
        logger.debug("d");
        logger.info("i");
        logger.warning("w");
        logger.error("e", {"cat"});
        logger.fatal("f");

        const std::vector<act::foundation::LogsLevel::Enum> expected{
            act::foundation::LogsLevel::Enum::TRACE,
            act::foundation::LogsLevel::Enum::DEBG,
            act::foundation::LogsLevel::Enum::INFO,
            act::foundation::LogsLevel::Enum::WARNING,
            act::foundation::LogsLevel::Enum::ERR,
            act::foundation::LogsLevel::Enum::FATAL};
        ASSERT_EQ(logger.getEntries().size(), expected.size());
        for (std::size_t idx = 0; idx < expected.size(); ++idx)
        {
            EXPECT_EQ(logger.getEntries()[idx].level, expected[idx]);
        }
        EXPECT_EQ(logger.getEntries()[4].categories, std::vector<std::string>{"cat"});
        EXPECT_TRUE(logger.getEntries()[5].categories.empty());
    }

    TEST(AbsExternalLoggerTest, NoneMinimumFiltersEveryMessageLevel)
    {
        const test::CapturingExternalLogger logger(act::foundation::LogsLevel::Enum::NONE);

        EXPECT_FALSE(logger.isLoggable(act::foundation::LogsLevel::Enum::FATAL, {}));
    }

} // namespace
} // namespace act::logger
