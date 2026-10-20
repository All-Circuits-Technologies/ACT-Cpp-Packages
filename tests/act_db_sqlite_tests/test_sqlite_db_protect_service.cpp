// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_sqlite_db_protect_service.cpp
 * @brief Unit tests of SQLiteDbProtectService.
 *
 * These tests run against a real SQLite database file, created in a temporary directory for each
 * test. Covers the changes kept when a protected query succeeds, rolled back when it fails or
 * throws a SQLite exception, kept when no transaction was asked for, the forwarding of a query
 * result, the refusal to run on a closed database, and a protected query with a transaction
 * nested inside another one.
 */

#include "act_db_sqlite/sqlite_db_protect_service.hpp"

#include "act_db_sqlite/sqlite_db_manager.hpp"
#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <SQLiteCpp/Database.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace act::db::sqlite
{
namespace
{
    /** @brief Concrete manager: the library class only lacks the life cycle init */
    class SqliteDbManager : public ASqLiteDbManager
    {
      public:
        using ASqLiteDbManager::ASqLiteDbManager;

        bool init() override
        {
            return true;
        }
    };

    class SqliteDbProtectServiceTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            std::string dirTemplate =
                (std::filesystem::temp_directory_path() / "act-db-sqlite-test-XXXXXX").string();
            ASSERT_NE(mkdtemp(dirTemplate.data()), nullptr);
            m_tmpDir = dirTemplate;

            m_db = std::make_unique<SqliteDbManager>(m_tmpDir / "test.sqlite",
                                                     "protect",
                                                     std::nullopt,
                                                     m_loggerManager);
            ASSERT_TRUE(m_db->open(false));
            ASSERT_TRUE(m_db->exec("CREATE TABLE t (a INTEGER)"));

            m_service = std::make_unique<SQLiteDbProtectService>(*m_db, m_loggerManager);
            ASSERT_TRUE(m_service->init());
        }

        void TearDown() override
        {
            m_service.reset();
            m_db.reset();
            std::filesystem::remove_all(m_tmpDir);
        }

        [[nodiscard]] std::optional<int> countRows() const
        {
            return m_db->execAndGetInt("SELECT COUNT(*) FROM t");
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] const std::filesystem::path &getTmpDir() const
        {
            return m_tmpDir;
        }

        [[nodiscard]] SqliteDbManager &getDb()
        {
            return *m_db;
        }

        [[nodiscard]] SQLiteDbProtectService &getService()
        {
            return *m_service;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::filesystem::path m_tmpDir;
        std::unique_ptr<SqliteDbManager> m_db;
        std::unique_ptr<SQLiteDbProtectService> m_service;
    };

    TEST_F(SqliteDbProtectServiceTest, SuccessfulQueryChangesAreKept)
    {
        EXPECT_TRUE(getService().protectQuery(
            [](ASqLiteDbManager &db) { return db.exec("INSERT INTO t VALUES (1)"); },
            "insert"));

        EXPECT_EQ(countRows(), std::optional<int>(1));
    }

    TEST_F(SqliteDbProtectServiceTest, FailedQueryChangesAreRolledBack)
    {
        EXPECT_FALSE(getService().protectQuery(
            [](ASqLiteDbManager &db) {
                return db.exec("INSERT INTO t VALUES (1)") &&
                       db.exec("INSERT INTO missing VALUES (1)");
            },
            "insert"));

        EXPECT_EQ(countRows(), std::optional<int>(0));
    }

    TEST_F(SqliteDbProtectServiceTest, ThrowingQueryChangesAreRolledBack)
    {
        EXPECT_FALSE(getService().protectQuery(
            [](ASqLiteDbManager &db) {
                db.getHandle()->exec("INSERT INTO t VALUES (1)");
                // Throws a SQLite::Exception
                db.getHandle()->exec("INSERT INTO missing VALUES (1)");
                return true;
            },
            "insert"));

        EXPECT_EQ(countRows(), std::optional<int>(0));
    }

    TEST_F(SqliteDbProtectServiceTest, QueryWithoutTransactionIsNotRolledBack)
    {
        EXPECT_FALSE(getService().protectQuery(
            [](ASqLiteDbManager &db) {
                return db.exec("INSERT INTO t VALUES (1)") &&
                       db.exec("INSERT INTO missing VALUES (1)");
            },
            "insert",
            false));

        EXPECT_EQ(countRows(), std::optional<int>(1));
    }

    TEST_F(SqliteDbProtectServiceTest, QueryResultIsForwarded)
    {
        ASSERT_TRUE(getDb().exec("INSERT INTO t VALUES (5); INSERT INTO t VALUES (6);"));

        const auto sum = getService().protectQueryWithResult<int>(
            [](ASqLiteDbManager &db) { return db.execAndGetInt("SELECT SUM(a) FROM t"); },
            "sum");

        EXPECT_EQ(sum, std::optional<int>(11));
    }

    TEST_F(SqliteDbProtectServiceTest, RefusesToRunOnAClosedDatabase)
    {
        SqliteDbManager closedDb(getTmpDir() / "closed.sqlite",
                                 "closed",
                                 std::nullopt,
                                 getLoggerManager());
        SQLiteDbProtectService service(closedDb, getLoggerManager());
        bool called = false;

        EXPECT_FALSE(service.protectQuery([&called](ASqLiteDbManager & /*db*/) {
            called = true;
            return true;
        }));
        EXPECT_FALSE(called);
    }

    TEST_F(SqliteDbProtectServiceTest, NestedTransactionIsRefusedWithoutUndoingTheOuterOne)
    {
        bool nestedResult = true;

        EXPECT_TRUE(getService().protectQuery(
            [this, &nestedResult](ASqLiteDbManager &db) {
                // SQLite cannot begin a transaction inside another one
                nestedResult = getService().protectQuery(
                    [](ASqLiteDbManager &innerDb) {
                        return innerDb.exec("INSERT INTO t VALUES (2)");
                    },
                    "nested");
                return db.exec("INSERT INTO t VALUES (1)");
            },
            "outer"));

        EXPECT_FALSE(nestedResult);
        EXPECT_EQ(getDb().execAndGetInt("SELECT a FROM t"), std::optional<int>(1));
        EXPECT_EQ(countRows(), std::optional<int>(1));
    }

    TEST_F(SqliteDbProtectServiceTest, NestedQueryWithoutTransactionJoinsTheOuterOne)
    {
        EXPECT_FALSE(getService().protectQuery(
            [this](ASqLiteDbManager & /*db*/) {
                EXPECT_TRUE(getService().protectQuery(
                    [](ASqLiteDbManager &innerDb) {
                        return innerDb.exec("INSERT INTO t VALUES (2)");
                    },
                    "nested",
                    false));
                return false;
            },
            "outer"));

        // The outer failure rolls back what the nested query wrote
        EXPECT_EQ(countRows(), std::optional<int>(0));
    }
} // namespace
} // namespace act::db::sqlite
