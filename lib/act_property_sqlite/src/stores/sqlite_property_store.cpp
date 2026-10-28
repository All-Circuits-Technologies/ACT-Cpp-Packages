// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_sqlite/stores/sqlite_property_store.hpp"

#include "act_db_sqlite/sqlite_db_constants.hpp"
#include "act_db_sqlite/sqlite_db_manager.hpp"
#include "act_db_sqlite/sqlite_db_protect_service.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/logger/logs_level.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"
#include "act_property_sqlite/types/table_provisioning.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Statement.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace act::property
{

namespace
{
    /** @brief Sub-logger category for a SQLite property store */
    constexpr const char *LoggerCategory = "property-store";

    /** @brief The query creating the table if needed, `{}` being the quoted table name */
    constexpr const char *CreateTableQuery =
        R"(CREATE TABLE IF NOT EXISTS {} ()"
        R"("key" TEXT PRIMARY KEY, "type" INTEGER NOT NULL, "value") WITHOUT ROWID)";

    /** @brief The query looking a table up by its name, case insensitively like SQLite does */
    constexpr const char *TableExistsQuery =
        R"(SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ? COLLATE NOCASE)";

    /** @brief The query reading the row of a key, `{}` being the quoted table name */
    constexpr const char *GetQuery = R"(SELECT "type", "value" FROM {} WHERE "key" = ?)";

    /** @brief The query creating or replacing the row of a key, `{}` being the quoted table name */
    constexpr const char *SetQuery =
        R"(INSERT INTO {} ("key", "type", "value") VALUES (?, ?, ?) )"
        R"(ON CONFLICT ("key") DO UPDATE SET "type" = excluded."type", "value" = excluded."value")";

    /** @brief The query removing the row of a key, `{}` being the quoted table name */
    constexpr const char *EraseQuery = R"(DELETE FROM {} WHERE "key" = ?)";

    /** @brief The query removing every row, `{}` being the quoted table name */
    constexpr const char *ClearAllQuery = R"(DELETE FROM {})";

    /** @brief The index of the key parameter, in the queries binding a key */
    constexpr int KeyParamIndex = 1;

    /** @brief The index of the type tag parameter, in the upsert query */
    constexpr int TypeParamIndex = 2;

    /** @brief The index of the value parameter, in the upsert query */
    constexpr int ValueParamIndex = 3;

    /** @brief The index of the table name parameter, in the table lookup query */
    constexpr int TableNameParamIndex = 1;

    /** @brief The index of the type tag column, in the result of the read query */
    constexpr int TypeResultIndex = 0;

    /** @brief The index of the value column, in the result of the read query */
    constexpr int ValueResultIndex = 1;

    /** @brief The SQLite name of the integer storage class, for the logs */
    constexpr const char *IntegerStorageClass = "INTEGER";

    /** @brief The SQLite name of the floating point storage class, for the logs */
    constexpr const char *RealStorageClass = "REAL";

    /** @brief The SQLite name of the text storage class, for the logs */
    constexpr const char *TextStorageClass = "TEXT";

    /** @brief The SQLite name of the binary storage class, for the logs */
    constexpr const char *BlobStorageClass = "BLOB";

    /** @brief The SQLite name of the storage class of a missing value, for the logs */
    constexpr const char *NullStorageClass = "NULL";

    /**
     * @brief Quote an SQL identifier, doubling the double quotes it contains
     * @param identifier The identifier to quote
     * @return The quoted identifier
     */
    std::string quoteIdentifier(const std::string &identifier)
    {
        std::string quoted = "\"";
        for (const char character : identifier)
        {
            if (character == '"')
            {
                quoted += '"';
            }

            quoted += character;
        }

        quoted += '"';
        return quoted;
    }

    // The accessors are read without a check on purpose: one that does not match the tag
    // throws, and the protect service catches it (see bindValue)
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    /**
     * @brief Bind an integer value, reading it through its typed accessor
     * @param statement The statement to bind the value to
     * @param value The value, whose accessor matches its tag
     */
    template <typename T>
    void bindInteger(SQLite::Statement &statement, const std::optional<T> &value)
    {
        statement.bind(ValueParamIndex, static_cast<std::int64_t>(value.value()));
    }

    /**
     * @brief Bind a stored value to the value parameter of the upsert query
     * @note A value whose accessor does not match its tag throws, which the protect service
     *       catches.
     * @param statement The statement to bind the value to
     * @param value The value to bind
     */
    void bindValue(SQLite::Statement &statement, const StoredValue &value)
    {
        switch (value.type())
        {
            case StoredType::BOOL:
                statement.bind(ValueParamIndex,
                               value.asBool().value() ? act::db::sqlite::SQLiteDbConstants::TRUE
                                                      : act::db::sqlite::SQLiteDbConstants::FALSE);
                break;
            case StoredType::INT8:
                bindInteger(statement, value.asInt8());
                break;
            case StoredType::INT16:
                bindInteger(statement, value.asInt16());
                break;
            case StoredType::INT32:
                bindInteger(statement, value.asInt32());
                break;
            case StoredType::INT64:
                bindInteger(statement, value.asInt64());
                break;
            case StoredType::UINT8:
                bindInteger(statement, value.asUInt8());
                break;
            case StoredType::UINT16:
                bindInteger(statement, value.asUInt16());
                break;
            case StoredType::UINT32:
                bindInteger(statement, value.asUInt32());
                break;
            case StoredType::UINT64:
                // A SQLite integer is signed: keep the bits, the tag tells how to read them back
                statement.bind(ValueParamIndex,
                               std::bit_cast<std::int64_t>(value.asUInt64().value()));
                break;
            case StoredType::FLOAT:
                statement.bind(ValueParamIndex, static_cast<double>(value.asFloat().value()));
                break;
            case StoredType::DOUBLE:
                statement.bind(ValueParamIndex, value.asDouble().value());
                break;
            case StoredType::STRING:
                statement.bind(ValueParamIndex, value.asString().value());
                break;
        }
    }
    // NOLINTEND(bugprone-unchecked-optional-access)

    /** @brief The row being read, so a rejected row is logged on a single line naming it */
    struct RowContext
    {
        /** @brief The logger explaining a rejected row */
        const act::foundation::AbsLogger &logger;

        /** @brief The key of the row */
        const std::string &key;

        /** @brief The name of the table holding the row */
        const std::string &tableName;

        /** @brief The type tag of the row, once it is known to be an integer */
        std::optional<std::int64_t> typeTag;
    };

    /**
     * @brief Log that a row cannot be read back, and why
     * @param row The row being read
     * @param reason Why the row is rejected
     */
    void logRejectedRow(const RowContext &row, const std::string &reason)
    {
        auto stream = row.logger.warningStream();
        stream << "The row of key '" << row.key << "' in table '" << row.tableName << "'";
        if (row.typeTag.has_value())
        {
            stream << " (type tag " << *row.typeTag << ")";
        }

        stream << " reads as absent: " << reason;
    }

    /**
     * @brief Log that a column does not hold the storage class its reader expects
     * @param row The row being read
     * @param column The column
     * @param expected The SQLite name of the expected storage class
     */
    void logStorageClassMismatch(const RowContext &row,
                                 const SQLite::Column &column,
                                 const char *expected)
    {
        const char *actual = NullStorageClass;
        if (column.isInteger())
        {
            actual = IntegerStorageClass;
        }
        else if (column.isFloat())
        {
            actual = RealStorageClass;
        }
        else if (column.isText())
        {
            actual = TextStorageClass;
        }
        else if (column.isBlob())
        {
            actual = BlobStorageClass;
        }

        logRejectedRow(row,
                       std::format("the '{}' column holds storage class {} where {} is expected",
                                   column.getName(),
                                   actual,
                                   expected));
    }

    /**
     * @brief Read an integer column into a value of the tagged type
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer or does not fit @p T
     */
    template <typename T>
    std::optional<StoredValue> readInteger(const RowContext &row, const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(row, column, IntegerStorageClass);
            return std::nullopt;
        }

        const std::int64_t raw = column.getInt64();
        if (!std::in_range<T>(raw))
        {
            logRejectedRow(row, std::format("the integer {} does not fit its tagged type", raw));
            return std::nullopt;
        }

        return StoredValue(static_cast<T>(raw));
    }

    /**
     * @brief Read a boolean column
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer equal to 0 or 1
     */
    std::optional<StoredValue> readBool(const RowContext &row, const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(row, column, IntegerStorageClass);
            return std::nullopt;
        }

        const std::int64_t raw = column.getInt64();
        if (raw == act::db::sqlite::SQLiteDbConstants::TRUE)
        {
            return StoredValue(true);
        }

        if (raw == act::db::sqlite::SQLiteDbConstants::FALSE)
        {
            return StoredValue(false);
        }

        logRejectedRow(row,
                       std::format("the boolean {} is neither {} nor {}",
                                   raw,
                                   static_cast<int>(act::db::sqlite::SQLiteDbConstants::FALSE),
                                   static_cast<int>(act::db::sqlite::SQLiteDbConstants::TRUE)));
        return std::nullopt;
    }

    /**
     * @brief Read an unsigned 64 bits integer column, stored as its signed reinterpretation
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer
     */
    std::optional<StoredValue> readUInt64(const RowContext &row, const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(row, column, IntegerStorageClass);
            return std::nullopt;
        }

        return StoredValue(std::bit_cast<std::uint64_t>(column.getInt64()));
    }

    /**
     * @brief Read a single precision floating point column
     * @note A `NULL` reads as NaN, see @ref readDouble.
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is neither a real nor `NULL`, or a
     * finite value is out of the float range
     */
    std::optional<StoredValue> readFloat(const RowContext &row, const SQLite::Column &column)
    {
        if (column.isNull())
        {
            return StoredValue(std::numeric_limits<float>::quiet_NaN());
        }

        if (!column.isFloat())
        {
            logStorageClassMismatch(row, column, RealStorageClass);
            return std::nullopt;
        }

        const double raw = column.getDouble();
        if (std::isfinite(raw) && (raw > static_cast<double>(std::numeric_limits<float>::max()) ||
                                   raw < static_cast<double>(std::numeric_limits<float>::lowest())))
        {
            logRejectedRow(row, std::format("the real {} is out of the float range", raw));
            return std::nullopt;
        }

        return StoredValue(static_cast<float>(raw));
    }

    /**
     * @brief Read a double precision floating point column
     * @note SQLite stores a NaN as `NULL`, which is the only `NULL` the store writes: reading it
     *       back as NaN keeps the round trip of a NaN value. The NaN payload and sign are lost,
     *       which @ref StoredValue equality ignores.
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is neither a real nor `NULL`
     */
    std::optional<StoredValue> readDouble(const RowContext &row, const SQLite::Column &column)
    {
        if (column.isNull())
        {
            return StoredValue(std::numeric_limits<double>::quiet_NaN());
        }

        if (!column.isFloat())
        {
            logStorageClassMismatch(row, column, RealStorageClass);
            return std::nullopt;
        }

        return StoredValue(column.getDouble());
    }

    /**
     * @brief Read a string column
     * @param row The row being read
     * @param column The value column
     * @return The value, or an empty optional if the column is not a text
     */
    std::optional<StoredValue> readString(const RowContext &row, const SQLite::Column &column)
    {
        if (!column.isText())
        {
            logStorageClassMismatch(row, column, TextStorageClass);
            return std::nullopt;
        }

        return StoredValue(column.getString());
    }

    /**
     * @brief Rebuild a stored value from the columns of a row
     * @note A rejected row is logged once, with its key, its table and the reason.
     * @param row The row being read
     * @param typeColumn The type tag column
     * @param valueColumn The value column
     * @return The value, or an empty optional if the tag is unknown or the value does not match it
     */
    std::optional<StoredValue> readRow(const RowContext &row,
                                       const SQLite::Column &typeColumn,
                                       const SQLite::Column &valueColumn)
    {
        if (!typeColumn.isInteger())
        {
            logStorageClassMismatch(row, typeColumn, IntegerStorageClass);
            return std::nullopt;
        }

        const std::int64_t typeTag = typeColumn.getInt64();
        if (!std::in_range<int>(typeTag))
        {
            logRejectedRow(row, std::format("the type tag {} is out of the int range", typeTag));
            return std::nullopt;
        }

        const RowContext taggedRow{.logger = row.logger,
                                   .key = row.key,
                                   .tableName = row.tableName,
                                   .typeTag = typeTag};

        // The underlying type is fixed, so any int is a valid StoredType; unknown tags hit default
        switch (static_cast<StoredType>(static_cast<int>(typeTag)))
        {
            case StoredType::BOOL:
                return readBool(taggedRow, valueColumn);
            case StoredType::INT8:
                return readInteger<std::int8_t>(taggedRow, valueColumn);
            case StoredType::INT16:
                return readInteger<std::int16_t>(taggedRow, valueColumn);
            case StoredType::INT32:
                return readInteger<std::int32_t>(taggedRow, valueColumn);
            case StoredType::INT64:
                return readInteger<std::int64_t>(taggedRow, valueColumn);
            case StoredType::UINT8:
                return readInteger<std::uint8_t>(taggedRow, valueColumn);
            case StoredType::UINT16:
                return readInteger<std::uint16_t>(taggedRow, valueColumn);
            case StoredType::UINT32:
                return readInteger<std::uint32_t>(taggedRow, valueColumn);
            case StoredType::UINT64:
                return readUInt64(taggedRow, valueColumn);
            case StoredType::FLOAT:
                return readFloat(taggedRow, valueColumn);
            case StoredType::DOUBLE:
                return readDouble(taggedRow, valueColumn);
            case StoredType::STRING:
                return readString(taggedRow, valueColumn);
            default:
                logRejectedRow(taggedRow, "the type tag is unknown");
                return std::nullopt;
        }
    }
} // namespace

