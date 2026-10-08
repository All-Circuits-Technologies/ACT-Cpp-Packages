// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"
#include "act_foundation/services/abs_with_life_cycle.hpp"

namespace act::foundation
{

/**
 * @brief Services shared behavior
 *
 * @note A service is typically managed by a manager and should not be copied or moved.
 */
class AbsService : public AbsWithLifeCycle, private NotCopiableNotMovable
{
  public:
    /// @brief Nothing special for default destructor
    ~AbsService() override = default;

  protected:
    /// @brief Nothing special for default constructor
    AbsService() = default;
};

} // namespace act::foundation
