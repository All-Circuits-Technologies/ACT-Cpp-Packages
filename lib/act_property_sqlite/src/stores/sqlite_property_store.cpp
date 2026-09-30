// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_sqlite/stores/sqlite_property_store.hpp"

#include "act_db_sqlite/sqlite_db_constants.hpp"
#include "act_db_sqlite/sqlite_db_manager.hpp"
#include "act_db_sqlite/sqlite_db_protect_service.hpp"
#include "act_foundation/logger/abs_logger.hpp"

#include <SQLiteCpp/SQLiteCpp.h>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
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

    /**
     * @brief Log that a column does not hold the storage class its reader expects
     * @param logger The logger to write to
     * @param column The column
     * @param expected The SQLite name of the expected storage class
     */
    void logStorageClassMismatch(const act::foundation::AbsLogger &logger,
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

        logger.warningStream() << "The '" << column.getName() << "' column holds storage class "
                               << actual << " where " << expected << " is expected";
    }

    /**
     * @brief Read an integer column into a value of the tagged type
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer or does not fit @p T
     */
    template <typename T>
    std::optional<StoredValue> readInteger(const act::foundation::AbsLogger &logger,
                                           const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(logger, column, IntegerStorageClass);
            return std::nullopt;
        }

        const std::int64_t raw = column.getInt64();
        if (!std::in_range<T>(raw))
        {
            logger.warningStream() << "The integer " << raw << " does not fit its tagged type";
            return std::nullopt;
        }

        return StoredValue(static_cast<T>(raw));
    }

    /**
     * @brief Read a boolean column
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer equal to 0 or 1
     */
    std::optional<StoredValue> readBool(const act::foundation::AbsLogger &logger,
                                        const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(logger, column, IntegerStorageClass);
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

        logger.warningStream() << "The boolean " << raw << " is neither "
                               << act::db::sqlite::SQLiteDbConstants::FALSE << " nor "
                               << act::db::sqlite::SQLiteDbConstants::TRUE;
        return std::nullopt;
    }

    /**
     * @brief Read an unsigned 64 bits integer column, stored as its signed reinterpretation
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not an integer
     */
    std::optional<StoredValue> readUInt64(const act::foundation::AbsLogger &logger,
                                          const SQLite::Column &column)
    {
        if (!column.isInteger())
        {
            logStorageClassMismatch(logger, column, IntegerStorageClass);
            return std::nullopt;
        }

        return StoredValue(std::bit_cast<std::uint64_t>(column.getInt64()));
    }

    /**
     * @brief Read a single precision floating point column
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not a real or a finite value is out
     * of the float range
     */
    std::optional<StoredValue> readFloat(const act::foundation::AbsLogger &logger,
                                         const SQLite::Column &column)
    {
        if (!column.isFloat())
        {
            logStorageClassMismatch(logger, column, RealStorageClass);
            return std::nullopt;
        }

        const double raw = column.getDouble();
        if (std::isfinite(raw) && (raw > static_cast<double>(std::numeric_limits<float>::max()) ||
                                   raw < static_cast<double>(std::numeric_limits<float>::lowest())))
        {
            logger.warningStream() << "The real " << raw << " is out of the float range";
            return std::nullopt;
        }

        return StoredValue(static_cast<float>(raw));
    }

    /**
     * @brief Read a double precision floating point column
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not a real
     */
    std::optional<StoredValue> readDouble(const act::foundation::AbsLogger &logger,
                                          const SQLite::Column &column)
    {
        if (!column.isFloat())
        {
            logStorageClassMismatch(logger, column, RealStorageClass);
            return std::nullopt;
        }

        return StoredValue(column.getDouble());
    }

    /**
     * @brief Read a string column
     * @param logger The logger explaining a rejected column
     * @param column The value column
     * @return The value, or an empty optional if the column is not a text
     */
    std::optional<StoredValue> readString(const act::foundation::AbsLogger &logger,
                                          const SQLite::Column &column)
    {
        if (!column.isText())
        {
            logStorageClassMismatch(logger, column, TextStorageClass);
            return std::nullopt;
        }

        return StoredValue(column.getString());
    }

    /**
     * @brief Rebuild a stored value from the columns of a row
     * @param logger The logger explaining a rejected row
     * @param typeColumn The type tag column
     * @param valueColumn The value column
     * @return The value, or an empty optional if the tag is unknown or the value does not match it
     */
    std::optional<StoredValue> readRow(const act::foundation::AbsLogger &logger,
                                       const SQLite::Column &typeColumn,
                                       const SQLite::Column &valueColumn)
    {
        if (!typeColumn.isInteger())
        {
            logStorageClassMismatch(logger, typeColumn, IntegerStorageClass);
            return std::nullopt;
        }

        const std::int64_t typeTag = typeColumn.getInt64();
        if (!std::in_range<int>(typeTag))
        {
            logger.warningStream() << "The type tag " << typeTag << " is out of the int range";
            return std::nullopt;
        }

        // The underlying type is fixed, so any int is a valid StoredType; unknown tags hit default
        switch (static_cast<StoredType>(static_cast<int>(typeTag)))
        {
            case StoredType::BOOL:
                return readBool(logger, valueColumn);
            case StoredType::INT8:
                return readInteger<std::int8_t>(logger, valueColumn);
            case StoredType::INT16:
                return readInteger<std::int16_t>(logger, valueColumn);
            case StoredType::INT32:
                return readInteger<std::int32_t>(logger, valueColumn);
            case StoredType::INT64:
                return readInteger<std::int64_t>(logger, valueColumn);
            case StoredType::UINT8:
                return readInteger<std::uint8_t>(logger, valueColumn);
            case StoredType::UINT16:
                return readInteger<std::uint16_t>(logger, valueColumn);
            case StoredType::UINT32:
                return readInteger<std::uint32_t>(logger, valueColumn);
            case StoredType::UINT64:
                return readUInt64(logger, valueColumn);
            case StoredType::FLOAT:
                return readFloat(logger, valueColumn);
            case StoredType::DOUBLE:
                return readDouble(logger, valueColumn);
            case StoredType::STRING:
                return readString(logger, valueColumn);
            default:
                logger.warningStream() << "The type tag " << typeTag << " is unknown";
                return std::nullopt;
        }
    }
} // namespace

SqlitePropertyStore::SqlitePropertyStore(act::db::sqlite::SQLiteDbProtectService &dbProtectService,
                                         act::foundation::AbsLogger &parentLogger,
                                         std::string tableName,
                                         TableProvisioning provisioning)
    : AbsPropertyStore(),
      m_dbProtectService(dbProtectService),
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
    switch (m_provisioning)
    {
        case TableProvisioning::SELF_CREATE:
            return createTable();
        case TableProvisioning::EXTERNAL:
            return checkTableExists();
    }

    return false;
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

            const SQLite::Column typeColumn = statement.getColumn(TypeResultIndex);
            auto value = readRow(*m_logger, typeColumn, statement.getColumn(ValueResultIndex));
            if (!value.has_value())
            {
                m_logger->warningStream() << "The row of key '" << key << "' in table '"
                                          << m_tableName << "' cannot be read back (type tag "
                                          << typeColumn.getString() << "), the key reads as absent";
            }

            return value;
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

} // namespace act::property
