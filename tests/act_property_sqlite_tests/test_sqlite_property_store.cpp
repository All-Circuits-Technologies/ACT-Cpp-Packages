// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_sqlite_property_store.cpp
 * @brief Unit tests of SqlitePropertyStore.
 *
 * These tests run against a real SQLite database file, created in a temporary directory for each
 * test. Covers the table provisioning (created at init or expected to exist), the round trip of
 * every stored type with the SQLite storage class it is written with, the strict reading of rows
 * which do not match their tag, overwriting, erasing and clearing, a table name which needs
 * quoting, the use of the store inside a caller transaction, a dedicated database reopened, and a
 * property manager reading and writing its properties through the store.
 */

#include "act_property_sqlite/stores/sqlite_property_store.hpp"

#include "act_db_core/db_transaction.hpp"
#include "act_db_sqlite/sqlite_db_manager.hpp"
#include "act_db_sqlite/sqlite_db_protect_service.hpp"
#include "act_foundation/logger/logs_level.hpp"
#include "act_logger/services/logger_manager.hpp"
#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/properties/rw_property.hpp"
#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"
#include "act_property_core/services/abs_property_manager.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"
#include "act_property_sqlite/types/table_provisioning.hpp"
#include "act_test_common/recording_logger.hpp"

#include <gtest/gtest.h>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

namespace act::property
{
namespace
{
    using act::foundation::LogsLevel;

    /** @brief The name of the table most tests use */
    const std::string TABLE_NAME{"properties"};

    /** @brief The key most tests use */
    const std::string KEY{"some.key"};

    /** @brief A float exactly representable, so it reads back equal */
    constexpr float SAMPLE_FLOAT = 1.5F;

    /** @brief A double exactly representable, so it reads back equal */
    constexpr double SAMPLE_DOUBLE = -2.25;

    /** @brief Concrete manager: the library class only lacks the life cycle init */
    class SqliteDbManager : public act::db::sqlite::ASqLiteDbManager
    {
      public:
        using ASqLiteDbManager::ASqLiteDbManager;

        bool init() override
        {
            return true;
        }
    };

    /**
     * @brief Recording logger whose sub-loggers record into it, so the messages of a component
     *        logging through a sub-logger can be checked
     */
    class CollectingLogger : public act::tests::RecordingLogger
    {
      public:
        [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
            const std::string & /*category*/, LogsLevel::Enum /*minLevel*/) override
        {
            return nonOwningSelf();
        }

        [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
            LogsLevel::Enum /*minLevel*/) override
        {
            return nonOwningSelf();
        }

      private:
        /** @brief Get a pointer to this logger which does not own it */
        [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> nonOwningSelf()
        {
            return {std::shared_ptr<act::foundation::AbsLogger>{}, this};
        }
    };

    /** @brief Owns a temporary directory holding an opened database and its protect service */
    class SqlitePropertyStoreTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());

            std::string dirTemplate =
                (std::filesystem::temp_directory_path() / "act-property-sqlite-test-XXXXXX")
                    .string();
            ASSERT_NE(mkdtemp(dirTemplate.data()), nullptr);
            m_tmpDir = dirTemplate;

            m_db = std::make_unique<SqliteDbManager>(m_tmpDir / "app.sqlite",
                                                     "property-store",
                                                     std::nullopt,
                                                     m_loggerManager);
            ASSERT_TRUE(m_db->open(false));

            m_service =
                std::make_unique<act::db::sqlite::SQLiteDbProtectService>(*m_db, m_loggerManager);
        }

        void TearDown() override
        {
            m_service.reset();
            m_db.reset();
            std::filesystem::remove_all(m_tmpDir);
        }

        /** @brief Build a store on the test database, logging into the collecting logger */
        [[nodiscard]] std::unique_ptr<SqlitePropertyStore> makeStore(
            const std::string &tableName = TABLE_NAME,
            TableProvisioning provisioning = TableProvisioning::SELF_CREATE)
        {
            return std::make_unique<SqlitePropertyStore>(*m_service,
                                                         m_logger,
                                                         tableName,
                                                         provisioning);
        }

        /** @brief Build a self created store and initialize it */
        [[nodiscard]] std::unique_ptr<SqlitePropertyStore> makeReadyStore()
        {
            auto store = makeStore();
            EXPECT_TRUE(store->init());
            return store;
        }

