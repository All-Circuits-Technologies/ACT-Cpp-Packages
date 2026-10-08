// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"
#include "act_foundation/services/abs_with_life_cycle.hpp"

namespace act::foundation
{

/**
 * @brief Managers shared behavior
 *
 * A manager is responsible for coordinating and controlling a set of related services or resources.
 * It's created and managed by a global manager.
 */
class AbsManager : public AbsWithLifeCycle, private NotCopiableNotMovable
{
  public:
    /// @brief Nothing special for default destructor
    ~AbsManager() override = default;

  protected:
    /// @brief Nothing special for default constructor
    AbsManager() = default;
};

} // namespace act::foundation
