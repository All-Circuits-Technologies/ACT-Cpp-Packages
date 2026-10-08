// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <string>

namespace act::property
{

/**
 * @brief What a registry needs from a registered descriptor, and nothing more: its key and the hook
 *        to materialize its seed.
 *
 * This is a pure capability interface, with no state. Its implementer is meant to inherit it as a
 * protected base: the implementer then hands itself to its registry at construction, while code
 * holding a reference to the descriptor cannot trigger the seed materialization.
 */
class AbsRegisteredProperty
{
  public:
    /** @brief Nothing special for default destructor */
    virtual ~AbsRegisteredProperty() = default;

    /**
     * @brief Get the key of the descriptor
     * @return The key
     */
    [[nodiscard]] virtual const std::string &getKey() const = 0;

    /**
     * @brief Materialize the seed of the descriptor into the store, if it carries one.
     * @note Called by the registry at init. It is a no-op for a default (or no provider)
     *       descriptor, and the realignment for a seeded one.
     * @return true when nothing had to be written or the write succeeded, false otherwise
     */
    virtual bool seedIntoStore() = 0;
};

} // namespace act::property
