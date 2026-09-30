// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::property
{

class AbsPropertyStore;
class AbsRegisteredProperty;

/**
 * @brief What a descriptor needs from its owner, and nothing more: a place to register itself and
 *        a way to reach the store.
 *
 * This is a pure capability interface, with no state. Its implementer is meant to inherit it as a
 * protected base: only the implementer and its derived classes can then convert themselves to this
 * interface, so descriptors can be declared as members of the owner, while code holding a reference
 * to the owner can neither reach the raw store nor register descriptors of its own.
 */
class AbsPropertyRegistry
{
  public:
    /** @brief Nothing special for default destructor */
    virtual ~AbsPropertyRegistry() = default;

    /**
     * @brief Register a descriptor.
     * @note Called by each descriptor at its construction. The registry does not own the
     *       descriptors; they are members of the owner and share its lifetime.
     * @param property The descriptor to register
     */
    virtual void registerProperty(AbsRegisteredProperty &property) = 0;

    /**
     * @brief Access the store the registered descriptors read from and write to.
     * @note Only called once the store exists (at init and at access time), never while the
     *       descriptors are being constructed.
     * @return The store
     */
    [[nodiscard]] virtual AbsPropertyStore &accessStore() = 0;
};

} // namespace act::property