SqlitePropertyStore::SqlitePropertyStore(act::db::sqlite::SQLiteDbProtectService &dbProtectService,
                                         act::foundation::AbsLogger &parentLogger,
                                         std::string tableName,
                                         TableProvisioning provisioning)
    : m_dbProtectService(dbProtectService),
      m_logger(
          parentLogger.createAbsSubLogger(LoggerCategory, act::foundation::LogsLevel::Enum::TRACE)),
      m_tableName(std::move(tableName)),
      m_provisioning(provisioning),
      m_getQuery(std::format(GetQuery, quoteIdentifier(m_tableName))),
      m_setQuery(std::format(SetQuery, quoteIdentifier(m_tableName))),
      m_eraseQuery(std::format(EraseQuery, quoteIdentifier(m_tableName))),
      m_clearAllQuery(std::format(ClearAllQuery, quoteIdentifier(m_tableName)))
{
}

bool SqlitePropertyStore::init()
{
    bool tableReady = false;
    switch (m_provisioning)
    {
        case TableProvisioning::SELF_CREATE:
            tableReady = createTable();
            break;
        case TableProvisioning::EXTERNAL:
            tableReady = checkTableExists();
            break;
    }

    return tableReady && checkTableSchema();
}

std::optional<StoredValue> SqlitePropertyStore::get(const std::string &key) const
{
    return m_dbProtectService.protectQueryWithResult<StoredValue>(
        [this, &key](act::db::sqlite::ASqLiteDbManager &db) -> std::optional<StoredValue> {
            SQLite::Statement statement(*db.getHandle(), m_getQuery);
            statement.bind(KeyParamIndex, key);
            if (!statement.executeStep())
            {
                return std::nullopt;
            }

            const RowContext row{.logger = *m_logger,
                                 .key = key,
                                 .tableName = m_tableName,
                                 .typeTag = std::nullopt};
            return readRow(row,
                           statement.getColumn(TypeResultIndex),
                           statement.getColumn(ValueResultIndex));
        },
        "SqlitePropertyStore::get",
        false);
}

