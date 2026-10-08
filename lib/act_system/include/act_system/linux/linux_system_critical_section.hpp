// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_system/abs_system_critical_section.hpp"

namespace act::system
{

/**
 * @brief Linux (POSIX) implementation of a multi-process-wide critical section.
 *
 * Uses flock on a lock file so that the lock is auto-released even in case of
 * app crash.
 */
class LinuxSystemCriticalSection : public AbsSystemCriticalSection
{
  public:
    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     */
    explicit LinuxSystemCriticalSection(const std::string &slug,
                                        const act::foundation::AbsLogger &logger)
        : LinuxSystemCriticalSection(slug.c_str(), logger)
    {
    }

    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     */
    explicit LinuxSystemCriticalSection(const char *slug, const act::foundation::AbsLogger &logger);

    /** @brief Release the lock file file descriptor */
    ~LinuxSystemCriticalSection() override;

  public:
    /** @brief Enter critical section, that is take the lock */
    [[nodiscard]] bool enter() const override;

    /** @brief Leave critical section, that is release the lock */
    [[nodiscard]] bool leave() const override;

  private:
    /** @brief Compute a lock file path given a slug
     * @param slug System-wide critical section slug / identifier
     * @return An absolute file path based on this slug
     */
    static std::string ComputeLockFilePath(const char *slug);

  private:
    /** @brief All processes can use the lock file */
    static const constexpr int LOCK_FILE_ACCESS_RIGHTS = 0666;

  private:
    /** @brief Lock file file descriptor */
    int m_fd;
};

} // namespace act::system
