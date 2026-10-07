// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_scalar_codec.cpp
 * @brief Unit tests of ScalarCodec.
 *
 * For every native scalar type: a value is encoded with the exact type tag of its type, decodes
 * back to itself (limits included), and a value carrying any other tag decodes to nothing, so
 * there is no implicit conversion between widths or signedness.
 */

#include "act_property_core/codecs/scalar_codec.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace act::property
{
namespace
{

    /** @brief Expected tag and sample values of one native scalar type */
    template <class T>
    struct ScalarSample;

    template <>
    struct ScalarSample<bool>
    {
        static constexpr StoredType TAG = StoredType::BOOL;

        static std::vector<bool> Values()
        {
            return {false, true};
        }
    };

    template <>
    struct ScalarSample<std::string>
    {
        static constexpr StoredType TAG = StoredType::STRING;

        static std::vector<std::string> Values()
        {
            return {"", "value", std::string("embedded\0nul", sizeof("embedded\0nul") - 1)};
        }
    };

    /** @brief Sample values of a numeric type: its limits, zero and one */
    template <class T, StoredType TagValue>
    struct NumericSample
    {
        static constexpr StoredType TAG = TagValue;

        static std::vector<T> Values();
    };

    template <class T, StoredType TagValue>
    std::vector<T> NumericSample<T, TagValue>::Values()
    {
        return {std::numeric_limits<T>::lowest(), T{0}, T{1}, std::numeric_limits<T>::max()};
    }

    template <>
    struct ScalarSample<std::int8_t> : NumericSample<std::int8_t, StoredType::INT8>
    {
    };

    template <>
    struct ScalarSample<std::int16_t> : NumericSample<std::int16_t, StoredType::INT16>
    {
    };

    template <>
    struct ScalarSample<std::int32_t> : NumericSample<std::int32_t, StoredType::INT32>
    {
    };

    template <>
    struct ScalarSample<std::int64_t> : NumericSample<std::int64_t, StoredType::INT64>
    {
    };

    template <>
    struct ScalarSample<std::uint8_t> : NumericSample<std::uint8_t, StoredType::UINT8>
    {
    };

    template <>
    struct ScalarSample<std::uint16_t> : NumericSample<std::uint16_t, StoredType::UINT16>
    {
    };

    template <>
    struct ScalarSample<std::uint32_t> : NumericSample<std::uint32_t, StoredType::UINT32>
    {
    };

    template <>
    struct ScalarSample<std::uint64_t> : NumericSample<std::uint64_t, StoredType::UINT64>
    {
    };

    template <>
    struct ScalarSample<float> : NumericSample<float, StoredType::FLOAT>
    {
    };

    template <>
    struct ScalarSample<double> : NumericSample<double, StoredType::DOUBLE>
    {
    };

    /** @brief One stored value of each tag, to check that only the codec's own tag decodes */
    std::vector<StoredValue> OneValuePerTag()
    {
        return {StoredValue(true),
                StoredValue(std::int8_t{1}),
                StoredValue(std::int16_t{1}),
                StoredValue(std::int32_t{1}),
                StoredValue(std::int64_t{1}),
                StoredValue(std::uint8_t{1}),
                StoredValue(std::uint16_t{1}),
                StoredValue(std::uint32_t{1}),
                StoredValue(std::uint64_t{1}),
                StoredValue(1.0F),
                StoredValue(1.0),
                StoredValue(std::string("1"))};
    }

    template <class T>
    class ScalarCodecTest : public ::testing::Test
    {
    };

    using ScalarTypes = ::testing::Types<bool,
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
                                         std::string>;

    TYPED_TEST_SUITE(ScalarCodecTest, ScalarTypes);

    TYPED_TEST(ScalarCodecTest, EncodeUsesTheTagOfTheExactType)
    {
        const ScalarCodec<TypeParam> codec;

        for (const auto &value : ScalarSample<TypeParam>::Values())
        {
            EXPECT_EQ(codec.encode(value).type(), ScalarSample<TypeParam>::TAG);
        }
    }

    TYPED_TEST(ScalarCodecTest, DecodeGivesBackTheEncodedValue)
    {
        const ScalarCodec<TypeParam> codec;

        for (const auto &value : ScalarSample<TypeParam>::Values())
        {
            EXPECT_EQ(codec.decode(codec.encode(value)),
                      std::optional<TypeParam>(TypeParam{value}));
        }
    }

    TYPED_TEST(ScalarCodecTest, DecodeRejectsEveryOtherTag)
    {
        const ScalarCodec<TypeParam> codec;

        for (const StoredValue &raw : OneValuePerTag())
        {
            if (raw.type() == ScalarSample<TypeParam>::TAG)
            {
                EXPECT_TRUE(codec.decode(raw).has_value());
            }
            else
            {
                EXPECT_EQ(codec.decode(raw), std::nullopt)
                    << "tag " << static_cast<int>(raw.type());
            }
        }
    }

} // namespace
} // namespace act::property
