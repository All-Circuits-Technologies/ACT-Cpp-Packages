// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_system/abs_system_critical_section.hpp"

#include <windows.h>

namespace act::system
{

/**
 * @brief Windows implementation of a multi-process-wide critical section.
 *
 * Uses a named Win32 mutex so that the lock is auto-released even in case of
 * app crash.
 */
class WinSystemCriticalSection : public AbsSystemCriticalSection
{
  public:
    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     */
    explicit WinSystemCriticalSection(const std::string &slug,
                                      const act::foundation::AbsLogger &logger)
        : WinSystemCriticalSection(slug.c_str(), logger)
    {
    }

    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     */
    explicit WinSystemCriticalSection(const char *slug, const act::foundation::AbsLogger &logger);

    /** @brief Close the Windows mutex handle */
    ~WinSystemCriticalSection() override;

  public:
    /** @brief Enter critical section, that is take the lock */
    [[nodiscard]] bool enter() const override;

    /** @brief Leave critical section, that is release the lock */
    [[nodiscard]] bool leave() const override;

  private:
    /** @brief Windows mutex handle */
    HANDLE m_handle{nullptr};
};

} // namespace act::system
