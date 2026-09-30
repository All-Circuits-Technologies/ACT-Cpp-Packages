// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace act::property
{

/**
 * @brief A read fallback value for a property (the "user data" nature).
 *
 * A default value is never written to the store: it only answers a read when the key is absent.
 * Runtime writes take precedence and are preserved across updates. Build it through the
 * @ref Default factory, either from a constant (@ref Default::of) or from a function
 * (@ref Default::from) that is evaluated lazily, only when a read finds the key missing (so it must
 * stay cheap).
 *
 * @tparam T The application type of the value
 */
template <class T>
class DefaultProvider
{
  public:
    /**
     * @brief Build a provider from a value producing function
     * @param valueFn The function producing the fallback value
     */
    explicit DefaultProvider(std::function<T()> valueFn)
        : m_valueFn(std::move(valueFn))
    {
    }

  public:
    /**
     * @brief Evaluate the fallback value
     * @return The value produced by the wrapped function
     */
    [[nodiscard]] T evaluate() const;

  private:
    /** @brief The function producing the fallback value */
    std::function<T()> m_valueFn;
};

template <typename T>
[[nodiscard]] T DefaultProvider<T>::evaluate() const
{
    return m_valueFn();
}

/**
 * @brief Factories for @ref DefaultProvider, deducing the value type from their argument, so that
 *        call sites read as `Default::of(50)` / `Default::from(&compute)`.
 *
 * The deduced type must be the exact value type of the property: a provider does not convert to
 * another value type. When the argument has another type (an `int` literal for a `std::uint8_t` or
 * `std::int64_t` property, a `double` literal for a `float`, a string literal for a `std::string`),
 * name the type explicitly: `Default::of<std::uint8_t>(50)`. The conversion then happens at the
 * call site, where the compiler warns if a constant does not fit. For @ref Default::from, give the
 * function the exact return type, e.g. `[]() -> std::uint8_t { return 50; }`.
 */
namespace Default
{

    /**
     * @brief Build a default from a constant value
     * @tparam T The value type, deduced from @p value or named explicitly
     * @param value The constant fallback value
     * @return The provider
     */
    template <class T>
    [[nodiscard]] DefaultProvider<T> of(T value)
    {
        return DefaultProvider<T>([value = std::move(value)]() { return value; });
    }

    /**
     * @brief Build a default from a value producing function, evaluated lazily at read time
     * @tparam Fn The callable type, whose return type is the deduced value type
     * @param valueFn The function producing the fallback value
     * @return The provider
     */
    template <class Fn>
    [[nodiscard]] auto from(Fn valueFn)
    {
        using T = std::invoke_result_t<Fn>;
        return DefaultProvider<T>(std::function<T()>(std::move(valueFn)));
    }

} // namespace Default

} // namespace act::property