        /** @brief Insert a row as is, the type and value being SQL literals */
        void insertRawRow(const std::string &key,
                          const std::string &typeLiteral,
                          const std::string &valueLiteral)
        {
            ASSERT_TRUE(m_db->exec("INSERT INTO \"" + TABLE_NAME + "\" VALUES ('" + key + "', " +
                                   typeLiteral + ", " + valueLiteral + ")"));
        }

        /** @brief Run a query returning a single text, such as `typeof(...)` */
        [[nodiscard]] std::string queryText(const std::string &sql) const
        {
            SQLite::Statement statement(*m_db->getHandle(), sql);
            if (!statement.executeStep())
            {
                return {};
            }

            return statement.getColumn(0).getString();
        }

        /** @brief Run a query returning a single 64 bits integer */
        [[nodiscard]] std::optional<std::int64_t> queryInt64(const std::string &sql) const
        {
            SQLite::Statement statement(*m_db->getHandle(), sql);
            if (!statement.executeStep())
            {
                return std::nullopt;
            }

            return statement.getColumn(0).getInt64();
        }

        /** @brief Test if the database has a table with exactly this name */
        [[nodiscard]] bool tableExists(const std::string &tableName) const
        {
            SQLite::Statement statement(
                *m_db->getHandle(),
                "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?");
            statement.bind(1, tableName);
            return statement.executeStep();
        }

        /** @brief Count the rows of the test table */
        [[nodiscard]] std::optional<int> countRows() const
        {
            return m_db->execAndGetInt("SELECT COUNT(*) FROM \"" + TABLE_NAME + "\"");
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

        [[nodiscard]] act::db::sqlite::SQLiteDbProtectService &getService()
        {
            return *m_service;
        }

        [[nodiscard]] CollectingLogger &getLogger()
        {
            return m_logger;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        CollectingLogger m_logger;
        std::filesystem::path m_tmpDir;
        std::unique_ptr<SqliteDbManager> m_db;
        std::unique_ptr<act::db::sqlite::SQLiteDbProtectService> m_service;
    };

    // -----------------------------------------------------------------------------------------
    // Table provisioning
    // -----------------------------------------------------------------------------------------

    TEST_F(SqlitePropertyStoreTest, SelfCreateInitCreatesTheTable)
    {
        auto store = makeStore();

        EXPECT_TRUE(store->init());
        EXPECT_TRUE(tableExists(TABLE_NAME));
        EXPECT_EQ(countRows(), std::optional<int>(0));
    }

    TEST_F(SqlitePropertyStoreTest, SelfCreateInitKeepsAnExistingTableAndItsRows)
    {
        ASSERT_TRUE(makeReadyStore()->set(KEY, StoredValue(std::int32_t{12})));

        auto store = makeStore();

        EXPECT_TRUE(store->init());
        EXPECT_EQ(store->get(KEY), StoredValue(std::int32_t{12}));
    }

    TEST_F(SqlitePropertyStoreTest, ExternalInitSucceedsWhenTheTableExists)
    {
        ASSERT_TRUE(getDb().exec("CREATE TABLE properties (\"key\" TEXT PRIMARY KEY, \"type\" "
                                 "INTEGER NOT NULL, \"value\") WITHOUT ROWID"));
        auto store = makeStore(TABLE_NAME, TableProvisioning::EXTERNAL);

        EXPECT_TRUE(store->init());
        EXPECT_TRUE(store->set(KEY, StoredValue(true)));
        EXPECT_EQ(store->get(KEY), StoredValue(true));
    }

    TEST_F(SqlitePropertyStoreTest, ExternalInitFailsWithoutTheTableAndDoesNotCreateIt)
    {
        auto store = makeStore(TABLE_NAME, TableProvisioning::EXTERNAL);

        EXPECT_FALSE(store->init());
        EXPECT_FALSE(tableExists(TABLE_NAME));
        EXPECT_TRUE(getLogger().contains(LogsLevel::ERR, "does not exist"));
        EXPECT_FALSE(store->set(KEY, StoredValue(true)));
    }

    TEST_F(SqlitePropertyStoreTest, ExternalInitFindsTheTableWhateverItsCase)
    {
        ASSERT_TRUE(getDb().exec("CREATE TABLE Properties (\"key\" TEXT PRIMARY KEY, \"type\" "
                                 "INTEGER NOT NULL, \"value\") WITHOUT ROWID"));
        auto store = makeStore("properties", TableProvisioning::EXTERNAL);

        EXPECT_TRUE(store->init());
    }

    TEST_F(SqlitePropertyStoreTest, InitFailsOnAClosedDatabase)
    {
        SqliteDbManager closedDb(getTmpDir() / "closed.sqlite",
                                 "closed",
                                 std::nullopt,
                                 getLoggerManager());
        act::db::sqlite::SQLiteDbProtectService service(closedDb, getLoggerManager());

        SqlitePropertyStore selfCreated(service,
                                        getLogger(),
                                        TABLE_NAME,
                                        TableProvisioning::SELF_CREATE);
        SqlitePropertyStore external(service, getLogger(), TABLE_NAME, TableProvisioning::EXTERNAL);

        EXPECT_FALSE(selfCreated.init());
        EXPECT_FALSE(external.init());
    }

    TEST_F(SqlitePropertyStoreTest, OperationsFailOnAClosedDatabase)
    {
        SqliteDbManager closedDb(getTmpDir() / "closed.sqlite",
                                 "closed",
                                 std::nullopt,
                                 getLoggerManager());
        act::db::sqlite::SQLiteDbProtectService service(closedDb, getLoggerManager());
        SqlitePropertyStore store(service, getLogger(), TABLE_NAME, TableProvisioning::SELF_CREATE);

        EXPECT_FALSE(store.set(KEY, StoredValue(true)));
        EXPECT_EQ(store.get(KEY), std::nullopt);
        EXPECT_FALSE(store.erase(KEY));
        EXPECT_FALSE(store.clearAll());
    }

    // -----------------------------------------------------------------------------------------
    // Round trip of every stored type
    // -----------------------------------------------------------------------------------------

    /** @brief A value written then read back, with the storage class SQLite must keep */
    struct RoundTripCase
    {
        const char *name;
        StoredValue value;
        const char *storageClass;
    };

    /** @brief Show a case by its name in the test listings */
    void PrintTo(const RoundTripCase &testCase, std::ostream *stream)
    {
        *stream << testCase.name;
    }

    class SqlitePropertyStoreRoundTripTest : public SqlitePropertyStoreTest,
                                             public ::testing::WithParamInterface<RoundTripCase>
    {
    };

    TEST_P(SqlitePropertyStoreRoundTripTest, ValueReadsBackEqualWithItsNativeStorageClass)
    {
        const RoundTripCase &testCase = GetParam();
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set(KEY, testCase.value));

        EXPECT_EQ(store->get(KEY), testCase.value);
        EXPECT_EQ(queryText("SELECT typeof(\"value\") FROM properties"), testCase.storageClass);
        EXPECT_EQ(queryInt64("SELECT \"type\" FROM properties"),
                  static_cast<std::int64_t>(testCase.value.type()));
        EXPECT_EQ(getLogger().count(LogsLevel::WARNING), 0U);
    }

