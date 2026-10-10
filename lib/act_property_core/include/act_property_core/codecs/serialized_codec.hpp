// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/stored_value.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace act::property
{

/**
 * @brief Codec that stores an arbitrary object as a single string.
 *
 * The core library holds no serialization dependency: the conversion to and from a string is
 * entirely supplied by the caller through two callables. Any encoding works (JSON, CSV, a hand
 * rolled format, ...) and any object type is supported, as long as it can be turned into a string
 * and back. The whole object is stored as one string value; an inconsistent stored string makes
 * the decode fail (empty optional), leaving it to the writer of the data to correct it. There is no
 * per-value versioning.
 *
 * @tparam T The application object type
 */
template <class T>
class SerializedCodec
{
  public:
    /** @brief Serializer: object -> string */
    using ToString = std::function<std::string(const T &)>;

    /** @brief Deserializer: string -> object, or an empty optional if the string is invalid */
    using FromString = std::function<std::optional<T>(const std::string &)>;

    /**
     * @brief Build a codec from a pair of conversion callables
     * @param toString The object -> string serializer
     * @param fromString The string -> object deserializer
     */
    SerializedCodec(ToString toString, FromString fromString)
        : m_toString(std::move(toString)),
          m_fromString(std::move(fromString))
    {
    }

  public:
    /**
     * @brief Encode an object into a string stored value
     * @param value The object to encode
     * @return A string stored value holding the serialized object
     */
    [[nodiscard]] StoredValue encode(const T &value) const;

    /**
     * @brief Decode a string stored value back into an object
     * @param raw The stored value to decode
     * @return The object, or an empty optional if the tag is not a string or the string is invalid
     */
    [[nodiscard]] std::optional<T> decode(const StoredValue &raw) const;

  private:
    /** @brief The object -> string serializer */
    ToString m_toString;

    /** @brief The string -> object deserializer */
    FromString m_fromString;
};

template <typename T>
[[nodiscard]] StoredValue SerializedCodec<T>::encode(const T &value) const
{
    return StoredValue{m_toString(value)};
}

template <typename T>
[[nodiscard]] std::optional<T> SerializedCodec<T>::decode(const StoredValue &raw) const
{
    const std::optional<std::string> serialized = raw.asString();
    if (!serialized.has_value())
    {
        return std::nullopt;
    }

    return m_fromString(*serialized);
}

/**
 * @brief Build a @ref SerializedCodec for `T` from two conversion callables
 * @tparam T The application object type
 * @param toString The object -> string serializer
 * @param fromString The string -> object deserializer
 * @return The codec
 */
template <class T>
[[nodiscard]] SerializedCodec<T> serializedCodec(typename SerializedCodec<T>::ToString toString,
                                                 typename SerializedCodec<T>::FromString fromString)
{
    return SerializedCodec<T>(std::move(toString), std::move(fromString));
}

} // namespace act::property
