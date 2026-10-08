// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/services/abs_property_store.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_sqlite/types/table_provisioning.hpp"

#include <memory>
#include <optional>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::db::sqlite
{
class SQLiteDbProtectService;
} // namespace act::db::sqlite

namespace act::property
{

/**
 * @brief A store backed by a key/value table of a SQLite database.
 *
 * The same class serves a table inside an application database and a dedicated database: in both
 * cases the store receives the protect service of the database that holds the table. Every query
 * goes through that service, with bound parameters (values are never interpolated into the SQL).
 *
 * The table has a generic schema, which never changes when a property is added:
 *
 * @code{.sql}
 * CREATE TABLE IF NOT EXISTS "<table>" (
 *     "key"   TEXT PRIMARY KEY,
 *     "type"  INTEGER NOT NULL,
 *     "value"
 * ) WITHOUT ROWID;
 * @endcode
 *
 * With @ref TableProvisioning::SELF_CREATE, @ref init runs this statement. With
 * @ref TableProvisioning::EXTERNAL, the migrations of the database owner must create the table with
 * this schema, and @ref init only checks that it exists.
 *
 * The `type` column holds the @ref StoredType tag. The `value` column has no declared type, so it
 * keeps the SQLite storage class it is written with:
 * - @ref StoredType::BOOL is an `INTEGER`, 0 or 1;
 * - the signed integers and @ref StoredType::UINT8 to @ref StoredType::UINT32 are an `INTEGER`
 *   holding the value itself;
 * - @ref StoredType::UINT64 is an `INTEGER` holding the two's complement reinterpretation of the
 *   value as a signed 64 bits integer, since a SQLite integer is signed. The round trip is
 *   lossless, but an external reader of the table sees a negative number for any value above
 *   `INT64_MAX` (2^63 - 1): it must reinterpret the column as unsigned when the tag is
 *   @ref StoredType::UINT64;
 * - @ref StoredType::FLOAT and @ref StoredType::DOUBLE are a `REAL`;
 * - @ref StoredType::STRING is a `TEXT`.
 *
 * Reading is strict: a row whose tag is unknown, whose storage class does not match its tag, or
 * whose integer does not fit the tagged type (for example 300 tagged @ref StoredType::INT8) reads
 * as absent, and the mismatch is logged. SQLite stores a NaN floating point value as `NULL`, so a
 * NaN written to the store reads back as absent.
 *
 * No query opens a transaction: each one is a single statement, hence already atomic, and the
 * store stays usable while the caller holds a transaction on the same connection.
 */
class SqlitePropertyStore : public AbsPropertyStore
{
  public:
    /**
     * @brief Construct the store
     * @note The table name is quoted in every query, so any name is accepted.
     * @param dbProtectService The protect service of the database holding the table
     * @param parentLogger The logger used to create the store sub-logger
     * @param tableName The name of the table holding the properties
     * @param provisioning Whether the store creates the table at init or expects it to exist
     */
    explicit SqlitePropertyStore(act::db::sqlite::SQLiteDbProtectService &dbProtectService,
                                 act::foundation::AbsLogger &parentLogger,
                                 std::string tableName,
                                 TableProvisioning provisioning);

    /// @brief Nothing special for default destructor
    ~SqlitePropertyStore() override = default;

    /**
     * @brief Prepare the table: create it, or check that it exists, depending on the provisioning
     * @return true if the table is ready, false otherwise
     */
    bool init() override;

    /** @copydoc AbsPropertyStore::get */
    [[nodiscard]] std::optional<StoredValue> get(const std::string &key) const override;

    /** @copydoc AbsPropertyStore::set */
    bool set(const std::string &key, const StoredValue &value) override;

    /** @copydoc AbsPropertyStore::erase */
    bool erase(const std::string &key) override;

    /** @copydoc AbsPropertyStore::clearAll */
    bool clearAll() override;

  private:
    /**
     * @brief Create the table if it does not exist yet
     * @return true on success, false otherwise
     */
    bool createTable();

    /**
     * @brief Check that the table exists
     * @return true if the table exists, false otherwise
     */
    bool checkTableExists();

  private:
    /** @brief The protect service of the database holding the table */
    act::db::sqlite::SQLiteDbProtectService &m_dbProtectService;

    /** @brief The store sub-logger */
    std::shared_ptr<act::foundation::AbsLogger> m_logger;

    /** @brief The table name, as given (not quoted) */
    std::string m_tableName;

    /** @brief Whether the store creates the table at init or expects it to exist */
    TableProvisioning m_provisioning;

    /** @brief The query reading the row of a key */
    std::string m_getQuery;

    /** @brief The query creating or replacing the row of a key */
    std::string m_setQuery;

    /** @brief The query removing the row of a key */
    std::string m_eraseQuery;

    /** @brief The query removing every row */
    std::string m_clearAllQuery;
};

} // namespace act::property
