// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_enum_codec.cpp
 * @brief Unit tests of EnumCodec.
 *
 * An enumerator is stored as its name in a string value, an unknown enumerator as an empty name;
 * a known name decodes back to its enumerator, while an unknown name or a value which is not a
 * string decodes to nothing.
 */

#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

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

    /** @brief Codec knowing the first two locales only */
    EnumCodec<Locale> MakeLocaleCodec()
    {
        return EnumCodec<Locale>({{Locale::EN_GB, "en_GB"}, {Locale::FR_FR, "fr_FR"}});
    }

    TEST(EnumCodecTest, EncodeStoresTheNameAsAString)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        const StoredValue raw = codec.encode(Locale::FR_FR);

        EXPECT_EQ(raw.type(), StoredType::STRING);
        EXPECT_EQ(raw.asString(), std::optional<std::string>("fr_FR"));
    }

    TEST(EnumCodecTest, EncodeOfAnUnknownEnumeratorStoresAnEmptyName)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        EXPECT_EQ(codec.encode(Locale::DE_DE).asString(), std::optional<std::string>(""));
    }

    TEST(EnumCodecTest, DecodeGivesBackTheEnumeratorOfAKnownName)
    {
        const EnumCodec<Locale> codec = MakeLocaleCodec();

        EXPECT_EQ(codec.decode(StoredValue(std::string("en_GB"))),
                  std::optional<Locale>(Locale::EN_GB));
        EXPECT_EQ(codec.decode(codec.encode(Locale::FR_FR)), std::optional<Locale>(Locale::FR_FR));
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

    TEST(EnumCodecTest, EmptyCodecEncodesAndDecodesNothing)
    {
        const EnumCodec<Locale> codec;

        EXPECT_EQ(codec.encode(Locale::EN_GB).asString(), std::optional<std::string>(""));
        EXPECT_EQ(codec.decode(StoredValue(std::string("en_GB"))), std::nullopt);
    }

} // namespace
} // namespace act::property