    template <typename T>
    RoundTripCase MinCase(const char *name)
    {
        return {name, StoredValue(std::numeric_limits<T>::min()), "integer"};
    }

    template <typename T>
    RoundTripCase MaxCase(const char *name)
    {
        return {name, StoredValue(std::numeric_limits<T>::max()), "integer"};
    }

    INSTANTIATE_TEST_SUITE_P(
        EveryStoredType,
        SqlitePropertyStoreRoundTripTest,
        ::testing::Values(
            RoundTripCase{"BoolTrue", StoredValue(true), "integer"},
            RoundTripCase{"BoolFalse", StoredValue(false), "integer"},
            MinCase<std::int8_t>("Int8Min"),
            MaxCase<std::int8_t>("Int8Max"),
            MinCase<std::int16_t>("Int16Min"),
            MaxCase<std::int16_t>("Int16Max"),
            MinCase<std::int32_t>("Int32Min"),
            MaxCase<std::int32_t>("Int32Max"),
            MinCase<std::int64_t>("Int64Min"),
            MaxCase<std::int64_t>("Int64Max"),
            MinCase<std::uint8_t>("UInt8Min"),
            MaxCase<std::uint8_t>("UInt8Max"),
            MaxCase<std::uint16_t>("UInt16Max"),
            MaxCase<std::uint32_t>("UInt32Max"),
            MinCase<std::uint64_t>("UInt64Min"),
            MaxCase<std::uint64_t>("UInt64Max"),
            RoundTripCase{"UInt64AboveInt64Max",
                          StoredValue(std::uint64_t{std::numeric_limits<std::int64_t>::max()} + 1U),
                          "integer"},
            RoundTripCase{"Float", StoredValue(SAMPLE_FLOAT), "real"},
            RoundTripCase{"FloatMax", StoredValue(std::numeric_limits<float>::max()), "real"},
            RoundTripCase{"FloatLowest", StoredValue(std::numeric_limits<float>::lowest()), "real"},
            RoundTripCase{"Double", StoredValue(SAMPLE_DOUBLE), "real"},
            RoundTripCase{"DoubleMax", StoredValue(std::numeric_limits<double>::max()), "real"},
            RoundTripCase{"String", StoredValue(std::string{"it's \"quoted\""}), "text"},
            RoundTripCase{"EmptyString", StoredValue(std::string{}), "text"}),
        [](const ::testing::TestParamInfo<RoundTripCase> &info) { return info.param.name; });

