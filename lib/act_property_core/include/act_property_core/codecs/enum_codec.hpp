// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/stored_value.hpp"

#include <initializer_list>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace act::property
{

/**
 * @brief Codec for an enumeration, stored by its name (a string).
 *
 * The codec is built from a value <-> name mapping. Storing the name rather than the numeric value
 * keeps the stored data readable and, above all, robust to reordering of the enumerators: an
 * unknown name (or a stored value whose tag is not a string) decodes to an empty optional, so a
 * corrupted or outdated entry reads as absent instead of crashing or yielding a wrong value. An
 * enumerator missing from the mapping fails to encode, so writing it fails instead of storing a
 * name that could never be read back.
 *
 * @tparam E The enumeration type
 */
template <class E>
    requires std::is_enum_v<E>
class EnumCodec
{
  public:
    /** @brief One value <-> name association */
    using Mapping = std::pair<E, std::string>;

    /**
     * @brief Build a codec from a value <-> name mapping
     * @param mappings The list of value/name associations
     */
    EnumCodec(std::initializer_list<Mapping> mappings)
        : m_mappings(mappings)
    {
    }

  public:
    /**
     * @brief Encode an enumerator into its stored name
     * @param value The enumerator to encode
     * @return A string stored value holding the name, or an empty optional if the enumerator is
     *         missing from the mapping
     */
    [[nodiscard]] std::optional<StoredValue> encode(E value) const;

    /**
     * @brief Decode a stored name back into its enumerator
     * @param raw The stored value to decode
     * @return The enumerator, or an empty optional if the tag is not a string or the name is
     *         unknown
     */
    [[nodiscard]] std::optional<E> decode(const StoredValue &raw) const;

  private:
    /** @brief The value <-> name associations */
    std::vector<Mapping> m_mappings;
};

template <typename E>
    requires std::is_enum_v<E>
[[nodiscard]] std::optional<StoredValue> EnumCodec<E>::encode(E value) const
{
    for (const Mapping &mapping : m_mappings)
    {
        if (mapping.first == value)
        {
            return StoredValue{mapping.second};
        }
    }

    return std::nullopt;
}

template <typename E>
    requires std::is_enum_v<E>
[[nodiscard]] std::optional<E> EnumCodec<E>::decode(const StoredValue &raw) const
{
    const std::optional<std::string> name = raw.asString();
    if (!name.has_value())
    {
        return std::nullopt;
    }

    for (const Mapping &mapping : m_mappings)
    {
        if (mapping.second == *name)
        {
            return mapping.first;
        }
    }

    return std::nullopt;
}

} // namespace act::property
