// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_sqlite_db_manager.cpp
 * @brief Unit tests of ASqLiteDbManager.
 *
 * These tests run against a real SQLite database file, created in a temporary directory for each
 * test. Covers the operations refused before opening, the file and directories creation, the
 * double opening, statements execution and integer queries (including invalid SQL), the migration
 * version kept in `user_version` across reopenings, migration scripts applied at opening (and
 * rolled back when one fails), defragmentation, busy timeout and the REGEXP SQL function.
 */

#include "act_db_sqlite/sqlite_db_manager.hpp"

#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <SQLiteCpp/Database.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
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

    class SqliteDbManagerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            std::string dirTemplate =
                (std::filesystem::temp_directory_path() / "act-db-sqlite-test-XXXXXX").string();
            ASSERT_NE(mkdtemp(dirTemplate.data()), nullptr);
            m_tmpDir = dirTemplate;

            // A slug unique to this test, so the migration lock file is too
            m_slug = m_tmpDir.filename().string();
            m_dbPath = m_tmpDir / "test.sqlite";
            m_migrationDir = m_tmpDir / "migrations";
            std::filesystem::create_directories(m_migrationDir);
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_tmpDir);
            std::filesystem::remove(std::filesystem::temp_directory_path() /
                                    (m_slug + "-db-database-migration.lock"));
        }

        std::unique_ptr<SqliteDbManager> createDb()
        {
            return std::make_unique<SqliteDbManager>(m_dbPath,
                                                     m_slug,
                                                     m_migrationDir,
                                                     m_loggerManager);
        }

        void writeMigration(int fromVersion, const std::string &content) const
        {
            std::ofstream file(m_migrationDir /
                               (m_slug + "-db-v" + std::to_string(fromVersion) + "-to-v" +
                                std::to_string(fromVersion + 1) + ".sql"));
            file << content;
        }

        [[nodiscard]] const std::filesystem::path &getTmpDir() const
        {
            return m_tmpDir;
        }

        [[nodiscard]] const std::filesystem::path &getDbPath() const
        {
            return m_dbPath;
        }

        /** @brief Change the database file of the managers created afterwards */
        void setDbPath(const std::filesystem::path &dbPath)
        {
            m_dbPath = dbPath;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::filesystem::path m_tmpDir;
        std::filesystem::path m_dbPath;
        std::filesystem::path m_migrationDir;
        std::string m_slug;
    };

    TEST_F(SqliteDbManagerTest, OperationsFailBeforeOpening)
    {
        auto db = createDb();

        EXPECT_FALSE(db->isOpened());
        EXPECT_EQ(db->getHandle(), nullptr);
        EXPECT_FALSE(db->exec("SELECT 1"));
        EXPECT_EQ(db->execAndGetInt("SELECT 1"), std::nullopt);
        EXPECT_EQ(db->getMigrationVersion(), 0);
        EXPECT_FALSE(db->setMigrationVersion(1));
        EXPECT_FALSE(db->defrag());
        EXPECT_FALSE(db->setBusyTimeout(100));
        EXPECT_FALSE(std::filesystem::exists(getDbPath()));
    }

    TEST_F(SqliteDbManagerTest, OpenCreatesTheFileAndItsDirectories)
    {
        setDbPath(getTmpDir() / "sub" / "dir" / "test.sqlite");
        auto db = createDb();

        EXPECT_TRUE(db->open());

        EXPECT_TRUE(db->isOpened());
        ASSERT_NE(db->getHandle(), nullptr);
        EXPECT_TRUE(std::filesystem::exists(getDbPath()));
    }

    TEST_F(SqliteDbManagerTest, OpenTwiceFails)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        EXPECT_FALSE(db->open());
        EXPECT_TRUE(db->isOpened());
    }

    TEST_F(SqliteDbManagerTest, ExecAndGetIntReturnsTheFirstColumnOfTheFirstRow)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        ASSERT_TRUE(db->exec("CREATE TABLE t (a INTEGER, b INTEGER);"
                             "INSERT INTO t VALUES (3, 30);"
                             "INSERT INTO t VALUES (4, 40);"));

        EXPECT_EQ(db->execAndGetInt("SELECT COUNT(*) FROM t"), std::optional<int>(2));
        EXPECT_EQ(db->execAndGetInt("SELECT a, b FROM t ORDER BY a"), std::optional<int>(3));
    }

    TEST_F(SqliteDbManagerTest, InvalidSqlIsReportedAsAFailure)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        EXPECT_FALSE(db->exec("NOT A STATEMENT"));
        EXPECT_FALSE(db->exec("SELECT * FROM missing_table"));
        EXPECT_EQ(db->execAndGetInt("NOT A STATEMENT"), std::nullopt);
    }

    TEST_F(SqliteDbManagerTest, ExecAndGetIntFailsWithoutAResultRow)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());
        ASSERT_TRUE(db->exec("CREATE TABLE t (a INTEGER)"));

        EXPECT_EQ(db->execAndGetInt("SELECT a FROM t"), std::nullopt);
    }

    TEST_F(SqliteDbManagerTest, MigrationVersionIsKeptAcrossReopenings)
    {
        {
            auto db = createDb();
            ASSERT_TRUE(db->open());
            EXPECT_EQ(db->getMigrationVersion(), 0);

            EXPECT_TRUE(db->setMigrationVersion(5));
            EXPECT_EQ(db->getMigrationVersion(), 5);
        }

        auto db = createDb();
        ASSERT_TRUE(db->open(false));
        EXPECT_EQ(db->getMigrationVersion(), 5);
        EXPECT_EQ(db->execAndGetInt("PRAGMA user_version"), std::optional<int>(5));
    }

    TEST_F(SqliteDbManagerTest, DataIsKeptAcrossReopenings)
    {
        {
            auto db = createDb();
            ASSERT_TRUE(db->open());
            ASSERT_TRUE(db->exec("CREATE TABLE t (a INTEGER); INSERT INTO t VALUES (7);"));
        }

        auto db = createDb();
        ASSERT_TRUE(db->open());
        EXPECT_EQ(db->execAndGetInt("SELECT a FROM t"), std::optional<int>(7));
    }

    TEST_F(SqliteDbManagerTest, OpenAppliesTheMigrationScripts)
    {
        writeMigration(0, "CREATE TABLE t (a INTEGER);");
        writeMigration(1, "INSERT INTO t VALUES (1); INSERT INTO t VALUES (2);");
        auto db = createDb();

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 2);
        EXPECT_EQ(db->execAndGetInt("SELECT COUNT(*) FROM t"), std::optional<int>(2));
    }

    TEST_F(SqliteDbManagerTest, FailedMigrationScriptIsRolledBack)
    {
        writeMigration(0, "CREATE TABLE t (a INTEGER);");
        writeMigration(1, "CREATE TABLE u (a INTEGER); INSERT INTO t VALUES (1); NOT SQL;");
        auto db = createDb();

        EXPECT_FALSE(db->open());

        ASSERT_TRUE(db->isOpened());
        EXPECT_EQ(db->getMigrationVersion(), 1);
        EXPECT_TRUE(db->getHandle()->tableExists("t"));
        EXPECT_FALSE(db->getHandle()->tableExists("u"));
        EXPECT_EQ(db->execAndGetInt("SELECT COUNT(*) FROM t"), std::optional<int>(0));
    }

    TEST_F(SqliteDbManagerTest, DefragSucceedsOnAnOpenedDatabase)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());
        ASSERT_TRUE(db->exec("CREATE TABLE t (a INTEGER); INSERT INTO t VALUES (1);"));

        EXPECT_TRUE(db->defrag());
        EXPECT_EQ(db->execAndGetInt("SELECT a FROM t"), std::optional<int>(1));
    }

    TEST_F(SqliteDbManagerTest, DefragFailsInsideATransaction)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());
        ASSERT_TRUE(db->exec("BEGIN TRANSACTION"));

        EXPECT_FALSE(db->defrag());

        EXPECT_TRUE(db->exec("ROLLBACK TRANSACTION"));
    }

    TEST_F(SqliteDbManagerTest, BusyTimeoutCanBeSetOnceOpened)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        EXPECT_TRUE(db->setBusyTimeout(250));
    }

    TEST_F(SqliteDbManagerTest, RegexpFunctionMatchesTheWholeText)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        EXPECT_EQ(db->execAndGetInt("SELECT '192.168.1.10' REGEXP '([0-9]{1,3}\\.){3}[0-9]{1,3}'"),
                  std::optional<int>(1));
        EXPECT_EQ(db->execAndGetInt("SELECT 'abc' REGEXP 'a.c'"), std::optional<int>(1));
        // The match is anchored on both ends
        EXPECT_EQ(db->execAndGetInt("SELECT 'xabc' REGEXP 'a.c'"), std::optional<int>(0));
        EXPECT_EQ(db->execAndGetInt("SELECT 'abd' REGEXP 'a.c'"), std::optional<int>(0));
    }

    TEST_F(SqliteDbManagerTest, RegexpFunctionDoesNotMatchNullOrAnInvalidPattern)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open());

        EXPECT_EQ(db->execAndGetInt("SELECT NULL REGEXP 'a'"), std::optional<int>(0));
        EXPECT_EQ(db->execAndGetInt("SELECT 'a' REGEXP NULL"), std::optional<int>(0));
        EXPECT_EQ(db->execAndGetInt("SELECT 'a' REGEXP '(['"), std::optional<int>(0));
    }
} // namespace
} // namespace act::db::sqlite
