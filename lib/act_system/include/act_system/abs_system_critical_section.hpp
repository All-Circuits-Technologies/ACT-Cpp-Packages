// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::system
{

/**
 * @brief Abstract base for a multi-process-wide critical section.
 *
 * Concrete subclasses provide the OS-specific implementation.
 * @note Uses flock-based locking on POSIX so that the lock is auto-released
 *       even in case of app crash.
 */
class AbsSystemCriticalSection : private act::foundation::NotCopiableNotMovable
{
  public:
    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     * @note Created critical section is not auto-entered, you must enter it.
     */
    explicit AbsSystemCriticalSection(const std::string &slug,
                                      const act::foundation::AbsLogger &logger)
        : AbsSystemCriticalSection(slug.c_str(), logger)
    {
    }

    /**
     * @brief Create a system-wide critical section
     * @param slug Critical section identifier to reuse in other processes
     * @param logger Logger instance
     * @note Created critical section is not auto-entered, you must enter it.
     */
    explicit AbsSystemCriticalSection(const char * /*slug*/,
                                      const act::foundation::AbsLogger &logger)
        : m_logger(logger)
    {
    }

    /** @brief Disconnect from created system-wide critical section */
    ~AbsSystemCriticalSection() override = default;

  public:
    /** @brief Enter critical section, that is take the lock */
    [[nodiscard]] virtual bool enter() const = 0;

    /** @brief Leave critical section, that is release the lock */
    [[nodiscard]] virtual bool leave() const = 0;

  protected:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;
};

} // namespace act::system
