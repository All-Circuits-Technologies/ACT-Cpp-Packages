// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_stored_scalar_traits.cpp
 * @brief Unit tests of StoredScalarTraits and of the StoredScalar concept.
 *
 * Covers the closed set of native scalar types, the types left out of it (checked at compile
 * time), and each trait reading back only the type tag of its own exact type.
 */

#include "act_property_core/codecs/stored_scalar_traits.hpp"
#include "act_property_core/stored_value.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace act::property
{
namespace
{

    enum class Color
    {
        RED,
    };

    struct Point
    {
        int x;
        int y;
    };

    // The closed set of native scalars
    static_assert(StoredScalar<bool>);
    static_assert(StoredScalar<std::int8_t>);
    static_assert(StoredScalar<std::int16_t>);
    static_assert(StoredScalar<std::int32_t>);
    static_assert(StoredScalar<std::int64_t>);
    static_assert(StoredScalar<std::uint8_t>);
    static_assert(StoredScalar<std::uint16_t>);
    static_assert(StoredScalar<std::uint32_t>);
    static_assert(StoredScalar<std::uint64_t>);
    static_assert(StoredScalar<float>);
    static_assert(StoredScalar<double>);
    static_assert(StoredScalar<std::string>);

    // Everything else needs a dedicated codec
    static_assert(!StoredScalar<char>);
    static_assert(!StoredScalar<long double>);
    static_assert(!StoredScalar<const char *>);
    static_assert(!StoredScalar<std::string_view>);
    static_assert(!StoredScalar<Color>);
    static_assert(!StoredScalar<Point>);

    TEST(StoredScalarTraitsTest, ReadGivesTheValueOfItsOwnTag)
    {
        EXPECT_EQ(StoredScalarTraits<bool>::Read(StoredValue(true)), std::optional<bool>(true));
        EXPECT_EQ(StoredScalarTraits<std::int16_t>::Read(StoredValue(std::int16_t{-3})),
                  std::optional<std::int16_t>(-3));
        EXPECT_EQ(StoredScalarTraits<std::uint64_t>::Read(StoredValue(std::uint64_t{3})),
                  std::optional<std::uint64_t>(3));
        EXPECT_EQ(StoredScalarTraits<double>::Read(StoredValue(0.5)), std::optional<double>(0.5));
        EXPECT_EQ(StoredScalarTraits<std::string>::Read(StoredValue(std::string("s"))),
                  std::optional<std::string>("s"));
    }

    TEST(StoredScalarTraitsTest, ReadRejectsAnyOtherTag)
    {
        EXPECT_EQ(StoredScalarTraits<std::int16_t>::Read(StoredValue(std::int32_t{-3})),
                  std::nullopt);
        EXPECT_EQ(StoredScalarTraits<std::uint64_t>::Read(StoredValue(std::int64_t{3})),
                  std::nullopt);
        EXPECT_EQ(StoredScalarTraits<double>::Read(StoredValue(0.5F)), std::nullopt);
        EXPECT_EQ(StoredScalarTraits<bool>::Read(StoredValue(std::uint8_t{1})), std::nullopt);
        EXPECT_EQ(StoredScalarTraits<std::string>::Read(StoredValue(true)), std::nullopt);
    }

} // namespace
} // namespace act::property
