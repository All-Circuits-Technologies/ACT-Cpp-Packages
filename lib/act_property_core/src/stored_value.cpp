// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_core/stored_value.hpp"

#include <cmath>
#include <utility>
#include <variant>

namespace act::property
{

StoredValue::StoredValue(bool value)
    : m_type(StoredType::BOOL),
      m_payload(value)
{
}

StoredValue::StoredValue(std::int8_t value)
    : m_type(StoredType::INT8),
      m_payload(value)
{
}

StoredValue::StoredValue(std::int16_t value)
    : m_type(StoredType::INT16),
      m_payload(value)
{
}

StoredValue::StoredValue(std::int32_t value)
    : m_type(StoredType::INT32),
      m_payload(value)
{
}

StoredValue::StoredValue(std::int64_t value)
    : m_type(StoredType::INT64),
      m_payload(value)
{
}

StoredValue::StoredValue(std::uint8_t value)
    : m_type(StoredType::UINT8),
      m_payload(value)
{
}

StoredValue::StoredValue(std::uint16_t value)
    : m_type(StoredType::UINT16),
      m_payload(value)
{
}

StoredValue::StoredValue(std::uint32_t value)
    : m_type(StoredType::UINT32),
      m_payload(value)
{
}

StoredValue::StoredValue(std::uint64_t value)
    : m_type(StoredType::UINT64),
      m_payload(value)
{
}

StoredValue::StoredValue(float value)
    : m_type(StoredType::FLOAT),
      m_payload(value)
{
}

StoredValue::StoredValue(double value)
    : m_type(StoredType::DOUBLE),
      m_payload(value)
{
}

StoredValue::StoredValue(std::string value)
    : m_type(StoredType::STRING),
      m_payload(std::move(value))
{
}

StoredValue::StoredValue(const char *value)
    : StoredValue(std::string(value))
{
}

StoredType StoredValue::type() const
{
    return m_type;
}

std::optional<bool> StoredValue::asBool() const
{
    if (m_type != StoredType::BOOL)
    {
        return std::nullopt;
    }

    return std::get<bool>(m_payload);
}

std::optional<std::int8_t> StoredValue::asInt8() const
{
    if (m_type != StoredType::INT8)
    {
        return std::nullopt;
    }

    return std::get<std::int8_t>(m_payload);
}

std::optional<std::int16_t> StoredValue::asInt16() const
{
    if (m_type != StoredType::INT16)
    {
        return std::nullopt;
    }

    return std::get<std::int16_t>(m_payload);
}

std::optional<std::int32_t> StoredValue::asInt32() const
{
    if (m_type != StoredType::INT32)
    {
        return std::nullopt;
    }

    return std::get<std::int32_t>(m_payload);
}

std::optional<std::int64_t> StoredValue::asInt64() const
{
    if (m_type != StoredType::INT64)
    {
        return std::nullopt;
    }

    return std::get<std::int64_t>(m_payload);
}

std::optional<std::uint8_t> StoredValue::asUInt8() const
{
    if (m_type != StoredType::UINT8)
    {
        return std::nullopt;
    }

    return std::get<std::uint8_t>(m_payload);
}

std::optional<std::uint16_t> StoredValue::asUInt16() const
{
    if (m_type != StoredType::UINT16)
    {
        return std::nullopt;
    }

    return std::get<std::uint16_t>(m_payload);
}

std::optional<std::uint32_t> StoredValue::asUInt32() const
{
    if (m_type != StoredType::UINT32)
    {
        return std::nullopt;
    }

    return std::get<std::uint32_t>(m_payload);
}

std::optional<std::uint64_t> StoredValue::asUInt64() const
{
    if (m_type != StoredType::UINT64)
    {
        return std::nullopt;
    }

    return std::get<std::uint64_t>(m_payload);
}

std::optional<float> StoredValue::asFloat() const
{
    if (m_type != StoredType::FLOAT)
    {
        return std::nullopt;
    }

    return std::get<float>(m_payload);
}

std::optional<double> StoredValue::asDouble() const
{
    if (m_type != StoredType::DOUBLE)
    {
        return std::nullopt;
    }

    return std::get<double>(m_payload);
}

std::optional<std::string> StoredValue::asString() const
{
    if (m_type != StoredType::STRING)
    {
        return std::nullopt;
    }

    return std::get<std::string>(m_payload);
}

bool StoredValue::operator==(const StoredValue &other) const
{
    if (m_type != other.m_type)
    {
        return false;
    }

    // NaN never equals itself as a number, but two NaN payloads hold the same stored content
    if (m_type == StoredType::FLOAT && std::isnan(std::get<float>(m_payload)) &&
        std::isnan(std::get<float>(other.m_payload)))
    {
        return true;
    }
    if (m_type == StoredType::DOUBLE && std::isnan(std::get<double>(m_payload)) &&
        std::isnan(std::get<double>(other.m_payload)))
    {
        return true;
    }

    return m_payload == other.m_payload;
}

bool StoredValue::operator!=(const StoredValue &other) const
{
    return !(*this == other);
}

} // namespace act::property
