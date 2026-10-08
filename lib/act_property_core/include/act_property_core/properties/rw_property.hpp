// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/properties/r_property.hpp"

namespace act::property
{

/**
 * @brief A read and write descriptor.
 *
 * The write capability is carried by the type, so it is checked at compile time: a consumer given
 * only a `const RProperty<T> &` cannot write. It inherits every constructor of @ref RProperty.
 *
 * @note A seed goes with a read only descriptor: because a seed is realigned at each start, a
 *       runtime write of a seeded property would be overwritten at the next boot.
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
     * @return true on success, false otherwise
     */
    bool set(const T &value);

    /**
     * @brief Reset the value to its provider value, or remove the key when there is no provider.
     * @return true on success, false otherwise
     */
    bool reset();
};

template <typename T>
bool RWProperty<T>::set(const T &value)
{
    const auto &encoder = this->getEncoder();
    return this->accessStore().set(this->getKey(), encoder(value));
}

template <typename T>
bool RWProperty<T>::reset()
{
    const auto &fallback = this->getFallback();
    if (fallback.has_value())
    {
        return set(fallback->evaluate());
    }

    const auto &seed = this->getSeed();
    if (seed.has_value())
    {
        return set(seed->evaluate());
    }

    return this->accessStore().erase(this->getKey());
}

} // namespace act::property
