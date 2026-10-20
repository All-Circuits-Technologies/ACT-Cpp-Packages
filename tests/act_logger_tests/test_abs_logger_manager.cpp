// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_logger_manager.cpp
 * @brief Unit tests for AbsLoggerManager: after init, every logging call and every sub-logger
 *        reaches the external logger given by the derived class, without category for the
 *        manager itself.
 */

#include "act_logger/services/abs_logger_manager.hpp"

#include "capturing_external_logger.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace act::logger
{
namespace
{

    /** @brief Manager which logs to a capturing external logger */
    class CapturingLoggerManager : public AbsLoggerManager
    {
      public:
        explicit CapturingLoggerManager(std::shared_ptr<test::CapturingExternalLogger> external)
            : m_external{std::move(external)}
        {
        }

        ~CapturingLoggerManager() override = default;

      protected:
        [[nodiscard]] std::shared_ptr<AbsExternalLogger> getExternalLogger() const override
        {
            return m_external;
        }

      private:
        std::shared_ptr<test::CapturingExternalLogger> m_external;
    };

    class AbsLoggerManagerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_manager.init());
        }

        [[nodiscard]] const std::shared_ptr<test::CapturingExternalLogger> &getExternal() const
        {
            return m_external;
        }

        [[nodiscard]] CapturingLoggerManager &getManager()
        {
            return m_manager;
        }

      private:
        std::shared_ptr<test::CapturingExternalLogger> m_external{
            std::make_shared<test::CapturingExternalLogger>()};
        CapturingLoggerManager m_manager{m_external};
    };

    TEST_F(AbsLoggerManagerTest, InitLogsThatTheManagerIsReady)
    {
        ASSERT_EQ(getExternal()->getEntries().size(), 1U);
        EXPECT_EQ(getExternal()->getEntries()[0].level, act::foundation::LogsLevel::Enum::INFO);
        EXPECT_TRUE(getExternal()->getEntries()[0].categories.empty());
    }

    TEST_F(AbsLoggerManagerTest, LoggingCallsReachTheExternalLogger)
    {
        getManager().trace("t");
        getManager().debug("d");
        getManager().info("i");
        getManager().warning("w");
        getManager().error("e");
        getManager().fatal("f");
        getManager().log(act::foundation::LogsLevel::Enum::WARNING, "l");
        getManager().traceStream() << "ts";
        getManager().debugStream() << "ds";
        getManager().infoStream() << "is";
        getManager().warningStream() << "ws";
        getManager().errorStream() << "es";
        getManager().fatalStream() << "fs";
        getManager().logStream(act::foundation::LogsLevel::Enum::ERR) << "ls";

        const std::vector<std::pair<act::foundation::LogsLevel::Enum, std::string>> expected{
            {act::foundation::LogsLevel::Enum::TRACE, "t"},
            {act::foundation::LogsLevel::Enum::DEBG, "d"},
            {act::foundation::LogsLevel::Enum::INFO, "i"},
            {act::foundation::LogsLevel::Enum::WARNING, "w"},
            {act::foundation::LogsLevel::Enum::ERR, "e"},
            {act::foundation::LogsLevel::Enum::FATAL, "f"},
            {act::foundation::LogsLevel::Enum::WARNING, "l"},
            {act::foundation::LogsLevel::Enum::TRACE, "ts"},
            {act::foundation::LogsLevel::Enum::DEBG, "ds"},
            {act::foundation::LogsLevel::Enum::INFO, "is"},
            {act::foundation::LogsLevel::Enum::WARNING, "ws"},
            {act::foundation::LogsLevel::Enum::ERR, "es"},
            {act::foundation::LogsLevel::Enum::FATAL, "fs"},
            {act::foundation::LogsLevel::Enum::ERR, "ls"},
        };
        const auto &entries = getExternal()->getEntries();
        // The first entry is the init message
        ASSERT_EQ(entries.size(), expected.size() + 1);
        for (std::size_t idx = 0; idx < expected.size(); ++idx)
        {
            EXPECT_EQ(entries[idx + 1].level, expected[idx].first);
            EXPECT_EQ(entries[idx + 1].message, expected[idx].second);
        }
    }

    TEST_F(AbsLoggerManagerTest, WouldBeLoggedFollowsTheExternalLogger)
    {
        getExternal()->setMinLevel(act::foundation::LogsLevel::Enum::WARNING);

        EXPECT_FALSE(getManager().wouldBeLogged(act::foundation::LogsLevel::Enum::INFO));
        EXPECT_TRUE(getManager().wouldBeLogged(act::foundation::LogsLevel::Enum::WARNING));
    }

    TEST_F(AbsLoggerManagerTest, SubLoggersCarryTheirCategory)
    {
        const auto sub =
            getManager().createSubLogger("sub", act::foundation::LogsLevel::Enum::INFO);
        const auto noCategory = getManager().createSubLogger();
        const auto absSub =
            getManager().createAbsSubLogger("abs", act::foundation::LogsLevel::Enum::TRACE);
        const auto absNoCategory =
            getManager().createAbsSubLogger(act::foundation::LogsLevel::Enum::TRACE);

        sub->debug("dropped");
        sub->info("sub");
        noCategory->info("none");
        absSub->info("abs");
        absNoCategory->info("abs none");

        const auto &entries = getExternal()->getEntries();
        ASSERT_EQ(entries.size(), 5U);
        EXPECT_EQ(entries[1].categories, std::vector<std::string>{"sub"});
        EXPECT_TRUE(entries[2].categories.empty());
        EXPECT_EQ(entries[3].categories, std::vector<std::string>{"abs"});
        EXPECT_TRUE(entries[4].categories.empty());
    }

} // namespace
} // namespace act::logger
