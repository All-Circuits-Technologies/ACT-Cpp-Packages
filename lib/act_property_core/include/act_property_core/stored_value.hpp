// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/types/stored_type.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace act::property
{

/**
 * @brief A raw value handled by a store: one of a closed set of native types, carrying its type
 *        tag.
 *
 * Enums and complex objects are reduced to one of these native types (typically a JSON string) by
 * the codec of a descriptor; the store never sees anything else. The type tag enables the schema
 * drift guard: a value written long ago as an @ref StoredType::INT32 and read back by a descriptor
 * expecting a @ref StoredType::STRING is reported as a mismatch by the codec (which returns an
 * empty optional) rather than silently mis-converted.
 */
class StoredValue
{
  public:
    /** @brief Build a boolean value (tag @ref StoredType::BOOL) */
    explicit StoredValue(bool value);

    /** @brief Build an integer value (tag @ref StoredType::INT8) */
    explicit StoredValue(std::int8_t value);

    /** @brief Build an integer value (tag @ref StoredType::INT16) */
    explicit StoredValue(std::int16_t value);

    /** @brief Build an integer value (tag @ref StoredType::INT32) */
    explicit StoredValue(std::int32_t value);

    /** @brief Build an integer value (tag @ref StoredType::INT64) */
    explicit StoredValue(std::int64_t value);

    /** @brief Build an integer value (tag @ref StoredType::UINT8) */
    explicit StoredValue(std::uint8_t value);

    /** @brief Build an integer value (tag @ref StoredType::UINT16) */
    explicit StoredValue(std::uint16_t value);

    /** @brief Build an integer value (tag @ref StoredType::UINT32) */
    explicit StoredValue(std::uint32_t value);

    /** @brief Build an integer value (tag @ref StoredType::UINT64) */
    explicit StoredValue(std::uint64_t value);

    /** @brief Build a floating point value (tag @ref StoredType::FLOAT) */
    explicit StoredValue(float value);

    /** @brief Build a floating point value (tag @ref StoredType::DOUBLE) */
    explicit StoredValue(double value);

    /** @brief Build a string value (tag @ref StoredType::STRING) */
    explicit StoredValue(std::string value);

    /**
     * @brief Build a string value from a C string (tag @ref StoredType::STRING)
     * @note Without it, a string literal would convert to bool, a standard conversion preferred
     *       to the user-defined one to std::string, and be stored as a boolean
     * @param value The C string to store, which must not be null
     */
    explicit StoredValue(const char *value);

  public:
    /**
     * @brief Get the type tag of the stored value
     * @return The type tag
     */
    [[nodiscard]] StoredType type() const;

    /**
     * @brief Get the value as a boolean
     * @return The boolean value, or an empty optional if the tag is not @ref StoredType::BOOL
     */
    [[nodiscard]] std::optional<bool> asBool() const;

    /**
     * @brief Get the value as an 8bits integer
     * @return The 8bits integer value, or an empty optional if the tag is not @ref StoredType::INT8
     */
    [[nodiscard]] std::optional<std::int8_t> asInt8() const;

    /**
     * @brief Get the value as an 16bits integer
     * @return The 16bits integer value, or an empty optional if the tag is not
     * @ref StoredType::INT16
     */
    [[nodiscard]] std::optional<std::int16_t> asInt16() const;

    /**
     * @brief Get the value as an 32bits integer
     * @return The 32bits integer value, or an empty optional if the tag is not
     * @ref StoredType::INT32
     */
    [[nodiscard]] std::optional<std::int32_t> asInt32() const;

    /**
     * @brief Get the value as an 64bits integer
     * @return The 64bits integer value, or an empty optional if the tag is not
     * @ref StoredType::INT64
     */
    [[nodiscard]] std::optional<std::int64_t> asInt64() const;

    /**
     * @brief Get the value as an 8bits unsigned integer
     * @return The 8bits unsigned integer value, or an empty optional if the tag is not
     * @ref StoredType::UINT8
     */
    [[nodiscard]] std::optional<std::uint8_t> asUInt8() const;

    /**
     * @brief Get the value as an 16bits unsigned integer
     * @return The 16bits unsigned integer value, or an empty optional if the tag is not
     * @ref StoredType::UINT16
     */
    [[nodiscard]] std::optional<std::uint16_t> asUInt16() const;

    /**
     * @brief Get the value as an 32bits unsigned integer
     * @return The 32bits unsigned integer value, or an empty optional if the tag is not
     * @ref StoredType::UINT32
     */
    [[nodiscard]] std::optional<std::uint32_t> asUInt32() const;

    /**
     * @brief Get the value as an 64bits unsigned integer
     * @return The 64bits unsigned integer value, or an empty optional if the tag is not
     * @ref StoredType::UINT64
     */
    [[nodiscard]] std::optional<std::uint64_t> asUInt64() const;

    /**
     * @brief Get the value as a floating point
     * @return The floating point value, or an empty optional if the tag is not
     *         @ref StoredType::FLOAT
     */
    [[nodiscard]] std::optional<float> asFloat() const;

    /**
     * @brief Get the value as a floating point
     * @return The floating point value, or an empty optional if the tag is not
     *         @ref StoredType::DOUBLE
     */
    [[nodiscard]] std::optional<double> asDouble() const;

    /**
     * @brief Get the value as a string
     * @return The string value, or an empty optional if the tag is not @ref StoredType::STRING
     */
    [[nodiscard]] std::optional<std::string> asString() const;

    /**
     * @brief Equality by (type, payload)
     * @note Used to write only when the value actually differs, sparing flash wear.
     * @note Two NaN payloads are equal, although NaN never equals itself as a number: they hold
     *       the same stored content.
     * @param other The value to compare with
     * @return True if both the type tag and the payload are equal
     */
    [[nodiscard]] bool operator==(const StoredValue &other) const;

    /**
     * @brief Inequality by (type, payload)
     * @param other The value to compare with
     * @return True if either the type tag or the payload differ
     */
    [[nodiscard]] bool operator!=(const StoredValue &other) const;

  private:
    /** @brief The type tag of the payload */
    StoredType m_type;

    /** @brief The native payload, whose active alternative matches @ref m_type */
    std::variant<bool,
                 std::int8_t,
                 std::int16_t,
                 std::int32_t,
                 std::int64_t,
                 std::uint8_t,
                 std::uint16_t,
                 std::uint32_t,
                 std::uint64_t,
                 float,
                 double,
                 std::string>
        m_payload;
};

} // namespace act::property
