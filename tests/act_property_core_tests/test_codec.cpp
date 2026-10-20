// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_codec.cpp
 * @brief Unit tests of the CodecFor concept.
 *
 * The concept is a compile-time contract, so most checks are static assertions: the shipped codecs
 * and a hand written one model it for their value type, while a codec for another type, or whose
 * members have the wrong signatures (an encode which cannot report a failure included), does not.
 * A runtime test then uses a hand written codec.
 */

#include "act_property_core/codecs/codec.hpp"
#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/codecs/scalar_codec.hpp"
#include "act_property_core/codecs/serialized_codec.hpp"
#include "act_property_core/stored_value.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

namespace act::property
{
namespace
{

    enum class Mode
    {
        OFF,
        ON,
    };

    struct Version
    {
        int major;
        int minor;
    };

    // A codec is used as an object, so its members stay non-static like those of a real codec
    // NOLINTBEGIN(readability-convert-member-functions-to-static)

    /** @brief Factor separating the major from the minor number in a packed version */
    constexpr std::int32_t VERSION_BASE = 1000;

    /** @brief Codec written without any base class: it only exposes the two members */
    struct VersionCodec
    {
        [[nodiscard]] std::optional<StoredValue> encode(const Version &value) const
        {
            return StoredValue{std::int32_t{(value.major * VERSION_BASE) + value.minor}};
        }

        [[nodiscard]] std::optional<Version> decode(const StoredValue &raw) const
        {
            const std::optional<std::int32_t> packed = raw.asInt32();
            if (!packed.has_value())
            {
                return std::nullopt;
            }

            return Version{*packed / VERSION_BASE, *packed % VERSION_BASE};
        }
    };

    /** @brief Only knows how to encode */
    struct EncodeOnlyCodec
    {
        [[nodiscard]] std::optional<StoredValue> encode(const int &value) const
        {
            return StoredValue{value};
        }
    };

    /** @brief Encodes into a stored value itself, so it cannot report an encode failure */
    struct NonOptionalEncodeCodec
    {
        [[nodiscard]] StoredValue encode(const int &value) const
        {
            return StoredValue{value};
        }

        [[nodiscard]] std::optional<int> decode(const StoredValue &raw) const
        {
            return raw.asInt32();
        }
    };

    /** @brief Decodes into the value type itself instead of an optional */
    struct NonOptionalDecodeCodec
    {
        [[nodiscard]] std::optional<StoredValue> encode(const int &value) const
        {
            return StoredValue{value};
        }

        [[nodiscard]] int decode(const StoredValue &raw) const
        {
            return raw.asInt32().value_or(0);
        }
    };

    /** @brief Encodes into a string instead of a stored value */
    struct StringEncodeCodec
    {
        [[nodiscard]] std::string encode(const int &value) const
        {
            return std::to_string(value);
        }

        [[nodiscard]] std::optional<int> decode(const StoredValue &raw) const
        {
            return raw.asInt32();
        }
    };

    /** @brief Members usable only on a mutable codec */
    struct NonConstCodec
    {
        std::optional<StoredValue> encode(const int &value)
        {
            return StoredValue{value};
        }

        std::optional<int> decode(const StoredValue &raw)
        {
            return raw.asInt32();
        }
    };

    // NOLINTEND(readability-convert-member-functions-to-static)

    static_assert(CodecFor<ScalarCodec<std::int32_t>, std::int32_t>);
    static_assert(CodecFor<ScalarCodec<std::string>, std::string>);
    static_assert(CodecFor<EnumCodec<Mode>, Mode>);
    static_assert(CodecFor<SerializedCodec<Version>, Version>);
    static_assert(CodecFor<VersionCodec, Version>);

    static_assert(!CodecFor<ScalarCodec<std::int32_t>, std::int64_t>);
    static_assert(!CodecFor<EnumCodec<Mode>, int>);
    static_assert(!CodecFor<VersionCodec, Mode>);
    static_assert(!CodecFor<EncodeOnlyCodec, int>);
    static_assert(!CodecFor<NonOptionalEncodeCodec, int>);
    static_assert(!CodecFor<NonOptionalDecodeCodec, int>);
    static_assert(!CodecFor<StringEncodeCodec, int>);
    static_assert(!CodecFor<NonConstCodec, int>);

    TEST(CodecTest, HandWrittenCodecRoundTrips)
    {
        const VersionCodec codec;
        constexpr Version version{2, 7};

        const std::optional<StoredValue> raw = codec.encode(version);
        ASSERT_TRUE(raw.has_value());

        const std::optional<Version> decoded = codec.decode(*raw);

        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(decoded->major, version.major);
        EXPECT_EQ(decoded->minor, version.minor);
        EXPECT_EQ(codec.decode(StoredValue(std::string("2.7"))), std::nullopt);
    }

} // namespace
} // namespace act::property
