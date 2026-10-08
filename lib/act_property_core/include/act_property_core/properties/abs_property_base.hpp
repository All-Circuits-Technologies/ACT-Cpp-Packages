// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"
#include "act_property_core/properties/abs_registered_property.hpp"

#include <string>

namespace act::property
{

class AbsPropertyRegistry;
class AbsPropertyStore;

/**
 * @brief The non-generic part common to every descriptor.
 *
 * It holds the key and the link to the owning registry. A descriptor registers itself with its
 * registry at construction, but never touches the store then; it reaches the store lazily through
 * @ref accessStore.
 *
 * The registry only knows the descriptor through the @ref AbsRegisteredProperty capability,
 * inherited as a protected base: the descriptor hands itself to the registry, which can then seed
 * every descriptor at init without knowing their value type, while code holding a reference to the
 * descriptor cannot trigger the seed materialization.
 */
class AbsPropertyBase : protected AbsRegisteredProperty,
                        private act::foundation::NotCopiableNotMovable
{

  protected:
    /**
     * @brief Construct the base and register it with its registry
     * @param registry The owning registry
     * @param key The key of this descriptor
     */
    AbsPropertyBase(AbsPropertyRegistry &registry, std::string key);

  public:
    /// @brief Nothing special for default destructor
    ~AbsPropertyBase() override = default;

  public:
    /** @copydoc AbsRegisteredProperty::getKey */
    [[nodiscard]] const std::string &getKey() const override
    {
        return m_key;
    }

  protected:
    /**
     * @brief Access the store provided by the registry.
     * @note Never called during construction, only at init and at access time.
     * @return The store
     */
    [[nodiscard]] AbsPropertyStore &accessStore() const;

  private:
    /** @brief The owning registry (not owned) */
    AbsPropertyRegistry &m_registry;

    /** @brief The key of this descriptor */
    const std::string m_key;
};

} // namespace act::property
