// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/stored_value.hpp"

#include <concepts>
#include <optional>

namespace act::property
{

/**
 * @brief The compile-time contract a codec must satisfy for a value type `T`.
 *
 * A codec converts between the application type `T` and a @ref StoredValue, with no virtual
 * dispatch: it is a concrete value type resolved at compile time. It is the only place that builds
 * or interprets a @ref StoredValue. Any type that exposes the two conversion members below models
 * the contract; there is no base class to inherit from.
 *
 * @tparam C The candidate codec type
 * @tparam T The application value type
 */
template <class C, class T>
concept CodecFor = requires(const C codec, const T value, const StoredValue raw) {
    { codec.encode(value) } -> std::same_as<StoredValue>;
    { codec.decode(raw) } -> std::same_as<std::optional<T>>;
};

} // namespace act::property
