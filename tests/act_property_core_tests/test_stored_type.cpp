// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_stored_type.cpp
 * @brief Unit tests of StoredType.
 *
 * The underlying values of the type tags are written next to every stored value, so they are an
 * on-disk contract: these tests pin each of them.
 */

#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace act::property
{
namespace
{

    /** @brief Get the value written on disk for a type tag */
    constexpr int TagValue(StoredType type)
    {
        return static_cast<std::underlying_type_t<StoredType>>(type);
    }

    TEST(StoredTypeTest, UnderlyingTypeIsInt)
    {
        EXPECT_TRUE((std::is_same_v<std::underlying_type_t<StoredType>, int>));
    }

    TEST(StoredTypeTest, TagValuesAreTheOnDiskOnes)
    {
        EXPECT_EQ(TagValue(StoredType::BOOL), 0);
        EXPECT_EQ(TagValue(StoredType::INT8), 1);
        EXPECT_EQ(TagValue(StoredType::INT16), 2);
        EXPECT_EQ(TagValue(StoredType::INT32), 3);
        EXPECT_EQ(TagValue(StoredType::INT64), 4);
        EXPECT_EQ(TagValue(StoredType::UINT8), 5);
        EXPECT_EQ(TagValue(StoredType::UINT16), 6);
        EXPECT_EQ(TagValue(StoredType::UINT32), 7);
        EXPECT_EQ(TagValue(StoredType::UINT64), 8);
        EXPECT_EQ(TagValue(StoredType::FLOAT), 9);
        EXPECT_EQ(TagValue(StoredType::DOUBLE), 10);
        EXPECT_EQ(TagValue(StoredType::STRING), 11);
    }

} // namespace
} // namespace act::property
