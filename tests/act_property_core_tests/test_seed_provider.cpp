// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_seed_provider.cpp
 * @brief Unit tests of SeedProvider and of its Seed factories.
 *
 * Covers the value type deduced by the factories (checked at compile time, together with the
 * absence of conversion between providers of different types), a constant seed, and a function
 * seed evaluated on each evaluation only.
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
    static_assert(std::is_same_v<decltype(Seed::of(1)), SeedProvider<int>>);
    static_assert(std::is_same_v<decltype(Seed::of(0.5F)), SeedProvider<float>>);
    static_assert(std::is_same_v<decltype(Seed::of<std::int64_t>(1)), SeedProvider<std::int64_t>>);
    static_assert(std::is_same_v<decltype(Seed::from([]() { return std::string("s"); })),
                                 SeedProvider<std::string>>);

    // A provider never converts to another value type, nor to a default
    static_assert(!std::is_convertible_v<SeedProvider<int>, SeedProvider<std::int64_t>>);
    static_assert(!std::is_convertible_v<SeedProvider<int>, DefaultProvider<int>>);

    TEST(SeedProviderTest, ConstantSeedEvaluatesToItsValue)
    {
        constexpr int seedValue = 3;
        const SeedProvider<int> provider = Seed::of(seedValue);

        EXPECT_EQ(provider.evaluate(), seedValue);
        EXPECT_EQ(provider.evaluate(), seedValue);
    }

    TEST(SeedProviderTest, FunctionSeedIsCalledOnEachEvaluationOnly)
    {
        int calls = 0;
        const SeedProvider<std::string> provider =
            Seed::from([&calls]() { return std::to_string(++calls); });

        EXPECT_EQ(calls, 0);
        EXPECT_EQ(provider.evaluate(), "1");
        EXPECT_EQ(provider.evaluate(), "2");
    }

} // namespace
} // namespace act::property
