// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_db_manager.cpp
 * @brief Unit tests of AbsDbManager.
 *
 * The engine-agnostic part is driven through a test double which records the executed statements.
 * Covers the opening with and without migrations, the migration scripts lookup
 * (`<slug>-db-v<N>-to-v<N+1>.sql`, applied in order from the current version, each in its own
 * transaction, stopping at the first missing version), the failures of a script or of the version
 * update, the missing migration directory, and the default script runner.
 */

#include "act_db_core/services/abs_db_manager.hpp"

#include "act_logger/services/logger_manager.hpp"
#include "fake_db_manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
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

    class AbsDbManagerTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            std::string dirTemplate =
                (std::filesystem::temp_directory_path() / "act-db-core-test-XXXXXX").string();
            ASSERT_NE(mkdtemp(dirTemplate.data()), nullptr);
            m_tmpDir = dirTemplate;

            // A slug unique to this test, so the migration lock file is too
            m_slug = m_tmpDir.filename().string();
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_tmpDir);
            std::filesystem::remove(std::filesystem::temp_directory_path() /
                                    (m_slug + "-db-database-migration.lock"));
        }

        /** @brief Create a manager whose migration scripts are looked for in the temp dir */
        std::unique_ptr<test::FakeDbManager> createDb()
        {
            return std::make_unique<test::FakeDbManager>(m_slug, m_tmpDir, m_loggerManager);
        }

        /** @brief Write the script migrating from @p fromVersion and return its content */
        std::string writeMigration(int fromVersion)
        {
            const std::string content = "-- migration from v" + std::to_string(fromVersion);
            WriteFile(m_tmpDir / (m_slug + "-db-v" + std::to_string(fromVersion) + "-to-v" +
                                  std::to_string(fromVersion + 1) + ".sql"),
                      content);
            return content;
        }

        static void WriteFile(const std::filesystem::path &path, const std::string &content)
        {
            std::ofstream file(path);
            file << content;
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] const std::filesystem::path &getTmpDir() const
        {
            return m_tmpDir;
        }

        [[nodiscard]] const std::string &getSlug() const
        {
            return m_slug;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::filesystem::path m_tmpDir;
        std::string m_slug;
    };

    TEST_F(AbsDbManagerTest, OpenWithoutMigrationOnlyOpens)
    {
        auto db = createDb();
        writeMigration(0);

        EXPECT_TRUE(db->open(false));

        EXPECT_TRUE(db->isOpened());
        EXPECT_EQ(db->getMigrationVersion(), 0);
        EXPECT_THAT(db->getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, FailedOpeningSkipsTheMigrations)
    {
        auto db = createDb();
        db->setFailOpen(true);
        writeMigration(0);

        EXPECT_FALSE(db->open());

        EXPECT_FALSE(db->isOpened());
        EXPECT_THAT(db->getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, OpenAppliesTheMigrationsInOrder)
    {
        auto db = createDb();
        const auto toV1 = writeMigration(0);
        const auto toV2 = writeMigration(1);

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 2);
        EXPECT_THAT(db->getExecuted(), ElementsAre(BEGIN, toV1, COMMIT, BEGIN, toV2, COMMIT));
    }

    TEST_F(AbsDbManagerTest, MigrationsStartFromTheCurrentVersion)
    {
        auto db = createDb();
        db->forceMigrationVersion(1);
        writeMigration(0);
        const auto toV2 = writeMigration(1);

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 2);
        EXPECT_THAT(db->getExecuted(), ElementsAre(BEGIN, toV2, COMMIT));
    }

    TEST_F(AbsDbManagerTest, MigrationsStopAtTheFirstMissingVersion)
    {
        auto db = createDb();
        const auto toV1 = writeMigration(0);
        writeMigration(2);

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 1);
        EXPECT_THAT(db->getExecuted(), ElementsAre(BEGIN, toV1, COMMIT));
    }

    TEST_F(AbsDbManagerTest, NoMigrationScriptIsNotAnError)
    {
        auto db = createDb();

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 0);
        EXPECT_THAT(db->getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, ScriptsOfAnotherSlugAreIgnored)
    {
        auto db = createDb();
        WriteFile(getTmpDir() / "other-db-v0-to-v1.sql", "-- other");

        EXPECT_TRUE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 0);
        EXPECT_THAT(db->getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, FailedScriptIsRolledBackAndStopsTheMigrations)
    {
        auto db = createDb();
        const auto toV1 = writeMigration(0);
        const auto toV2 = writeMigration(1);
        writeMigration(2);
        db->failStatement(toV2);

        EXPECT_FALSE(db->open());

        // The first migration is kept, the failing one leaves the version where it was
        EXPECT_EQ(db->getMigrationVersion(), 1);
        EXPECT_THAT(db->getExecuted(), ElementsAre(BEGIN, toV1, COMMIT, BEGIN, toV2, ROLLBACK));
    }

    TEST_F(AbsDbManagerTest, FailedVersionUpdateFailsTheMigration)
    {
        auto db = createDb();
        const auto toV1 = writeMigration(0);
        writeMigration(1);
        db->setFailSetMigrationVersion(true);

        EXPECT_FALSE(db->open());

        EXPECT_EQ(db->getMigrationVersion(), 0);
        EXPECT_THAT(db->getExecuted(), ElementsAre(BEGIN, toV1, COMMIT));
    }

    TEST_F(AbsDbManagerTest, MigrationFailsWithoutAMigrationDirectory)
    {
        test::FakeDbManager db(getSlug(), std::nullopt, getLoggerManager());

        // The database itself is opened, only the migration step fails
        EXPECT_FALSE(db.open());
        EXPECT_TRUE(db.isOpened());

        EXPECT_FALSE(db.applyMigrationUpdates());
        EXPECT_THAT(db.getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, RunScriptExecutesTheWholeFile)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open(false));
        const std::string script = "CREATE TABLE t (a INTEGER);\nINSERT INTO t VALUES (1);\n";
        WriteFile(getTmpDir() / "script.sql", script);

        EXPECT_TRUE(db->runScript(getTmpDir() / "script.sql"));

        EXPECT_THAT(db->getExecuted(), ElementsAre(script));
    }

    TEST_F(AbsDbManagerTest, RunScriptFailsOnAMissingFile)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open(false));

        EXPECT_FALSE(db->runScript(getTmpDir() / "missing.sql"));

        EXPECT_THAT(db->getExecuted(), IsEmpty());
    }

    TEST_F(AbsDbManagerTest, RunScriptReportsAFailedExecution)
    {
        auto db = createDb();
        ASSERT_TRUE(db->open(false));
        WriteFile(getTmpDir() / "script.sql", "BROKEN");
        db->failStatement("BROKEN");

        EXPECT_FALSE(db->runScript(getTmpDir() / "script.sql"));
    }
} // namespace
} // namespace act::db::core