    TEST_F(SqlitePropertyStoreTest, UInt64IsStoredAsItsSignedReinterpretation)
    {
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set(KEY, StoredValue(std::numeric_limits<std::uint64_t>::max())));
        EXPECT_EQ(queryInt64("SELECT \"value\" FROM properties"), std::optional<std::int64_t>(-1));

        ASSERT_TRUE(
            store->set(KEY,
                       StoredValue(std::uint64_t{std::numeric_limits<std::int64_t>::max()} + 1U)));
        EXPECT_EQ(queryInt64("SELECT \"value\" FROM properties"),
                  std::optional<std::int64_t>(std::numeric_limits<std::int64_t>::min()));
    }

    TEST_F(SqlitePropertyStoreTest, BoolIsStoredAsZeroOrOne)
    {
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set(KEY, StoredValue(true)));
        EXPECT_EQ(queryInt64("SELECT \"value\" FROM properties"), std::optional<std::int64_t>(1));

        ASSERT_TRUE(store->set(KEY, StoredValue(false)));
        EXPECT_EQ(queryInt64("SELECT \"value\" FROM properties"), std::optional<std::int64_t>(0));
    }

    TEST_F(SqlitePropertyStoreTest, NanReadsBackAsAbsent)
    {
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set(KEY, StoredValue(std::numeric_limits<double>::quiet_NaN())));
        EXPECT_EQ(store->get(KEY), std::nullopt);

        ASSERT_TRUE(store->set(KEY, StoredValue(std::numeric_limits<float>::quiet_NaN())));
        EXPECT_EQ(store->get(KEY), std::nullopt);
    }

    // -----------------------------------------------------------------------------------------
    // Strict reading
    // -----------------------------------------------------------------------------------------

    /** @brief A row inserted as is, which must read as absent with the given explanation */
    struct InvalidRowCase
    {
        const char *name;
        const char *typeLiteral;
        const char *valueLiteral;
        const char *explanation;
    };

    /** @brief Show a case by its name in the test listings */
    void PrintTo(const InvalidRowCase &testCase, std::ostream *stream)
    {
        *stream << testCase.name;
    }

    class SqlitePropertyStoreInvalidRowTest : public SqlitePropertyStoreTest,
                                              public ::testing::WithParamInterface<InvalidRowCase>
    {
    };

    TEST_P(SqlitePropertyStoreInvalidRowTest, RowReadsAsAbsentAndTheCauseIsLogged)
    {
        const InvalidRowCase &testCase = GetParam();
        auto store = makeReadyStore();
        insertRawRow(KEY, testCase.typeLiteral, testCase.valueLiteral);

        EXPECT_EQ(store->get(KEY), std::nullopt);
        EXPECT_TRUE(getLogger().contains(LogsLevel::WARNING, testCase.explanation));
        EXPECT_TRUE(getLogger().contains(LogsLevel::WARNING,
                                         "The row of key '" + KEY + "' in table '" + TABLE_NAME +
                                             "' cannot be read back"));
    }

    INSTANTIATE_TEST_SUITE_P(
        StrictReading,
        SqlitePropertyStoreInvalidRowTest,
        ::testing::Values(
            InvalidRowCase{"UnknownTag", "99", "1", "The type tag 99 is unknown"},
            InvalidRowCase{"NegativeTag", "-1", "1", "The type tag -1 is unknown"},
            InvalidRowCase{"TagOutOfIntRange", "1099511627776", "1", "is out of the int range"},
            InvalidRowCase{"TagNotAnInteger",
                           "'int'",
                           "1",
                           "holds storage class TEXT where INTEGER is expected"},
            InvalidRowCase{"Int8AboveMax", "1", "300", "The integer 300 does not fit"},
            InvalidRowCase{"Int16BelowMin", "2", "-40000", "The integer -40000 does not fit"},
            InvalidRowCase{
                "Int32AboveMax", "3", "2147483648", "The integer 2147483648 does not fit"},
            InvalidRowCase{"UInt8Negative", "5", "-1", "The integer -1 does not fit"},
            InvalidRowCase{"UInt16AboveMax", "6", "65536", "The integer 65536 does not fit"},
            InvalidRowCase{"UInt32Negative", "7", "-1", "The integer -1 does not fit"},
            InvalidRowCase{"BoolNeitherZeroNorOne", "0", "2", "The boolean 2 is neither 0 nor 1"},
            InvalidRowCase{
                "BoolAsText", "0", "'true'", "holds storage class TEXT where INTEGER is expected"},
            InvalidRowCase{
                "IntegerAsText", "3", "'12'", "holds storage class TEXT where INTEGER is expected"},
            InvalidRowCase{
                "IntegerAsReal", "4", "1.5", "holds storage class REAL where INTEGER is expected"},
            InvalidRowCase{
                "UInt64AsText", "8", "'1'", "holds storage class TEXT where INTEGER is expected"},
            InvalidRowCase{
                "FloatAsInteger", "9", "1", "holds storage class INTEGER where REAL is expected"},
            InvalidRowCase{"FloatOutOfRange", "9", "1e300", "is out of the float range"},
            InvalidRowCase{
                "DoubleAsInteger", "10", "1", "holds storage class INTEGER where REAL is expected"},
            InvalidRowCase{
                "StringAsInteger", "11", "5", "holds storage class INTEGER where TEXT is expected"},
            InvalidRowCase{
                "StringAsBlob", "11", "x'00ff'", "holds storage class BLOB where TEXT is expected"},
            InvalidRowCase{
                "NullValue", "11", "NULL", "holds storage class NULL where TEXT is expected"}),
        [](const ::testing::TestParamInfo<InvalidRowCase> &info) { return info.param.name; });

    TEST_F(SqlitePropertyStoreTest, MissingKeyReadsAsAbsentWithoutWarning)
    {
        auto store = makeReadyStore();

        EXPECT_EQ(store->get("missing"), std::nullopt);
        EXPECT_EQ(getLogger().count(LogsLevel::WARNING), 0U);
    }

    TEST_F(SqlitePropertyStoreTest, InvalidRowCanBeOverwritten)
    {
        auto store = makeReadyStore();
        insertRawRow(KEY, "99", "1");

        ASSERT_TRUE(store->set(KEY, StoredValue(std::int16_t{-3})));

        EXPECT_EQ(store->get(KEY), StoredValue(std::int16_t{-3}));
    }

    // -----------------------------------------------------------------------------------------
    // Writing, erasing and clearing
    // -----------------------------------------------------------------------------------------

    TEST_F(SqlitePropertyStoreTest, SetReplacesTheValueAndTheTypeOfAKey)
    {
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set(KEY, StoredValue(std::int32_t{1})));
        ASSERT_TRUE(store->set(KEY, StoredValue(std::string{"text"})));

        EXPECT_EQ(store->get(KEY), StoredValue(std::string{"text"}));
        EXPECT_EQ(countRows(), std::optional<int>(1));
        EXPECT_EQ(queryText("SELECT typeof(\"value\") FROM properties"), "text");
    }

    TEST_F(SqlitePropertyStoreTest, KeysAreCaseSensitive)
    {
        auto store = makeReadyStore();

        ASSERT_TRUE(store->set("key", StoredValue(std::int32_t{1})));
        ASSERT_TRUE(store->set("KEY", StoredValue(std::int32_t{2})));

        EXPECT_EQ(store->get("key"), StoredValue(std::int32_t{1}));
        EXPECT_EQ(store->get("KEY"), StoredValue(std::int32_t{2}));
    }

    TEST_F(SqlitePropertyStoreTest, KeyIsBoundNotInterpolated)
    {
        auto store = makeReadyStore();
        const std::string trickyKey{"a'); DROP TABLE properties; --"};

        ASSERT_TRUE(store->set(trickyKey, StoredValue(true)));

        EXPECT_EQ(store->get(trickyKey), StoredValue(true));
        EXPECT_TRUE(tableExists(TABLE_NAME));
    }

    TEST_F(SqlitePropertyStoreTest, EraseRemovesOnlyThatKey)
    {
        auto store = makeReadyStore();
        ASSERT_TRUE(store->set("first", StoredValue(true)));
        ASSERT_TRUE(store->set("second", StoredValue(false)));

        EXPECT_TRUE(store->erase("first"));

        EXPECT_EQ(store->get("first"), std::nullopt);
        EXPECT_EQ(store->get("second"), StoredValue(false));
    }

    TEST_F(SqlitePropertyStoreTest, EraseOfAMissingKeySucceeds)
    {
        auto store = makeReadyStore();

        EXPECT_TRUE(store->erase("missing"));
    }

    TEST_F(SqlitePropertyStoreTest, ClearAllRemovesEveryRow)
    {
        auto store = makeReadyStore();
        ASSERT_TRUE(store->set("first", StoredValue(true)));
        ASSERT_TRUE(store->set("second", StoredValue(false)));

        EXPECT_TRUE(store->clearAll());

        EXPECT_EQ(countRows(), std::optional<int>(0));
        EXPECT_TRUE(tableExists(TABLE_NAME));
    }

    TEST_F(SqlitePropertyStoreTest, TableNameNeedingQuotesIsAccepted)
    {
        const std::string oddName{"my \"odd\" table; DROP TABLE x"};
        auto store = makeStore(oddName);

        ASSERT_TRUE(store->init());
        ASSERT_TRUE(store->set(KEY, StoredValue(std::int8_t{-7})));

        EXPECT_TRUE(tableExists(oddName));
        EXPECT_EQ(store->get(KEY), StoredValue(std::int8_t{-7}));
        EXPECT_TRUE(store->erase(KEY));
        EXPECT_TRUE(store->clearAll());
    }

    TEST_F(SqlitePropertyStoreTest, StoresOnTwoTablesAreIndependent)
    {
        auto first = makeStore("first");
        auto second = makeStore("second");
        ASSERT_TRUE(first->init());
        ASSERT_TRUE(second->init());

        ASSERT_TRUE(first->set(KEY, StoredValue(std::int32_t{1})));
        ASSERT_TRUE(second->set(KEY, StoredValue(std::int32_t{2})));
        ASSERT_TRUE(second->clearAll());

        EXPECT_EQ(first->get(KEY), StoredValue(std::int32_t{1}));
        EXPECT_EQ(second->get(KEY), std::nullopt);
    }

    // -----------------------------------------------------------------------------------------
    // Transactions and databases
    // -----------------------------------------------------------------------------------------

    TEST_F(SqlitePropertyStoreTest, WorksInsideACallerTransactionWhichCommits)
    {
        auto store = makeReadyStore();

        EXPECT_TRUE(getService().protectQuery(
            [&store](act::db::sqlite::ASqLiteDbManager & /*db*/) {
                return store->set(KEY, StoredValue(std::int32_t{5})) && store->get(KEY).has_value();
            },
            "caller"));

        EXPECT_EQ(store->get(KEY), StoredValue(std::int32_t{5}));
    }

    TEST_F(SqlitePropertyStoreTest, WritesAreUndoneWhenTheCallerTransactionRollsBack)
    {
        auto store = makeReadyStore();
        ASSERT_TRUE(store->set(KEY, StoredValue(std::int32_t{1})));

        EXPECT_FALSE(getService().protectQuery(
            [&store](act::db::sqlite::ASqLiteDbManager & /*db*/) {
                EXPECT_TRUE(store->set(KEY, StoredValue(std::int32_t{2})));
                EXPECT_TRUE(store->set("other", StoredValue(true)));
                EXPECT_EQ(store->get(KEY), StoredValue(std::int32_t{2}));
                return false;
            },
            "caller"));

        EXPECT_EQ(store->get(KEY), StoredValue(std::int32_t{1}));
        EXPECT_EQ(store->get("other"), std::nullopt);
    }

    TEST_F(SqlitePropertyStoreTest, WritesAreUndoneByAnExplicitRollback)
    {
        auto store = makeReadyStore();
        act::db::core::DbTransaction transaction(getDb(), getLogger());
        ASSERT_TRUE(transaction.begin());

        ASSERT_TRUE(store->set(KEY, StoredValue(true)));
        ASSERT_TRUE(transaction.rollback());

        EXPECT_EQ(store->get(KEY), std::nullopt);
    }

    TEST_F(SqlitePropertyStoreTest, DedicatedDatabaseKeepsItsValuesAcrossReopenings)
    {
        const std::filesystem::path dbPath = getTmpDir() / "dedicated.sqlite";
        {
            SqliteDbManager db(dbPath, "dedicated", std::nullopt, getLoggerManager());
            ASSERT_TRUE(db.open(false));
            act::db::sqlite::SQLiteDbProtectService service(db, getLoggerManager());
            SqlitePropertyStore store(service,
                                      getLogger(),
                                      TABLE_NAME,
                                      TableProvisioning::SELF_CREATE);
            ASSERT_TRUE(store.init());
            ASSERT_TRUE(store.set(KEY, StoredValue(std::string{"kept"})));
        }

        SqliteDbManager db(dbPath, "dedicated", std::nullopt, getLoggerManager());
        ASSERT_TRUE(db.open(false));
        act::db::sqlite::SQLiteDbProtectService service(db, getLoggerManager());
        SqlitePropertyStore store(service, getLogger(), TABLE_NAME, TableProvisioning::EXTERNAL);

        ASSERT_TRUE(store.init());
        EXPECT_EQ(store.get(KEY), StoredValue(std::string{"kept"}));
        // The application database of the fixture is untouched
        EXPECT_FALSE(tableExists(TABLE_NAME));
    }

    // -----------------------------------------------------------------------------------------
    // Through a property manager
    // -----------------------------------------------------------------------------------------

    /** @brief Default value of the read and write property of TestPropertyManager */
    constexpr std::int32_t DEFAULT_COUNT = 7;

    /** @brief Property manager holding its properties in the test database */
    class TestPropertyManager : public AbsPropertyManager
    {
      public:
        TestPropertyManager(act::db::sqlite::SQLiteDbProtectService &service,
                            act::logger::LoggerManager &logger)
            : AbsPropertyManager(logger),
              m_store(service, accessLogger(), TABLE_NAME, TableProvisioning::SELF_CREATE)
        {
        }

        RWProperty<std::int32_t> count{*this, "count", Default::of<std::int32_t>(DEFAULT_COUNT)};

        RProperty<std::string> name{*this, "name", Seed::of(std::string{"seeded"})};

      protected:
        [[nodiscard]] AbsPropertyStore &accessStore() override
        {
            return m_store;
        }

      private:
        SqlitePropertyStore m_store;
    };

    TEST_F(SqlitePropertyStoreTest, ManagerSeedsAndPersistsItsProperties)
    {
        {
            TestPropertyManager manager(getService(), getLoggerManager());
            ASSERT_TRUE(manager.init());

            // The default is a read fallback, never written; the seed is written at init
            EXPECT_EQ(manager.count.get(), std::optional<std::int32_t>(DEFAULT_COUNT));
            EXPECT_EQ(manager.name.get(), std::optional<std::string>("seeded"));
            EXPECT_EQ(countRows(), std::optional<int>(1));
            EXPECT_EQ(queryText("SELECT typeof(\"value\") FROM properties WHERE \"key\" = 'name'"),
                      "text");

            ASSERT_TRUE(manager.count.set(42));
        }

        TestPropertyManager manager(getService(), getLoggerManager());
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.count.get(), std::optional<std::int32_t>(42));
        EXPECT_EQ(queryText("SELECT typeof(\"value\") FROM properties WHERE \"key\" = 'count'"),
                  "integer");
    }

    TEST_F(SqlitePropertyStoreTest, ManagerRealignsAChangedSeed)
    {
        TestPropertyManager first(getService(), getLoggerManager());
        ASSERT_TRUE(first.init());
        ASSERT_TRUE(
            getDb().exec("UPDATE properties SET \"value\" = 'stale' WHERE \"key\" = 'name'"));

        TestPropertyManager second(getService(), getLoggerManager());
        ASSERT_TRUE(second.init());

        EXPECT_EQ(second.name.get(), std::optional<std::string>("seeded"));
    }

} // namespace
} // namespace act::property
