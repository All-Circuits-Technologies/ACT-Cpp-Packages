// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <string_view>

#include "act_db_core/db_core_constants.hpp"
#include "act_foundation/not_copiable_not_movable.hpp"

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::db::core
{
class AbsDbManager;

/**
 * @brief Helper class to handle database transactions
 * @note This class will automatically rollback the transaction if it is not committed when
 *       destructed, so it is recommended to use it with a local scope to ensure proper transaction
 *       handling.
 */
class DbTransaction : private act::foundation::NotCopiableNotMovable
{
  private:
    /**
     * @brief State of the transaction, to track if it has been started, committed or rolled back
     */
    enum class State
    {
        NOT_STARTED,
        STARTED,
        COMMITTED,
        ROLLED_BACK
    };

  public:
    /**
     * @brief Construct a new Db Transaction object
     *
     * @param db Shared database manager to use for executing transaction commands
     * @param logger The logger to use for logging transaction operations
     */
    explicit DbTransaction(AbsDbManager &db, const act::foundation::AbsLogger &logger);

    /** @brief Destructor */
    ~DbTransaction() override;

  public:
    /**
     * @brief Begin the transaction
     * @note If the transaction has been committed or rolled back, it can be started again.
     * @param beginExtension This is the extension to the BEGIN statement, which can be used to
     * specify the transaction type.
     * @return True if the transaction was successfully started, false otherwise
     */
    bool begin(std::string_view beginExtension = DbCoreConstants::TRANSACTION_NAME);

    /**
     * @brief Commit the transaction
     * @return True if the transaction was successfully committed, false otherwise
     */
    bool commit();

    /**
     * @brief Rollback the transaction
     * @return True if the transaction was successfully rolled back, false otherwise
     */
    bool rollback();

  private:
    /**
     * @brief Rollback the transaction if it has not been committed yet, to ensure proper
     * transaction handling
     * @return True if the transaction was successfully rolled back, committed or was not started,
     * false if the rollback failed
     */
    bool rollbackIfNotCommitted();

  private:
    /**
     * @brief Default SQL command to commit a transaction, using the standard "COMMIT TRANSACTION"
     */
    static constexpr const char *COMMIT_TRANSACTION = "COMMIT TRANSACTION";

    /**
     * @brief Default SQL command to rollback a transaction, using the standard "ROLLBACK
     * TRANSACTION"
     */
    static constexpr const char *ROLLBACK_TRANSACTION = "ROLLBACK TRANSACTION";

  private:
    /** @brief Shared database manager */
    AbsDbManager &m_db;

    /** @brief Logger for transaction operations */
    const act::foundation::AbsLogger &m_logger;

    /** @brief Flag to indicate if the transaction has been committed or rolled back */
    State m_state{State::NOT_STARTED};
};
} // namespace act::db::core
