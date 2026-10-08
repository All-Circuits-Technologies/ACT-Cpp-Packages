// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_db_core/services/abs_db_manager.hpp"

#include "act_db_core/services/db_protect_service.hpp"
#include "act_logger/services/logger_manager.hpp"
#include "act_system/system_critical_section.hpp"
#include "act_system/system_critical_section_guard.hpp"

#include <fstream>
#include <iostream>

#ifndef _WIN32
#include <unistd.h> // sync
#endif

namespace act::db::core
{

AbsDbManager::AbsDbManager(const std::string &dbSlug,
                           const std::optional<std::filesystem::path> &migrationDataDir,
                           const act::logger::LoggerManager &loggerManager)
    : act::foundation::AbsManager(),
      m_dbSlug(dbSlug + "-db"),
      m_migrationDataDir(migrationDataDir),
      m_logger{loggerManager.createSubLogger(dbSlug + "-db")}
{
}

bool AbsDbManager::applyMigrationUpdates()
{
    act::system::SystemCriticalSection criticalSection(getDbSlug() + "-database-migration",
                                                       *m_logger);
    const act::system::SystemCriticalSectionGuard guard(criticalSection);
    DbProtectService protectService(*this, *m_logger);

    if (!m_migrationDataDir.has_value())
    {
        m_logger->error("Migration data directory is not set.");
        return false;
    }

    int currentVersion = getMigrationVersion();
    std::filesystem::path versionBumpScript = computeMigrationScriptPath(currentVersion);

    m_logger->infoStream() << "Current database version is " << currentVersion;
    m_logger->debugStream() << "Looking for migration scripts in " << m_migrationDataDir.value()
                            << ", starting with " << versionBumpScript.filename();

    bool allSucceed = true;
    int bumpCount = 0;
    while (allSucceed && std::filesystem::exists(versionBumpScript))
    {
        m_logger->debugStream() << "Applying migration script " << versionBumpScript.filename();

        allSucceed &= protectService.protectQuery(
            [&versionBumpScript, this](AbsDbManager &db) {
                return db.runScript(versionBumpScript);
            },
            "Migration script");

        if (!allSucceed)
        {
            m_logger->errorStream()
                << "Migration script " << versionBumpScript.filename() << " failed";
            return false;
        }

        allSucceed &= setMigrationVersion(currentVersion + 1);
        if (!allSucceed)
        {
            m_logger->errorStream()
                << "Failed to bump migration version to " << (currentVersion + 1);
            return false;
        }

#ifndef _WIN32
        (void)sync();
#endif

        currentVersion++;
        versionBumpScript = computeMigrationScriptPath(currentVersion);
        bumpCount++;
    }

    if (bumpCount == 0)
    {
        m_logger->debug("No migration found/required");
    }
    else
    {
        m_logger->debugStream() << "Successfully applied " << bumpCount << " migration scripts";
    }

    return allSucceed;
}

std::filesystem::path AbsDbManager::computeMigrationScriptPath(int currentVersion) const
{
    if (!m_migrationDataDir.has_value())
    {
        m_logger->error("Migration data directory is not set.");
        return {};
    }

    // Look like <slug>-v<currentVersion>-to-v<currentVersion+1>.sql
    const std::filesystem::path filename = getDbSlug() + "-v" + std::to_string(currentVersion) +
                                           "-to-v" + std::to_string(currentVersion + 1) + ".sql";
    return m_migrationDataDir.value() / filename;
}

bool AbsDbManager::runScript(const std::filesystem::path &scriptPath)
{
    std::ifstream scriptFile(scriptPath);

    if (!scriptFile)
    {
        m_logger->errorStream() << "Failed to open script file: " << scriptPath;
        return false;
    }

    const std::string sql((std::istreambuf_iterator<char>(scriptFile)),
                          std::istreambuf_iterator<char>());
    return exec(sql);
}

bool AbsDbManager::open(bool autoMigrate)
{
    bool allSucceed = true;

    m_logger->debug("Opening database");

    if (allSucceed)
    {
        allSucceed &= openImpl();
    }

    if (allSucceed && autoMigrate)
    {
        allSucceed &= applyMigrationUpdates();
    }

    return allSucceed;
}

} // namespace act::db::core
