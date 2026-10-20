// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_sub_logger_helper.cpp
 * @brief Unit tests for SubLoggerHelper: the categories it inherits from its parent, its own
 *        minimum level, and the external logger which stays the parent's one, read at each call
 *        and replaced through the parent.
 */

#include "act_logger/helpers/sub_logger_helper.hpp"

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

    class SubLoggerHelperTest : public ::testing::Test
    {
      protected:
        [[nodiscard]] const std::shared_ptr<test::CapturingExternalLogger> &getExternal() const
        {
            return m_external;
        }

        [[nodiscard]] LoggerHelper &getParent()
        {
            return m_parent;
        }

      private:
        std::shared_ptr<test::CapturingExternalLogger> m_external{
            std::make_shared<test::CapturingExternalLogger>()};
        LoggerHelper m_parent{m_external, "parent"};
    };

    TEST_F(SubLoggerHelperTest, CategoryIsAppendedToTheParentCategories)
    {
        SubLoggerHelper sub("child", getParent());
        const SubLoggerHelper grandChild("leaf", sub);

        EXPECT_EQ(sub.getCategories(), (std::vector<std::string>{"parent", "child"}));
        EXPECT_EQ(grandChild.getCategories(),
                  (std::vector<std::string>{"parent", "child", "leaf"}));
    }

    TEST_F(SubLoggerHelperTest, SubLoggerWithoutCategoryCopiesTheParentCategories)
    {
        const SubLoggerHelper sub(getParent());

        EXPECT_EQ(sub.getCategories(), std::vector<std::string>{"parent"});
    }

    TEST_F(SubLoggerHelperTest, MinimumLevelIsTheSubLoggerOne)
    {
        LoggerHelper strictParent(getExternal(), "parent", act::foundation::LogsLevel::Enum::ERR);
        const SubLoggerHelper sub("child", strictParent, act::foundation::LogsLevel::Enum::DEBG);

        sub.trace("dropped");
        sub.debug("kept");

        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].message, "kept");
        EXPECT_EQ(getExternal()->getEntries()[0].categories,
                  (std::vector<std::string>{"parent", "child"}));
    }

    TEST_F(SubLoggerHelperTest, ExternalLoggerIsFollowedWhenTheParentChangesIt)
    {
        const SubLoggerHelper sub("child", getParent());
        auto other = std::make_shared<test::CapturingExternalLogger>();

        EXPECT_EQ(sub.getLogger(), getExternal());
        getParent().updateLogger(other);
        sub.info("message");

        EXPECT_EQ(sub.getLogger(), other);
        EXPECT_TRUE(getExternal()->getEntries().empty());
        EXPECT_EQ(other->getEntries().size(), 1U);
    }

    TEST_F(SubLoggerHelperTest, UpdateLoggerReplacesTheParentLogger)
    {
        SubLoggerHelper sub("child", getParent());
        auto other = std::make_shared<test::CapturingExternalLogger>();

        sub.updateLogger(other);
        getParent().info("from parent");

        EXPECT_EQ(getParent().getLogger(), other);
        ASSERT_EQ(other->getEntries().size(), 1U);
        EXPECT_EQ(other->getEntries()[0].message, "from parent");
    }

    TEST_F(SubLoggerHelperTest, WouldBeLoggedUsesTheParentExternalLoggerFilters)
    {
        getExternal()->setMinLevel(act::foundation::LogsLevel::Enum::WARNING);
        getExternal()->setCategoryMinLevel("child", act::foundation::LogsLevel::Enum::DEBG);
        const SubLoggerHelper sub("child", getParent());

        EXPECT_TRUE(sub.wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG));
        EXPECT_FALSE(getParent().wouldBeLogged(act::foundation::LogsLevel::Enum::DEBG));
    }

} // namespace
} // namespace act::logger
