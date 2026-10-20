// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "act_db_core/db_transaction.hpp"
#include "act_db_core/services/abs_db_manager.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/services/abs_manager.hpp"

namespace act::db::core
{
/**
 * @brief Service to help protecting database queries with error handling and optional transactions
 */
template <typename DbManager = AbsDbManager>
class DbProtectService : public foundation::AbsManager
{
  public:
    /**
     * @brief Construct a new Db Protect Service object
     *
     * @param db Shared database manager to use for executing queries
     * @param parentLogger The parent logger to use for creating the service logger
     */
    explicit DbProtectService(DbManager &db, act::foundation::AbsLogger &parentLogger)
        : AbsManager(),
          m_db(db),
          m_logger(parentLogger.createAbsSubLogger(SUB_LOGGER_CATEGORY,
                                                   act::foundation::LogsLevel::Enum::TRACE))
    {
    }

    /** @brief Destructor */
    ~DbProtectService() override = default;

  public:
    /**
     * @brief Initialization
     * @return true if initialization was successful, false otherwise
     */
    bool init() override
    {
        return true;
    }

    /**
     * @brief Protect a database query with error handling and optional transaction
     * @note This method will handle database exceptions and log errors with the provided query
     * name. If useTransaction is true, the query will be executed within a transaction, which will
     * be rolled back in case of an error.
     * @param queryFunc The function containing the query to execute, taking a SQLite database
     * reference
     * @param queryName An optional name for the query, used in logging
     * @param useTransaction Whether to execute the query within a transaction (default: true)
     * @return True if the query executed successfully, false otherwise
     */
    bool protectQuery(const std::function<bool(DbManager &db)> &queryFunc,
                      const std::string &queryName = {},
                      bool useTransaction = true);

    /**
     * @brief Protect a database query with error handling and optional transaction
     * @note This method will handle database exceptions and log errors with the provided query
     * name. If useTransaction is true, the query will be executed within a transaction, which will
     * be rolled back in case of an error.
     * @param queryFunc The function containing the query to execute, taking a SQLite database
     * reference. If the function returns std::nullopt, it will be considered as a failure and the
     * transaction will be rolled back if useTransaction is true.
     * @param queryName An optional name for the query, used in logging
     * @param useTransaction Whether to execute the query within a transaction (default: true)
     * @return An optional containing the query result if the query executed successfully,
     * std::nullopt otherwise
     */
    template <typename T>
    std::optional<T> protectQueryWithResult(
        const std::function<std::optional<T>(DbManager &db)> &queryFunc,
        const std::string &queryName = {},
        bool useTransaction = true);

    /**
     * @brief Access the db manager
     * @return The database manager
     */
    [[nodiscard]] DbManager &accessDb() const
    {
        return m_db;
    }

    /**
     * @brief Get the db executor
     * @return The database executor
     */
    [[nodiscard]] const DbManager &getDb() const
    {
        return m_db;
    }

  private:
    /**
     * @brief Protect a database query with error handling and transaction
     * @note This method will handle database exceptions and log errors with the provided query
     * name.
     * @warning This method doesn't catch exceptions, caller should catch them if needed.
     * @param queryFunc The function containing the query to execute, taking a SQLite database
     * reference. If the function returns std::nullopt, it will be considered as a failure and the
     * transaction will be rolled back if useTransaction is true.
     * @param queryName An optional name for the query, used in logging
     * @return An optional containing the query result if the query executed successfully,
     * std::nullopt otherwise
     */
    template <typename T>
    std::optional<T> protectQueryWithTransactionNotSafe(
        const std::function<std::optional<T>(DbManager &db)> &queryFunc,
        const std::string &queryName);

  private:
    /** @brief Sub logger category for this service */
    static const constexpr char *SUB_LOGGER_CATEGORY = "protect";

  private:
    /** @brief Shared database handle */
    DbManager &m_db;

    /** @brief Logger helper */
    std::shared_ptr<act::foundation::AbsLogger> m_logger;
};

template <typename DbExecutor>
inline bool DbProtectService<DbExecutor>::protectQuery(
    const std::function<bool(DbExecutor &db)> &queryFunc,
    const std::string &queryName,
    bool useTransaction)
{
    auto queryResult = protectQueryWithResult<bool>(
        [&queryFunc](DbExecutor &db) -> std::optional<bool> {
            auto success = queryFunc(db);
            if (!success)
            {
                return std::nullopt;
            }

            return true;
        },
        queryName,
        useTransaction);
    return queryResult.has_value() && queryResult.value();
}

template <typename DbExecutor>
template <typename T>
inline std::optional<T> DbProtectService<DbExecutor>::protectQueryWithResult(
    const std::function<std::optional<T>(DbExecutor &db)> &queryFunc,
    const std::string &queryName,
    bool useTransaction)
{
    if (!m_db.isOpened())
    {
        m_logger->errorStream() << "Database is not opened, cannot perform operation: "
                                << queryName;
        return std::nullopt;
    }

    std::optional<T> result = std::nullopt;
    try
    {
        if (useTransaction)
        {
            result = protectQueryWithTransactionNotSafe(queryFunc, queryName);
        }
        else
        {
            result = queryFunc(m_db);
        }
    }
    catch (const std::exception &e)
    {
        m_logger->errorStream() << "Error occurred: " << e.what() << " upon " << queryName;
        result = std::nullopt;
    }

    return result;
}

template <typename DbExecutor>
template <typename T>
inline std::optional<T> DbProtectService<DbExecutor>::protectQueryWithTransactionNotSafe(
    const std::function<std::optional<T>(DbExecutor &db)> &queryFunc, const std::string &queryName)
{
    DbTransaction transaction(m_db, *m_logger);
    auto transactionResult = transaction.begin();
    if (!transactionResult)
    {
        m_logger->warningStream() << "Failed to begin transaction for query: " << queryName;
        return std::nullopt;
    }

    auto result = queryFunc(m_db);
    if (result.has_value())
    {
        transactionResult = transaction.commit();
    }
    else
    {
        transactionResult = transaction.rollback();
    }

    if (!transactionResult)
    {
        m_logger->warningStream() << "Failed to commit/rollback transaction for query: "
                                  << queryName;
        return std::nullopt;
    }

    return result;
}
} // namespace act::db::core
