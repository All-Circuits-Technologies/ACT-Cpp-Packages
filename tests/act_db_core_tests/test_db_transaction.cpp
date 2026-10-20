// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_db_transaction.cpp
 * @brief Unit tests of DbTransaction.
 *
 * Covers the statements sent for begin (with or without an extension), commit and rollback, the
 * state checks (no commit or rollback before begin, no double begin, no commit after the end),
 * restarting a finished transaction, failures reported by the database, and the rollback done by
 * the destructor when the transaction was left open.
 */

#include "act_db_core/db_transaction.hpp"

#include "act_logger/services/logger_manager.hpp"
#include "fake_db_manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
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

    class DbTransactionTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());
            m_db =
                std::make_unique<test::FakeDbManager>("transaction", std::nullopt, m_loggerManager);
        }

        [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
        {
            return m_loggerManager;
        }

        [[nodiscard]] test::FakeDbManager &getDb()
        {
            return *m_db;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::unique_ptr<test::FakeDbManager> m_db;
    };

    TEST_F(DbTransactionTest, BeginStartsATransaction)
    {
        DbTransaction transaction(getDb(), getLoggerManager());

        EXPECT_TRUE(transaction.begin());
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN));
        EXPECT_TRUE(transaction.commit());
    }

    TEST_F(DbTransactionTest, BeginAppendsTheExtension)
    {
        DbTransaction transaction(getDb(), getLoggerManager());

        EXPECT_TRUE(transaction.begin("IMMEDIATE TRANSACTION"));
        EXPECT_TRUE(transaction.commit());

        EXPECT_THAT(getDb().getExecuted(), ElementsAre("BEGIN IMMEDIATE TRANSACTION", COMMIT));
    }

    TEST_F(DbTransactionTest, BeginWithAnEmptyExtensionSendsBeginAlone)
    {
        DbTransaction transaction(getDb(), getLoggerManager());

        EXPECT_TRUE(transaction.begin(""));
        EXPECT_TRUE(transaction.commit());

        EXPECT_THAT(getDb().getExecuted(), ElementsAre("BEGIN", COMMIT));
    }

    TEST_F(DbTransactionTest, CommitEndsTheTransaction)
    {
        {
            DbTransaction transaction(getDb(), getLoggerManager());
            ASSERT_TRUE(transaction.begin());
            EXPECT_TRUE(transaction.commit());
        }

        // Nothing is rolled back by the destructor once committed
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, COMMIT));
    }

    TEST_F(DbTransactionTest, RollbackEndsTheTransaction)
    {
        {
            DbTransaction transaction(getDb(), getLoggerManager());
            ASSERT_TRUE(transaction.begin());
            EXPECT_TRUE(transaction.rollback());
        }

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, ROLLBACK));
    }

    TEST_F(DbTransactionTest, DestructorRollsBackAnOpenTransaction)
    {
        {
            DbTransaction transaction(getDb(), getLoggerManager());
            ASSERT_TRUE(transaction.begin());
        }

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, ROLLBACK));
    }

    TEST_F(DbTransactionTest, DestructorDoesNothingWhenNeverStarted)
    {
        {
            const DbTransaction transaction(getDb(), getLoggerManager());
        }

        EXPECT_THAT(getDb().getExecuted(), IsEmpty());
    }

    TEST_F(DbTransactionTest, CommitAndRollbackFailBeforeBegin)
    {
        DbTransaction transaction(getDb(), getLoggerManager());

        EXPECT_FALSE(transaction.commit());
        EXPECT_FALSE(transaction.rollback());
        EXPECT_THAT(getDb().getExecuted(), IsEmpty());
    }

    TEST_F(DbTransactionTest, BeginTwiceFails)
    {
        DbTransaction transaction(getDb(), getLoggerManager());

        ASSERT_TRUE(transaction.begin());
        EXPECT_FALSE(transaction.begin());
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN));
    }

    TEST_F(DbTransactionTest, CannotEndAFinishedTransactionAgain)
    {
        DbTransaction transaction(getDb(), getLoggerManager());
        ASSERT_TRUE(transaction.begin());
        ASSERT_TRUE(transaction.commit());

        EXPECT_FALSE(transaction.commit());
        EXPECT_FALSE(transaction.rollback());
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, COMMIT));
    }

    TEST_F(DbTransactionTest, FinishedTransactionCanBeStartedAgain)
    {
        DbTransaction transaction(getDb(), getLoggerManager());
        ASSERT_TRUE(transaction.begin());
        ASSERT_TRUE(transaction.rollback());

        EXPECT_TRUE(transaction.begin());
        EXPECT_TRUE(transaction.commit());
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, ROLLBACK, BEGIN, COMMIT));
    }

    TEST_F(DbTransactionTest, FailedBeginLeavesTheTransactionNotStarted)
    {
        getDb().failStatement(BEGIN);
        {
            DbTransaction transaction(getDb(), getLoggerManager());

            EXPECT_FALSE(transaction.begin());
            EXPECT_FALSE(transaction.commit());
        }

        // Neither the commit nor the destructor sends anything after the failed begin
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN));
    }

    TEST_F(DbTransactionTest, FailedCommitLeavesTheTransactionOpen)
    {
        getDb().failStatement(COMMIT);
        {
            DbTransaction transaction(getDb(), getLoggerManager());
            ASSERT_TRUE(transaction.begin());

            EXPECT_FALSE(transaction.commit());
        }

        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, COMMIT, ROLLBACK));
    }

    TEST_F(DbTransactionTest, FailedRollbackIsReported)
    {
        getDb().failStatement(ROLLBACK);
        DbTransaction transaction(getDb(), getLoggerManager());
        ASSERT_TRUE(transaction.begin());

        EXPECT_FALSE(transaction.rollback());

        // Still open, so it can be committed instead
        EXPECT_TRUE(transaction.commit());
        EXPECT_THAT(getDb().getExecuted(), ElementsAre(BEGIN, ROLLBACK, COMMIT));
    }
} // namespace
} // namespace act::db::core
