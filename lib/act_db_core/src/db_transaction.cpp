// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_db_core/db_transaction.hpp"

#include "act_db_core/db_core_constants.hpp"
#include "act_db_core/services/abs_db_manager.hpp"
#include "act_foundation/logger/abs_logger.hpp"

namespace act::db::core
{
DbTransaction::DbTransaction(AbsDbManager &db, const act::foundation::AbsLogger &logger)
    : m_db(db),
      m_logger(logger),
      m_state{State::NOT_STARTED}
{
}

DbTransaction::~DbTransaction()
{
    rollbackIfNotCommitted();
}

bool DbTransaction::begin(std::string_view beginExtension)
{
    if (m_state == State::STARTED)
    {
        m_logger.warningStream() << "Transaction already started, cannot begin again: "
                                 << beginExtension;
        return false;
    }

    std::string query = DbCoreConstants::BEGIN_NAME;
    if (!beginExtension.empty())
    {
        query += " " + std::string(beginExtension);
    }

    auto success = m_db.exec(query);
    if (!success)
    {
        m_logger.warningStream() << "Failed to begin transaction " << beginExtension;
        return false;
    }

    m_state = State::STARTED;

    return true;
}

/** @brief Commit the transaction */
bool DbTransaction::commit()
{
    if (m_state == State::NOT_STARTED)
    {
        m_logger.warningStream() << "Transaction not started, cannot commit";
        return false;
    }

    if (m_state != State::STARTED)
    {
        m_logger.warning("Transaction already committed or rolled back, cannot commit again");
        return false;
    }

    auto success = m_db.exec(COMMIT_TRANSACTION);
    if (!success)
    {
        m_logger.warning("Failed to commit transaction");
        return false;
    }

    m_state = State::COMMITTED;

    return true;
}

/** @brief Rollback the transaction */
bool DbTransaction::rollback()
{
    if (m_state == State::NOT_STARTED)
    {
        m_logger.warningStream() << "Transaction not started, cannot roll back";
        return false;
    }

    if (m_state != State::STARTED)
    {
        m_logger.warning("Transaction already committed or rolled back, cannot rollback");
        return false;
    }

    auto success = m_db.exec(ROLLBACK_TRANSACTION);
    if (!success)
    {
        m_logger.warning("Failed to rollback transaction");
        return false;
    }

    m_state = State::ROLLED_BACK;

    return true;
}

bool DbTransaction::rollbackIfNotCommitted()
{
    if (m_state != State::STARTED)
    {
        // Nothing to do, transaction is not started or already committed/rolled back
        return true;
    }

    return rollback();
}

} // namespace act::db::core
