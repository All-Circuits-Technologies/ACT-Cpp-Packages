// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "system_critical_section.hpp"

#include "act_foundation/not_copiable_not_movable.hpp"

#include <tuple>

namespace act::system
{

/** @brief Lock a critical section during entire object life type */
class SystemCriticalSectionGuard : private act::foundation::NotCopiableNotMovable
{
  public:
    /**  @brief Lock criticalSection until destructor
     * @param criticalSection Critical section to lock
     */
    explicit SystemCriticalSectionGuard(SystemCriticalSection &criticalSection)
        : m_criticalSection(criticalSection)
    {
        std::ignore = m_criticalSection.enter();
    }

    /** @brief Unlock critical section */
    ~SystemCriticalSectionGuard() override
    {
        std::ignore = m_criticalSection.leave();
    }

  private:
    /** @brief Critical section */
    SystemCriticalSection &m_criticalSection;
};

} // namespace act::system
