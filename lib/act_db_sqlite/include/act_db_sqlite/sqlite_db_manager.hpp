// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_db_core/services/abs_db_manager.hpp"

#include <SQLiteCpp/Database.h>

#include <filesystem>
#include <memory>
#include <optional>

namespace act::logger
{
class LoggerManager;
} // namespace act::logger

namespace act::db::sqlite
{

/**
 * @brief This manager handles SQLite3 databases
 */
class ASqLiteDbManager : public act::db::core::AbsDbManager
{
  public:
    /** @brief Create the manager
     * @param dbFilePath The path to the SQLite database file to open/create
     * @param dbSlug The database slug used inside log traces.
     *               Slug should be lowercase and hyphenated (e.g. "foo-bar").
     * @param migrationDataDir The path to the migration data directory, if any
     * @param loggerManager The logger manager to use to create loggers
     */
    explicit ASqLiteDbManager(std::filesystem::path dbFilePath,
                              const std::string &dbSlug,
                              const std::optional<std::filesystem::path> &migrationDataDir,
                              const act::logger::LoggerManager &loggerManager);

    /** @brief Destructor */
    ~ASqLiteDbManager() override = default;

  public:
    /** @brief Tells if database is opened */
    [[nodiscard]] bool isOpened() const override
    {
        return m_db != nullptr;
    }

    /** @brief Get database current migration version
     * More or less equivalent to the schema version
     * @note First version is 0 and means that database is empty or do not exist
     * @return The current database version
     */
    [[nodiscard]] int getMigrationVersion() const override;

    /** @brief Set the current migration version (likely schema verion)
     * @return The current database version
     */
    bool setMigrationVersion(int version) override;

    /** @brief Defragment database and likely compress it a bit */
    bool defrag() override;

    /** @brief Immediately synchronize the database state with the disk */
    bool sync() override;

    /** @brief Execute a simple statement with no result
     * @param sql SQL query to execute, or PRAGMA query, etc
     * @return True if executed successfully, false otherwise
     */
    bool exec(const std::string &sql) override;

    /** @brief Execute a simple statement and return the integer result
     * @param sql SQL query to execute, or PRAGMA query, etc
     * @return The integer result (first column of first result row),
     *         or empty optional if error
     */
    std::optional<int> execAndGetInt(const std::string &sql) override;

    /** @brief Wait up to xxx milliseconds if database is busy
     * @param busyTimeoutMs The timeout duration in milliseconds
     * @return True if updated, false if unsupported or bad argument
     * @note Database must be opened first
     */
    bool setBusyTimeout(int busyTimeoutMs) override;

    /** @brief Get the SQLiteCpp database handle
     * @note Useful to process arbitrary queries and peek their results
     * @return The SQLiteCpp database handle
     */
    std::shared_ptr<SQLite::Database> getHandle()
    {
        return m_db;
    }

  protected:
    /** @brief Open the SQLite database */
    bool openImpl() override;

  private:
    /// @brief The path to the SQLite database file
    const std::filesystem::path m_dbFilePath;

    /// @brief Opened database
    std::shared_ptr<SQLite::Database> m_db;
};

} // namespace act::db::sqlite
