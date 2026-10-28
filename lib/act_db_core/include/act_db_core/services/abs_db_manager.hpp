// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/services/abs_manager.hpp"

#include <filesystem>
#include <optional>

namespace act::logger
{
class LoggerHelper;
class LoggerManager;
} // namespace act::logger

namespace act::db::core
{
/**
 * @brief This abstract manager contains db-engine-agnostic shared code
 */
class AbsDbManager : public act::foundation::AbsManager
{
  public:
    /** @brief Create the manager
     * @param dbSlug The database slug used inside log traces and migrations
     *               scripts file names.
     *               Slug should be lowercase and hyphenated (e.g. "foo-bar"),
     *               without the "-db" suffix.
     * @param migrationDataDir The path to the migration data directory, if any
     * @param loggerManager The logger manager to use to create loggers
     */
    AbsDbManager(const std::string &dbSlug,
                 const std::optional<std::filesystem::path> &migrationDataDir,
                 const act::logger::LoggerManager &loggerManager);

    /// @brief Destructor
    ~AbsDbManager() override = default;

  public:
    /** @brief Open database
     * @param autoMigrate Should migration scripts if any be executed
     * @return True if the database was opened successfully, false otherwise
     */
    bool open(bool autoMigrate = true);

    /** @brief Tells if database is opened */
    [[nodiscard]] virtual bool isOpened() const = 0;

    /** @brief Apply migration upgrades if some updates exists
     * More or less equivalent to applying schema updates, but a migration may
     * also let schema unchanged and perform simple data insertions/fixes.
     * @return true if the migration was successful, false otherwise
     * @note Relies on @ref getMigrationVersion and @ref setMigrationVersion
     * @note Caller should better rely on @ref open with autoMigrate set to true
     */
    bool applyMigrationUpdates();

    /** @brief Get database current migration version
     * More or less equivalent to the schema version
     * @note First version is 0 and means that database is empty or do not exist
     * @return The current database version
     */
    [[nodiscard]] virtual int getMigrationVersion() const = 0;

    /** @brief Set the current migration version (likely schema verion)
     * @return The current database version
     */
    virtual bool setMigrationVersion(int version) = 0;

    /** @brief Defragment database and likely compress it a bit */
    virtual bool defrag() = 0;

    /** @brief Immediately synchronize the database state with the disk */
    virtual bool sync() = 0;

    /** @brief Script execution helper
     * @note Default implementation reads entire file and call @ref exec
     * @return True upon success, false otherwise
     */
    virtual bool runScript(const std::filesystem::path &scriptPath);

    /** @brief Execute a simple statement with no requets result
     * @param sql SQL query to execute, or PRAGMA query, etc
     * @return True if executed successfully, false otherwise
     */
    virtual bool exec(const std::string &sql) = 0;

    /** @brief Helper for queries returning a simple integer
     * @param sql Query to execute
     * @return The integer result (first column of first result row),
     *         or empty optional if error
     */
    virtual std::optional<int> execAndGetInt(const std::string &sql) = 0;

    /** @brief Change busy timeout.
     * Ask database to wait a bit if it is busy at the time of a request
     * A non-zero value, if supported, can greatly help shared database usage
     * @param busyTimeoutMs The busy timeout duration in milliseconds
     * @return True if applied, false if unsupported or unapplicable
     * @note Subclasses may want to call it from an overridden init.
     * @note Depending on implementation, database may/must be opened first
     */
    virtual bool setBusyTimeout(int busyTimeoutMs) = 0;

  protected:
    /** @brief Actually open the database */
    virtual bool openImpl() = 0;

    /** @brief Get database unique name in the "foo-bar-db" form
     * @return The database name, lower case with dashes is multi-word
     *         and with its final "-db" suffix added.
     */
    [[nodiscard]] const std::string &getDbSlug() const
    {
        return m_dbSlug;
    }

    /**
     * @brief Get the logger helper
     * @return The logger helper
     */
    [[nodiscard]] const std::shared_ptr<act::logger::LoggerHelper> &getLogger() const
    {
        return m_logger;
    }

  private:
    /** @brief Compute the migration script path for a given version
     * @param currentVersion The current database version
     * @return The path whare a script which migrates database from
     *         (currentVersion) to (currentVersion+1) should lie
     */
    [[nodiscard]] std::filesystem::path computeMigrationScriptPath(int currentVersion) const;

  private:
    /** @brief Database slug with added "-db" suffix */
    const std::string m_dbSlug;

    /** @brief The path to the migration data directory, if any */
    const std::optional<std::filesystem::path> m_migrationDataDir;

    /** @brief Logger helper */
    std::shared_ptr<act::logger::LoggerHelper> m_logger;
};

} // namespace act::db::core
