// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_db_sqlite/sqlite_db_manager.hpp"

#include "act_db_core/db_log_helper.hpp"
// NOLINTNEXTLINE: this include is actually needed
#include "act_db_core/services/abs_db_manager.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <sqlite3.h>
#include <string>
#include <utility>

namespace act::db::sqlite
{

ASqLiteDbManager::ASqLiteDbManager(std::filesystem::path dbFilePath,
                                   const std::string &dbSlug,
                                   const std::optional<std::filesystem::path> &migrationDataDir,
                                   const act::logger::LoggerManager &loggerManager)
    : act::db::core::AbsDbManager(dbSlug, migrationDataDir, loggerManager),
      m_dbFilePath(std::move(dbFilePath))
{
}

int ASqLiteDbManager::getMigrationVersion() const
{
    // Reading the schema version changes nothing, but the executor interface is not const
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<ASqLiteDbManager *>(this)->execAndGetInt("PRAGMA user_version;").value_or(0);
}

bool ASqLiteDbManager::setMigrationVersion(int version)
{
    FALSE_IF_NO_DB(setMigrationVersion, getLogger());

    FALSE_IF_THROW(setMigrationVersion,
                   getLogger(),
                   m_db->exec("PRAGMA user_version = " + std::to_string(version) + ";"));

    return true;
}

bool ASqLiteDbManager::defrag()
{
    const auto &logger = getLogger();
    FALSE_IF_NO_DB(defrag, logger);

    FALSE_IF_THROW(defrag, logger, m_db->exec("VACUUM;"));

    return true;
}

bool ASqLiteDbManager::sync()
{
    FALSE_IF_NO_DB(sync, getLogger());

    // TODO(aloiseau) Implement the logic to synchronize the SQLite database with the disk
    return false;
}

bool ASqLiteDbManager::exec(const std::string &sql)
{
    const auto &logger = getLogger();
    FALSE_IF_NO_DB(exec, logger);

    FALSE_IF_THROW(exec, logger, m_db->exec(sql));

    return true;
}

std::optional<int> ASqLiteDbManager::execAndGetInt(const std::string &sql)
{
    const auto &logger = getLogger();
    EMPTY_IF_NO_DB(execAndGetInt, logger);

    int result = 0;
    EMPTY_IF_THROW(execAndGetInt,
                   logger,
                   // May throw and Column has no default constructor to be moved up
                   const SQLite::Column column = m_db->execAndGet(sql);
                   result = column.getInt(););

    return result;
}

bool ASqLiteDbManager::setBusyTimeout(int busyTimeoutMs)
{
    const auto &logger = getLogger();
    FALSE_IF_NO_DB(setBusyTimeout, logger);

    FALSE_IF_THROW(setBusyTimeout, logger, m_db->setBusyTimeout(busyTimeoutMs));

    return true;
}

bool ASqLiteDbManager::openImpl()
{
    const auto &logger = getLogger();
    if (m_db)
    {
        logger->error("Database is already opened.");
        return false;
    }

    std::filesystem::create_directories(m_dbFilePath.parent_path());

    FALSE_IF_THROW(
        open,
        logger,
        m_db = std::make_shared<SQLite::Database>(m_dbFilePath,
                                                  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));

    /**
     * Enable REGEXP function for IPv4 validation and other regex operations
     * This provides a basic REGEXP implementation using C++ std::regex
     */
    FALSE_IF_THROW(open_regexp,
                   logger,
                   m_db->createFunction(
                       "REGEXP",
                       2,
                       true,
                       nullptr,
                       [](sqlite3_context *context, int /*argc*/, sqlite3_value **argv) {
                           try
                           {
                               // sqlite3's C API hands the arguments as an array and the
                               // text as unsigned char
                               // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast,
                               // cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               const char *pattern =
                                   reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
                               const char *text =
                                   reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
                               // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast,
                               // cppcoreguidelines-pro-bounds-pointer-arithmetic)

                               if (!pattern || !text)
                               {
                                   sqlite3_result_int(context, 0);
                                   return;
                               }

                               const std::regex regex_pattern(pattern);
                               const bool matches = std::regex_match(text, regex_pattern);
                               sqlite3_result_int(context, matches ? 1 : 0);
                           }
                           catch (const std::exception &)
                           {
                               sqlite3_result_int(context, 0);
                           }
                       }));

    return true;
}

} // namespace act::db::sqlite
