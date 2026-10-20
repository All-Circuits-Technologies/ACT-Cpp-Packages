// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_enum_codec.cpp
 * @brief Unit tests of EnumCodec.
 *
 * An enumerator is stored as its name in a string value, while an enumerator missing from the
 * mapping fails to encode; a known name decodes back to its enumerator, while an unknown name or a
 * value which is not a string decodes to nothing. Static assertions check that the codec only
 * accepts an enumeration and cannot be built without a mapping.
 */

#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

namespace act::property
{
namespace
{

    enum class Locale
    {
        EN_GB,
        FR_FR,
        DE_DE,
    };

    /** @brief Tells whether EnumCodec accepts `T` as its value type */
    template <class T>
    concept EnumCodecAccepts = requires { typename EnumCodec<T>; };

    static_assert(EnumCodecAccepts<Locale>);
    static_assert(!EnumCodecAccepts<int>);
    static_assert(!EnumCodecAccepts<std::string>);
    static_assert(!std::is_default_constructible_v<EnumCodec<Locale>>);

    /** @brief Codec knowing the first two locales only */
    EnumCodec<Locale> MakeLocaleCodec()
    {
        return EnumCodec<Locale>({{Locale::EN_GB, "en_GB"}, {Locale::FR_FR, "fr_FR"}});
    }

    TEST(EnumCodecTest, EncodeStoresTheNameAsAString)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        const std::optional<StoredValue> raw = codec.encode(Locale::FR_FR);

        ASSERT_TRUE(raw.has_value());
        EXPECT_EQ(raw->type(), StoredType::STRING);
        EXPECT_EQ(raw->asString(), std::optional<std::string>("fr_FR"));
    }

    TEST(EnumCodecTest, EncodeOfAnEnumeratorMissingFromTheMappingFails)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        EXPECT_EQ(codec.encode(Locale::DE_DE), std::nullopt);
    }

    TEST(EnumCodecTest, DecodeGivesBackTheEnumeratorOfAKnownName)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        EXPECT_EQ(codec.decode(StoredValue(std::string("en_GB"))),
                  std::optional<Locale>(Locale::EN_GB));
        const std::optional<StoredValue> raw = codec.encode(Locale::FR_FR);
        ASSERT_TRUE(raw.has_value());
        EXPECT_EQ(codec.decode(*raw), std::optional<Locale>(Locale::FR_FR));
    }

    TEST(EnumCodecTest, DecodeOfAnUnknownNameGivesNothing)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        EXPECT_EQ(codec.decode(StoredValue(std::string("de_DE"))), std::nullopt);
        EXPECT_EQ(codec.decode(StoredValue(std::string("EN_GB"))), std::nullopt);
        EXPECT_EQ(codec.decode(StoredValue(std::string{})), std::nullopt);
    }

    TEST(EnumCodecTest, DecodeOfAValueWhichIsNotAStringGivesNothing)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        // The numeric value of an enumerator is never accepted, so reordering stays harmless
        EXPECT_EQ(codec.decode(StoredValue(std::int32_t{0})), std::nullopt);
        EXPECT_EQ(codec.decode(StoredValue(false)), std::nullopt);
    }

} // namespace
} // namespace act::property
