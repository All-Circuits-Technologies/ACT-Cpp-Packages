// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_db_protect_service.cpp
 * @brief Unit tests of DbProtectService.
 *
 * Covers the refusal to run on a closed database, the transaction wrapped around a query
 * (committed on success, rolled back on a failure, an empty result or an exception), the query run
 * without a transaction, the forwarding of a query result, and the failures of the transaction
 * statements themselves.
 */

#include "act_db_core/services/db_protect_service.hpp"

#include "act_foundation/constants/def_soft.hpp"
#include "act_logger/services/logger_manager.hpp"
#include "fake_db_manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace act::db::core
{
namespace
{
    using ::testing::ElementsAre;
    using ::testing::IsEmpty;

    const std::string BEGIN = "BEGIN TRANSACTION";
    const std::string COMMIT = "COMMIT TRANSACTION";
    const std::string ROLLBACK = "ROLLBACK TRANSACTION";
    const std::string QUERY = "INSERT INTO t VALUES (1)";
    constexpr int QUERY_RESULT = 42;

    class DbProtectServiceTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());
            m_db = std::make_unique<test::FakeDbManager>("protect", std::nullopt, m_loggerManager);
            ASSERT_TRUE(m_db->open(false));
            m_service = std::make_unique<DbProtectService<>>(*m_db, m_loggerManager);
            ASSERT_TRUE(m_service->init());
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] test::FakeDbManager &getDb()
        {
            return *m_db;
        }

        [[nodiscard]] DbProtectService<> &getService()
        {
            return *m_service;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::unique_ptr<test::FakeDbManager> m_db;
        std::unique_ptr<DbProtectService<>> m_service;
    };

    TEST_F(DbProtectServiceTest, GivesAccessToTheProtectedDatabase)
    {
        EXPECT_EQ(&getService().accessDb(), &getDb());
        EXPECT_EQ(&getService().getDb(), &getDb());
    }

    TEST_F(DbProtectServiceTest, RefusesToRunOnAClosedDatabase)
    {
        test::FakeDbManager closedDb("closed", std::nullopt, getLoggerManager());
        DbProtectService<> service(closedDb, getLoggerManager());
        bool called = false;

        EXPECT_FALSE(service.protectQuery([&called](AbsDbManager & /*db*/) {
            called = true;
            return true;
        }));
        EXPECT_FALSE(called);
        EXPECT_THAT(closedDb.getExecuted(), IsEmpty());
    }

    TEST_F(DbProtectServiceTest, SuccessfulQueryIsCommitted)
    {
        EXPECT_TRUE(getService().protectQuery([](AbsDbManager &db) { return db.exec(QUERY); }));

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, QUERY, COMMIT));
    }

    TEST_F(DbProtectServiceTest, FailedQueryIsRolledBack)
    {
        getDb().failStatement(QUERY);

        EXPECT_FALSE(getService().protectQuery([](AbsDbManager &db) { return db.exec(QUERY); }));

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, QUERY, ROLLBACK));
    }

    TEST_F(DbProtectServiceTest, ThrowingQueryIsRolledBackAndReportedAsFailed)
    {
        EXPECT_FALSE(getService().protectQuery([](AbsDbManager &db) -> bool {
            UNUSED(db.exec(QUERY));
            throw std::runtime_error("query failure");
        }));

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, QUERY, ROLLBACK));
    }

    TEST_F(DbProtectServiceTest, QueryWithoutTransactionRunsAlone)
    {
        EXPECT_TRUE(getService().protectQuery([](AbsDbManager &db) { return db.exec(QUERY); },
                                              "no transaction",
                                              false));

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(QUERY));
    }

    TEST_F(DbProtectServiceTest, ThrowingQueryWithoutTransactionIsReportedAsFailed)
    {
        EXPECT_FALSE(getService().protectQuery(
            [](AbsDbManager & /*db*/) -> bool { throw std::runtime_error("query failure"); },
            "no transaction",
            false));

        EXPECT_THAT(getDb().getExecuted(), IsEmpty());
    }

    TEST_F(DbProtectServiceTest, QueryResultIsForwarded)
    {
        const auto result = getService().protectQueryWithResult<int>(
            [](AbsDbManager & /*db*/) -> std::optional<int> { return QUERY_RESULT; });

        EXPECT_EQ(result, std::optional<int>(QUERY_RESULT));
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, COMMIT));
    }

    TEST_F(DbProtectServiceTest, EmptyQueryResultIsRolledBack)
    {
        const auto result = getService().protectQueryWithResult<int>(
            [](AbsDbManager & /*db*/) -> std::optional<int> { return std::nullopt; });

        EXPECT_EQ(result, std::nullopt);
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, ROLLBACK));
    }

    TEST_F(DbProtectServiceTest, QueryIsNotRunWhenTheTransactionCannotBegin)
    {
        getDb().failStatement(BEGIN);
        bool called = false;

        EXPECT_FALSE(getService().protectQuery([&called](AbsDbManager & /*db*/) {
            called = true;
            return true;
        }));
        EXPECT_FALSE(called);
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN));
    }

    TEST_F(DbProtectServiceTest, FailedCommitIsReportedAsFailed)
    {
        getDb().failStatement(COMMIT);

        const auto result = getService().protectQueryWithResult<int>(
            [](AbsDbManager & /*db*/) -> std::optional<int> { return QUERY_RESULT; });

        EXPECT_EQ(result, std::nullopt);
        // The transaction left open by the failed commit is rolled back when leaving
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, COMMIT, ROLLBACK));
    }
} // namespace
} // namespace act::db::core