bool SqlitePropertyStore::set(const std::string &key, const StoredValue &value)
{
    return m_dbProtectService.protectQuery(
        [this, &key, &value](act::db::sqlite::ASqLiteDbManager &db) {
            SQLite::Statement statement(*db.getHandle(), m_setQuery);
            statement.bind(KeyParamIndex, key);
            statement.bind(TypeParamIndex, static_cast<int>(value.type()));
            bindValue(statement, value);
            statement.exec();
            return true;
        },
        "SqlitePropertyStore::set",
        false);
}

bool SqlitePropertyStore::erase(const std::string &key)
{
    return m_dbProtectService.protectQuery(
        [this, &key](act::db::sqlite::ASqLiteDbManager &db) {
            SQLite::Statement statement(*db.getHandle(), m_eraseQuery);
            statement.bind(KeyParamIndex, key);
            statement.exec();
            return true;
        },
        "SqlitePropertyStore::erase",
        false);
}

bool SqlitePropertyStore::clearAll()
{
    return m_dbProtectService.protectQuery(
        [this](act::db::sqlite::ASqLiteDbManager &db) {
            SQLite::Statement statement(*db.getHandle(), m_clearAllQuery);
            statement.exec();
            return true;
        },
        "SqlitePropertyStore::clearAll",
        false);
}

