// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_default_provider.cpp
 * @brief Unit tests of DefaultProvider and of its Default factories.
 *
 * Covers the value type deduced by the factories (checked at compile time, together with the
 * absence of conversion between providers of different types), a constant default, and a function
 * default evaluated on each evaluation only.
 */

#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <type_traits>

namespace act::property
{
namespace
{

    // The deduced type is the exact type of the argument, or the one named explicitly
    static_assert(std::is_same_v<decltype(Default::of(1)), DefaultProvider<int>>);
    static_assert(std::is_same_v<decltype(Default::of(0.5)), DefaultProvider<double>>);
    static_assert(std::is_same_v<decltype(Default::of("text")), DefaultProvider<const char *>>);
    static_assert(
        std::is_same_v<decltype(Default::of<std::uint8_t>(1)), DefaultProvider<std::uint8_t>>);
    static_assert(std::is_same_v<decltype(Default::from([]() -> std::uint8_t { return 1; })),
                                 DefaultProvider<std::uint8_t>>);

    // A provider never converts to another value type, nor to a seed
    static_assert(!std::is_convertible_v<DefaultProvider<int>, DefaultProvider<std::uint8_t>>);
    static_assert(!std::is_convertible_v<DefaultProvider<double>, DefaultProvider<float>>);
    static_assert(
        !std::is_convertible_v<DefaultProvider<const char *>, DefaultProvider<std::string>>);
    static_assert(!std::is_convertible_v<DefaultProvider<int>, SeedProvider<int>>);

    TEST(DefaultProviderTest, ConstantDefaultEvaluatesToItsValue)
    {
        const DefaultProvider<std::string> provider = Default::of(std::string("fallback"));

        EXPECT_EQ(provider.evaluate(), "fallback");
        EXPECT_EQ(provider.evaluate(), "fallback");
    }

    TEST(DefaultProviderTest, FunctionDefaultIsCalledOnEachEvaluationOnly)
    {
        int calls = 0;
        const DefaultProvider<int> provider = Default::from([&calls]() { return ++calls; });

        EXPECT_EQ(calls, 0);
        EXPECT_EQ(provider.evaluate(), 1);
        EXPECT_EQ(provider.evaluate(), 2);
        EXPECT_EQ(calls, 2);
    }

} // namespace
} // namespace act::property
