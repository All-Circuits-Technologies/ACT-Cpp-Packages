// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::foundation
{

/**
 * @brief Abstract class representing an entity with a life cycle.
 *
 * @note This class defines the interface for entities that have a life cycle, requiring them to
 *       implement the `init` method for initialization.
 */
class AbsWithLifeCycle
{
  public:
    /** @brief Nothing special for default destructor */
    virtual ~AbsWithLifeCycle() = default;

    /**
     * @brief Initialize the entity.
     *
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init() = 0;
};
} // namespace act::foundation
