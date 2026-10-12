// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/stored_value.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>

namespace act::property
{

/**
 * @brief Tells how to read a native scalar type back from a @ref StoredValue.
 *
 * The primary template is intentionally left undefined: only the types specialized below are
 * native scalars. Each specialization binds one exact type to the accessor of its own type tag, so
 * a value is always read back with the width it was written with. Writing needs no trait:
 * @ref StoredValue has one constructor per exact type, selected by overload resolution.
 *
 * @tparam T The scalar type
 */
template <class T>
struct StoredScalarTraits;

/** @brief Read a @ref StoredType::BOOL value */
template <>
struct StoredScalarTraits<bool>
{
    [[nodiscard]] static std::optional<bool> Read(const StoredValue &raw)
    {
        return raw.asBool();
    }
};

/** @brief Read a @ref StoredType::INT8 value */
template <>
struct StoredScalarTraits<std::int8_t>
{
    [[nodiscard]] static std::optional<std::int8_t> Read(const StoredValue &raw)
    {
        return raw.asInt8();
    }
};

/** @brief Read a @ref StoredType::INT16 value */
template <>
struct StoredScalarTraits<std::int16_t>
{
    [[nodiscard]] static std::optional<std::int16_t> Read(const StoredValue &raw)
    {
        return raw.asInt16();
    }
};

/** @brief Read a @ref StoredType::INT32 value */
template <>
struct StoredScalarTraits<std::int32_t>
{
    [[nodiscard]] static std::optional<std::int32_t> Read(const StoredValue &raw)
    {
        return raw.asInt32();
    }
};

/** @brief Read a @ref StoredType::INT64 value */
template <>
struct StoredScalarTraits<std::int64_t>
{
    [[nodiscard]] static std::optional<std::int64_t> Read(const StoredValue &raw)
    {
        return raw.asInt64();
    }
};

/** @brief Read a @ref StoredType::UINT8 value */
template <>
struct StoredScalarTraits<std::uint8_t>
{
    [[nodiscard]] static std::optional<std::uint8_t> Read(const StoredValue &raw)
    {
        return raw.asUInt8();
    }
};

/** @brief Read a @ref StoredType::UINT16 value */
template <>
struct StoredScalarTraits<std::uint16_t>
{
    [[nodiscard]] static std::optional<std::uint16_t> Read(const StoredValue &raw)
    {
        return raw.asUInt16();
    }
};

/** @brief Read a @ref StoredType::UINT32 value */
template <>
struct StoredScalarTraits<std::uint32_t>
{
    [[nodiscard]] static std::optional<std::uint32_t> Read(const StoredValue &raw)
    {
        return raw.asUInt32();
    }
};

/** @brief Read a @ref StoredType::UINT64 value */
template <>
struct StoredScalarTraits<std::uint64_t>
{
    [[nodiscard]] static std::optional<std::uint64_t> Read(const StoredValue &raw)
    {
        return raw.asUInt64();
    }
};

/** @brief Read a @ref StoredType::FLOAT value */
template <>
struct StoredScalarTraits<float>
{
    [[nodiscard]] static std::optional<float> Read(const StoredValue &raw)
    {
        return raw.asFloat();
    }
};

/** @brief Read a @ref StoredType::DOUBLE value */
template <>
struct StoredScalarTraits<double>
{
    [[nodiscard]] static std::optional<double> Read(const StoredValue &raw)
    {
        return raw.asDouble();
    }
};

/** @brief Read a @ref StoredType::STRING value */
template <>
struct StoredScalarTraits<std::string>
{
    [[nodiscard]] static std::optional<std::string> Read(const StoredValue &raw)
    {
        return raw.asString();
    }
};

/**
 * @brief A type is a native scalar when @ref StoredScalarTraits knows how to read it back.
 *
 * @tparam T The candidate type
 */
template <class T>
concept StoredScalar = requires(const StoredValue raw) {
    { StoredScalarTraits<T>::Read(raw) } -> std::same_as<std::optional<T>>;
};

} // namespace act::property
