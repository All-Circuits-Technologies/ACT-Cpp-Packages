// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/stored_value.hpp"

#include <optional>

namespace act::property
{

/**
 * @brief A read and write descriptor.
 *
 * The write capability is carried by the type, so it is checked at compile time: a consumer given
 * only a `const RProperty<T> &` cannot write. It inherits every constructor of @ref RProperty.
 *
 * @note A seeded read and write descriptor holds a temporary override: a runtime write lasts for
 *       the lifetime of the application, and the seed is realigned at the next start. Each write
 *       still reaches the store, so it costs a flash write and external readers see the override
 *       until then.
 *
 * @tparam T The application type of the value
 */
template <class T>
class RWProperty : public RProperty<T>
{
  public:
    using RProperty<T>::RProperty;

  public:
    /**
     * @brief Write the value into the store (upsert)
     * @param value The value to store
     * @return true on success, false if the codec cannot encode @p value or the write failed
     */
    bool set(const T &value);

    /**
     * @brief Reset the value: write the seed back if there is one, remove the key otherwise.
     * @note Removing the key lets a read fall back on the default, if any, as the default itself is
     *       never written.
     * @return true on success, false otherwise
     */
    bool reset();
};

template <typename T>
bool RWProperty<T>::set(const T &value)
{
    const std::optional<StoredValue> encoded = this->encode(value);
    if (!encoded.has_value())
    {
        return false;
    }

    return this->accessStore().set(this->getKey(), *encoded);
}

template <typename T>
bool RWProperty<T>::reset()
{
    const auto &seed = this->getSeed();
    if (seed.has_value())
    {
        return set(seed->evaluate());
    }

    return this->accessStore().erase(this->getKey());
}

} // namespace act::property
