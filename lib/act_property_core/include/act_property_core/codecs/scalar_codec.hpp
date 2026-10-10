// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/codecs/stored_scalar_traits.hpp"
#include "act_property_core/stored_value.hpp"

#include <optional>

namespace act::property
{

/**
 * @brief Codec for the native scalar types: bool, the fixed-width integers (signed and unsigned),
 *        float, double and std::string.
 *
 * A codec is a compile-time contract (no virtual dispatch): it converts between the application
 * type `T` and a @ref StoredValue. Each type is stored with its own exact type tag and read back
 * only under that tag, so there is no implicit conversion between widths. Only the exact types of
 * @ref StoredScalarTraits are accepted: a platform-dependent type (`long`, `size_t`, ...) compiles
 * only where it aliases one of the fixed-width types, so prefer the fixed-width names. Non-scalar
 * types (enums, complex objects) use a dedicated codec instead.
 *
 * @tparam T The scalar application type
 */
template <StoredScalar T>
struct ScalarCodec
{
    /**
     * @brief Encode a value into its native stored representation
     * @param value The value to encode
     * @return The stored value carrying the type tag of `T`
     */
    [[nodiscard]] StoredValue encode(const T &value) const;

    /**
     * @brief Decode a stored value back into the application type
     * @param raw The stored value to decode
     * @return The decoded value, or an empty optional if the type tag is not the one of `T`
     */
    [[nodiscard]] std::optional<T> decode(const StoredValue &raw) const;
};

template <StoredScalar T>
[[nodiscard]] StoredValue ScalarCodec<T>::encode(const T &value) const
{
    return StoredValue{value};
}

template <StoredScalar T>
[[nodiscard]] std::optional<T> ScalarCodec<T>::decode(const StoredValue &raw) const
{
    return StoredScalarTraits<T>::Read(raw);
}

} // namespace act::property