bool SqlitePropertyStore::createTable()
{
    const std::string query = std::format(CreateTableQuery, quoteIdentifier(m_tableName));

    const bool created = m_dbProtectService.protectQuery(
        [&query](act::db::sqlite::ASqLiteDbManager &db) {
            db.getHandle()->exec(query);
            return true;
        },
        "SqlitePropertyStore::createTable",
        false);
    if (!created)
    {
        m_logger->errorStream() << "Failed to create the property table '" << m_tableName << "'";
    }

    return created;
}

bool SqlitePropertyStore::checkTableExists()
{
    const std::optional<bool> exists = m_dbProtectService.protectQueryWithResult<bool>(
        [this](act::db::sqlite::ASqLiteDbManager &db) -> std::optional<bool> {
            SQLite::Statement statement(*db.getHandle(), TableExistsQuery);
            statement.bind(TableNameParamIndex, m_tableName);
            return statement.executeStep();
        },
        "SqlitePropertyStore::checkTableExists",
        false);
    if (!exists.value_or(false))
    {
        m_logger->errorStream()
            << "The property table '" << m_tableName
            << "' does not exist; it must be created by the database migrations";
        return false;
    }

    return true;
}

bool SqlitePropertyStore::checkTableSchema()
{
    // Nothing else catches a table of another schema before the first query uses it:
    // - `CREATE TABLE IF NOT EXISTS` keeps an existing table of the same name, whatever its schema,
    //   and an external table is only looked up by name;
    // - SQLite reads a double quoted identifier which names no column as a string literal, so on a
    //   table without a "key" column, `WHERE "key" = ?` compares the literal 'key' to the bound
    //   key, and the read and erase queries silently match the wrong rows.
    // Preparing the upsert compiles it against the actual table without running it: it fails if a
    // column is missing, or if "key" is neither the primary key nor unique, as its ON CONFLICT
    // clause requires. The declared column types are not checked.
    const bool valid = m_dbProtectService.protectQuery(
        [this](act::db::sqlite::ASqLiteDbManager &db) {
            const SQLite::Statement statement(*db.getHandle(), m_setQuery);
            return true;
        },
        "SqlitePropertyStore::checkTableSchema",
        false);
    if (!valid)
    {
        m_logger->errorStream() << "The property table '" << m_tableName
                                << "' does not have the expected schema";
    }

    return valid;
}

} // namespace act::property
