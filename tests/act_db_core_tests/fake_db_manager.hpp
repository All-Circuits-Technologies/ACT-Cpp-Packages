// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file fake_db_manager.hpp
 * @brief Database manager test double which records the statements it is asked to execute.
 */

#pragma once

#include "act_db_core/services/abs_db_manager.hpp"

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace act::db::core::test
{

/**
 * @brief In-memory database manager: every executed statement is recorded, and chosen statements,
 * the opening or the version update can be made to fail.
 */
class FakeDbManager : public AbsDbManager
{
  public:
    FakeDbManager(const std::string &dbSlug,
                  const std::optional<std::filesystem::path> &migrationDataDir,
                  const act::logger::LoggerManager &loggerManager)
        : AbsDbManager(dbSlug, migrationDataDir, loggerManager)
    {
    }

    ~FakeDbManager() override = default;

  public:
    bool init() override
    {
        return true;
    }

    [[nodiscard]] bool isOpened() const override
    {
        return m_opened;
    }

    [[nodiscard]] int getMigrationVersion() const override
    {
        return m_migrationVersion;
    }

    bool setMigrationVersion(int version) override
    {
        if (m_failSetMigrationVersion)
        {
            return false;
        }

        m_migrationVersion = version;
        return true;
    }

    bool defrag() override
    {
        return true;
    }

    bool sync() override
    {
        return true;
    }

    bool exec(const std::string &sql) override
    {
        m_executed.push_back(sql);
        return !m_failingStatements.contains(sql);
    }

    std::optional<int> execAndGetInt(const std::string & /*sql*/) override
    {
        return std::nullopt;
    }

    bool setBusyTimeout(int /*busyTimeoutMs*/) override
    {
        return false;
    }

  public:
    /** @brief The statements given to @ref exec, in call order */
    [[nodiscard]] const std::vector<std::string> &getExecuted() const
    {
        return m_executed;
    }

    /** @brief Make @ref exec fail (after recording it) when given exactly this statement */
    void failStatement(std::string sql)
    {
        m_failingStatements.insert(std::move(sql));
    }

    /** @brief Make the opening fail */
    void setFailOpen(bool failOpen)
    {
        m_failOpen = failOpen;
    }

    /** @brief Make @ref setMigrationVersion fail */
    void setFailSetMigrationVersion(bool fail)
    {
        m_failSetMigrationVersion = fail;
    }

    /** @brief Force the migration version, as if the database had been migrated before */
    void forceMigrationVersion(int version)
    {
        m_migrationVersion = version;
    }

  protected:
    bool openImpl() override
    {
        if (m_failOpen)
        {
            return false;
        }

        m_opened = true;
        return true;
    }

  private:
    std::vector<std::string> m_executed;
    std::set<std::string> m_failingStatements;
    int m_migrationVersion{0};
    bool m_opened{false};
    bool m_failOpen{false};
    bool m_failSetMigrationVersion{false};
};

} // namespace act::db::core::test
