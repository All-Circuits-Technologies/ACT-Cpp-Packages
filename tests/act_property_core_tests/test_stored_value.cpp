// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_stored_value.cpp
 * @brief Unit tests of StoredValue.
 *
 * Covers the type tag given by each constructor, the accessor of that tag returning the payload
 * (limits included) while every other accessor returns nothing, and the equality by type tag and
 * payload.
 */

#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace act::property
{
namespace
{

    /** @brief Count the accessors of @p value which return a value */
    int CountReadableAccessors(const StoredValue &value)
    {
        int count = 0;
        count += value.asBool().has_value() ? 1 : 0;
        count += value.asInt8().has_value() ? 1 : 0;
        count += value.asInt16().has_value() ? 1 : 0;
        count += value.asInt32().has_value() ? 1 : 0;
        count += value.asInt64().has_value() ? 1 : 0;
        count += value.asUInt8().has_value() ? 1 : 0;
        count += value.asUInt16().has_value() ? 1 : 0;
        count += value.asUInt32().has_value() ? 1 : 0;
        count += value.asUInt64().has_value() ? 1 : 0;
        count += value.asFloat().has_value() ? 1 : 0;
        count += value.asDouble().has_value() ? 1 : 0;
        count += value.asString().has_value() ? 1 : 0;
        return count;
    }

    TEST(StoredValueTest, BoolIsTaggedAndReadAsBool)
    {
        const StoredValue value(true);

        EXPECT_EQ(value.type(), StoredType::BOOL);
        EXPECT_EQ(value.asBool(), std::optional<bool>(true));
        EXPECT_EQ(StoredValue(false).asBool(), std::optional<bool>(false));
        EXPECT_EQ(CountReadableAccessors(value), 1);
    }

    TEST(StoredValueTest, SignedIntegersAreTaggedWithTheirWidth)
    {
        const StoredValue int8(std::numeric_limits<std::int8_t>::min());
        const StoredValue int16(std::numeric_limits<std::int16_t>::min());
        const StoredValue int32(std::numeric_limits<std::int32_t>::min());
        const StoredValue int64(std::numeric_limits<std::int64_t>::min());

        EXPECT_EQ(int8.type(), StoredType::INT8);
        EXPECT_EQ(int16.type(), StoredType::INT16);
        EXPECT_EQ(int32.type(), StoredType::INT32);
        EXPECT_EQ(int64.type(), StoredType::INT64);

        EXPECT_EQ(int8.asInt8(), std::numeric_limits<std::int8_t>::min());
        EXPECT_EQ(int16.asInt16(), std::numeric_limits<std::int16_t>::min());
        EXPECT_EQ(int32.asInt32(), std::numeric_limits<std::int32_t>::min());
        EXPECT_EQ(int64.asInt64(), std::numeric_limits<std::int64_t>::min());

        EXPECT_EQ(CountReadableAccessors(int8), 1);
        EXPECT_EQ(CountReadableAccessors(int16), 1);
        EXPECT_EQ(CountReadableAccessors(int32), 1);
        EXPECT_EQ(CountReadableAccessors(int64), 1);
    }

    TEST(StoredValueTest, UnsignedIntegersAreTaggedWithTheirWidth)
    {
        const StoredValue uint8(std::numeric_limits<std::uint8_t>::max());
        const StoredValue uint16(std::numeric_limits<std::uint16_t>::max());
        const StoredValue uint32(std::numeric_limits<std::uint32_t>::max());
        const StoredValue uint64(std::numeric_limits<std::uint64_t>::max());

        EXPECT_EQ(uint8.type(), StoredType::UINT8);
        EXPECT_EQ(uint16.type(), StoredType::UINT16);
        EXPECT_EQ(uint32.type(), StoredType::UINT32);
        EXPECT_EQ(uint64.type(), StoredType::UINT64);

        EXPECT_EQ(uint8.asUInt8(), std::numeric_limits<std::uint8_t>::max());
        EXPECT_EQ(uint16.asUInt16(), std::numeric_limits<std::uint16_t>::max());
        EXPECT_EQ(uint32.asUInt32(), std::numeric_limits<std::uint32_t>::max());
        EXPECT_EQ(uint64.asUInt64(), std::numeric_limits<std::uint64_t>::max());

        EXPECT_EQ(CountReadableAccessors(uint8), 1);
        EXPECT_EQ(CountReadableAccessors(uint16), 1);
        EXPECT_EQ(CountReadableAccessors(uint32), 1);
        EXPECT_EQ(CountReadableAccessors(uint64), 1);
    }

    TEST(StoredValueTest, FloatingPointsAreTaggedWithTheirWidth)
    {
        constexpr float floatValue = 1.5F;
        constexpr double doubleValue = -2.25;
        const StoredValue floatStored(floatValue);
        const StoredValue doubleStored(doubleValue);

        EXPECT_EQ(floatStored.type(), StoredType::FLOAT);
        EXPECT_EQ(doubleStored.type(), StoredType::DOUBLE);
        EXPECT_EQ(floatStored.asFloat(), std::optional<float>(floatValue));
        EXPECT_EQ(doubleStored.asDouble(), std::optional<double>(doubleValue));
        EXPECT_EQ(floatStored.asDouble(), std::nullopt);
        EXPECT_EQ(doubleStored.asFloat(), std::nullopt);
        EXPECT_EQ(CountReadableAccessors(floatStored), 1);
        EXPECT_EQ(CountReadableAccessors(doubleStored), 1);
    }

    TEST(StoredValueTest, StringIsTaggedAndReadAsString)
    {
        const StoredValue value(std::string("text"));
        const StoredValue empty(std::string{});

        EXPECT_EQ(value.type(), StoredType::STRING);
        EXPECT_EQ(value.asString(), std::optional<std::string>("text"));
        EXPECT_EQ(empty.asString(), std::optional<std::string>(""));
        EXPECT_EQ(CountReadableAccessors(value), 1);
    }

    TEST(StoredValueTest, StringLiteralIsTaggedAndReadAsString)
    {
        const StoredValue value("text");

        EXPECT_EQ(value.type(), StoredType::STRING);
        EXPECT_EQ(value.asString(), std::optional<std::string>("text"));
        EXPECT_EQ(value, StoredValue(std::string("text")));
    }

    TEST(StoredValueTest, AccessorOfAnotherWidthReturnsNothing)
    {
        const StoredValue value(std::int32_t{1});

        EXPECT_EQ(value.asInt8(), std::nullopt);
        EXPECT_EQ(value.asInt16(), std::nullopt);
        EXPECT_EQ(value.asInt64(), std::nullopt);
        EXPECT_EQ(value.asUInt32(), std::nullopt);
        EXPECT_EQ(value.asBool(), std::nullopt);
        EXPECT_EQ(value.asString(), std::nullopt);
    }

    TEST(StoredValueTest, EqualValuesHaveTheSameTagAndPayload)
    {
        EXPECT_TRUE(StoredValue(std::int32_t{7}) == StoredValue(std::int32_t{7}));
        EXPECT_FALSE(StoredValue(std::int32_t{7}) != StoredValue(std::int32_t{7}));
        EXPECT_TRUE(StoredValue(std::string("a")) == StoredValue(std::string("a")));
    }

    TEST(StoredValueTest, NanPayloadsAreEqual)
    {
        const StoredValue floatNan(std::numeric_limits<float>::quiet_NaN());
        const StoredValue doubleNan(std::numeric_limits<double>::quiet_NaN());

        EXPECT_EQ(floatNan, StoredValue(std::numeric_limits<float>::quiet_NaN()));
        EXPECT_EQ(doubleNan, StoredValue(std::numeric_limits<double>::quiet_NaN()));
        EXPECT_NE(floatNan, doubleNan);
        EXPECT_NE(doubleNan, StoredValue(0.0));
    }

    TEST(StoredValueTest, DifferentPayloadsAreNotEqual)
    {
        EXPECT_FALSE(StoredValue(std::int32_t{7}) == StoredValue(std::int32_t{8}));
        EXPECT_TRUE(StoredValue(std::int32_t{7}) != StoredValue(std::int32_t{8}));
        EXPECT_TRUE(StoredValue(std::string("a")) != StoredValue(std::string("b")));
    }

    TEST(StoredValueTest, SameNumberWithAnotherTagIsNotEqual)
    {
        EXPECT_TRUE(StoredValue(std::int32_t{1}) != StoredValue(std::int64_t{1}));
        EXPECT_TRUE(StoredValue(std::int32_t{1}) != StoredValue(std::uint32_t{1}));
        EXPECT_TRUE(StoredValue(true) != StoredValue(std::uint8_t{1}));
        EXPECT_TRUE(StoredValue(1.0F) != StoredValue(1.0));
    }

    TEST(StoredValueTest, CopyKeepsTheTagAndThePayload)
    {
        const StoredValue original(std::string("copied"));
        const StoredValue copy = original; // NOLINT(performance-unnecessary-copy-initialization)

        EXPECT_EQ(copy.type(), StoredType::STRING);
        EXPECT_TRUE(copy == original);
    }

} // namespace
} // namespace act::property
