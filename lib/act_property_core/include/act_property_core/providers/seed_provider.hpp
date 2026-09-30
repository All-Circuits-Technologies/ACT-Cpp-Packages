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
 * @brief An init value for a property (the "code driven data" nature).
 *
 * A seed is written to the store at init and realigned on every start when the seed in the code has
 * changed, so that external readers (cloud, a CLI reading the table) always see the current value.
 * Because it is realigned at each start, a seeded property is meant to be read only at runtime: a
 * runtime write would be overwritten at the next start. Build it through the @ref Seed factory,
 * either from a constant (@ref Seed::of) or from a function (@ref Seed::from) that is evaluated
 * once at init, when the value is written to the store.
 *
 * @tparam T The application type of the value
 */
template <class T>
class SeedProvider
{
  public:
    /**
     * @brief Build a provider from a value producing function
     * @param valueFn The function producing the seed value
     */
    explicit SeedProvider(std::function<T()> valueFn)
        : m_valueFn(std::move(valueFn))
    {
    }

  public:
    /**
     * @brief Evaluate the seed value
     * @return The value produced by the wrapped function
     */
    [[nodiscard]] T evaluate() const;

  private:
    /** @brief The function producing the seed value */
    std::function<T()> m_valueFn;
};

template <typename T>
[[nodiscard]] T SeedProvider<T>::evaluate() const
{
    return m_valueFn();
}

/**
 * @brief Factories for @ref SeedProvider, deducing the value type from their argument, so that call
 *        sites read as `Seed::of(3)` / `Seed::from(&readSerial)`.
 *
 * The deduced type must be the exact value type of the property: a provider does not convert to
 * another value type. When the argument has another type (an `int` literal for a `std::uint8_t` or
 * `std::int64_t` property, a `double` literal for a `float`, a string literal for a `std::string`),
 * name the type explicitly: `Seed::of<std::uint8_t>(50)`. The conversion then happens at the call
 * site, where the compiler warns if a constant does not fit. For @ref Seed::from, give the
 * function the exact return type, e.g. `[]() -> std::uint8_t { return 50; }`.
 */
namespace Seed
{

    /**
     * @brief Build a seed from a constant value
     * @tparam T The value type, deduced from @p value or named explicitly
     * @param value The constant seed value
     * @return The provider
     */
    template <class T>
    [[nodiscard]] SeedProvider<T> of(T value)
    {
        return SeedProvider<T>([value = std::move(value)]() { return value; });
    }

    /**
     * @brief Build a seed from a value producing function, evaluated once at init
     * @tparam Fn The callable type, whose return type is the deduced value type
     * @param valueFn The function producing the seed value
     * @return The provider
     */
    template <class Fn>
    [[nodiscard]] auto from(Fn valueFn)
    {
        using T = std::invoke_result_t<Fn>;
        return SeedProvider<T>(std::function<T()>(std::move(valueFn)));
    }

} // namespace Seed

} // namespace act::property
