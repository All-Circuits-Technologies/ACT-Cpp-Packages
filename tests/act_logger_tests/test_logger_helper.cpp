// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_logger_helper.cpp
 * @brief Unit tests for LoggerHelper: its own minimum level, the category attached to every
 *        message, the combination with the external logger filters in wouldBeLogged, the level
 *        helpers and streams, the replacement of the external logger and the sub-loggers it
 *        creates.
 */

#include "act_logger/helpers/logger_helper.hpp"

#include "capturing_external_logger.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    class LoggerHelperTest : public ::testing::Test
    {
      protected:
        [[nodiscard]] const std::shared_ptr<test::CapturingExternalLogger> &getExternal() const
        {
            return m_external;
        }

      private:
        std::shared_ptr<test::CapturingExternalLogger> m_external{
            std::make_shared<test::CapturingExternalLogger>()};
    };

    TEST_F(LoggerHelperTest, CategoryIsAttachedToEveryMessage)
    {
        const LoggerHelper logger(getExternal(), "main");

        EXPECT_EQ(logger.getCategories(), std::vector<std::string>{"main"});
        logger.info("message");

        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].categories, std::vector<std::string>{"main"});
    }

    TEST_F(LoggerHelperTest, LoggerWithoutCategorySendsNoCategory)
    {
        const LoggerHelper logger(getExternal());

        logger.info("message");

        EXPECT_TRUE(logger.getCategories().empty());
        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_TRUE(getExternal()->getEntries()[0].categories.empty());
    }

    TEST_F(LoggerHelperTest, MessagesBelowTheHelperMinimumAreDropped)
    {
        const LoggerHelper logger(getExternal(), "main", act::foundation::LogsLevel::Enum::WARNING);

        logger.info("dropped");
        logger.warning("kept");

        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].message, "kept");
    }

    TEST_F(LoggerHelperTest, WouldBeLoggedCombinesHelperAndExternalFilters)
    {
        getExternal()->setMinLevel(act::foundation::LogsLevel::Enum::INFO);
        const LoggerHelper logger(getExternal(), "main", act::foundation::LogsLevel::Enum::DEBG);

        EXPECT_FALSE(
            logger.wouldBeLogged(act::foundation::LogsLevel::Enum::TRACE)); // Helper filter
        EXPECT_FALSE(
            logger.wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG)); // External filter
        EXPECT_TRUE(logger.wouldBeLogged(act::foundation::LogsLevel::Enum::INFO));

        // The external logger sees the helper categories
        getExternal()->setCategoryMinLevel("main", act::foundation::LogsLevel::Enum::DEBG);
        EXPECT_TRUE(logger.wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG));
    }

    TEST_F(LoggerHelperTest, LevelHelpersAndStreamsLogWithTheirLevel)
    {
        const LoggerHelper logger(getExternal());

        logger.trace("t");
        logger.debug("d");
        logger.info("i");
        logger.warning("w");
        logger.error("e");
        logger.fatal("f");
        logger.traceStream() << "ts";
        logger.debugStream() << "ds";
        logger.infoStream() << "is";
        logger.warningStream() << "ws";
        logger.errorStream() << "es";
        logger.fatalStream() << "fs";
        logger.logStream(act::foundation::LogsLevel::Enum::WARNING) << "ls";

        const std::vector<act::foundation::LogsLevel::Enum> levels{
            act::foundation::LogsLevel::Enum::TRACE,
            act::foundation::LogsLevel::Enum::DEBG,
            act::foundation::LogsLevel::Enum::INFO,
            act::foundation::LogsLevel::Enum::WARNING,
            act::foundation::LogsLevel::Enum::ERR,
            act::foundation::LogsLevel::Enum::FATAL};
        const auto &entries = getExternal()->getEntries();
        ASSERT_EQ(entries.size(), (2 * levels.size()) + 1);
        for (std::size_t idx = 0; idx < levels.size(); ++idx)
        {
            EXPECT_EQ(entries[idx].level, levels[idx]);
            EXPECT_EQ(entries[idx + levels.size()].level, levels[idx]);
        }
        EXPECT_EQ(entries[0].message, "t");
        EXPECT_EQ(entries[levels.size()].message, "ts");
        EXPECT_EQ(entries.back().level, act::foundation::LogsLevel::Enum::WARNING);
        EXPECT_EQ(entries.back().message, "ls");
    }

    TEST_F(LoggerHelperTest, UpdateLoggerRedirectsTheNextMessages)
    {
        LoggerHelper logger(getExternal(), "main");
        auto other = std::make_shared<test::CapturingExternalLogger>();

        logger.info("before");
        logger.updateLogger(other);
        logger.info("after");

        EXPECT_EQ(logger.getLogger(), other);
        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].message, "before");
        ASSERT_EQ(other->getEntries().size(), 1U);
        EXPECT_EQ(other->getEntries()[0].message, "after");
    }

    TEST_F(LoggerHelperTest, MessagesWithoutExternalLoggerAreDropped)
    {
        LoggerHelper logger(getExternal());
        logger.updateLogger(nullptr);

        logger.error("lost");

        EXPECT_EQ(logger.getLogger(), nullptr);
        EXPECT_TRUE(getExternal()->getEntries().empty());
    }

    TEST_F(LoggerHelperTest, NothingWouldBeLoggedWithoutExternalLogger)
    {
        LoggerHelper logger(getExternal());
        logger.updateLogger(nullptr);

        EXPECT_FALSE(logger.wouldBeLogged(act::foundation::LogsLevel::Enum::FATAL));
    }

    TEST_F(LoggerHelperTest, SubLoggerAppendsItsCategoryAndSharesTheExternalLogger)
    {
        LoggerHelper logger(getExternal(), "main");

        const auto sub = logger.createSubLogger("sub");
        sub->info("message");

        EXPECT_EQ(sub->getCategories(), (std::vector<std::string>{"main", "sub"}));
        EXPECT_EQ(sub->getLogger(), getExternal());
        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].categories,
                  (std::vector<std::string>{"main", "sub"}));
    }

    TEST_F(LoggerHelperTest, SubLoggerWithoutCategoryKeepsTheParentCategories)
    {
        LoggerHelper logger(getExternal(), "main");

        const auto sub = logger.createSubLogger(act::foundation::LogsLevel::Enum::ERR);
        sub->warning("dropped");
        sub->error("kept");

        EXPECT_EQ(sub->getCategories(), std::vector<std::string>{"main"});
        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].message, "kept");
    }

    TEST_F(LoggerHelperTest, AbsSubLoggerBehavesLikeASubLogger)
    {
        LoggerHelper logger(getExternal(), "main");

        const std::shared_ptr<act::foundation::AbsLogger> sub =
            logger.createAbsSubLogger("sub", act::foundation::LogsLevel::Enum::INFO);
        const std::shared_ptr<act::foundation::AbsLogger> same =
            logger.createAbsSubLogger(act::foundation::LogsLevel::Enum::INFO);
        sub->debug("dropped");
        sub->info("first");
        same->info("second");

        const auto &entries = getExternal()->getEntries();
        ASSERT_EQ(entries.size(), 2U);
        EXPECT_EQ(entries[0].categories, (std::vector<std::string>{"main", "sub"}));
        EXPECT_EQ(entries[1].categories, std::vector<std::string>{"main"});
    }

} // namespace
} // namespace act::logger
