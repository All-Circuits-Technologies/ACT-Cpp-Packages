// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_serialized_codec.cpp
 * @brief Unit tests of SerializedCodec and of its serializedCodec factory.
 *
 * An object is stored as the string its serializer produces; a string value is handed to the
 * deserializer, whose failure is reported as nothing, while a value which is not a string is
 * rejected without calling it.
 */

#include "act_property_core/codecs/serialized_codec.hpp"
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

    struct Contact
    {
        std::string firstName;
        std::string lastName;

        bool operator==(const Contact &other) const = default;
    };

    /** @brief Separator of the two fields in the serialized form */
    constexpr char SEPARATOR = ';';

    std::string Serialize(const Contact &contact)
    {
        return contact.firstName + SEPARATOR + contact.lastName;
    }

    std::optional<Contact> Parse(const std::string &serialized)
    {
        const std::string::size_type separator = serialized.find(SEPARATOR);
        if (separator == std::string::npos)
        {
            return std::nullopt;
        }

        return Contact{serialized.substr(0, separator), serialized.substr(separator + 1)};
    }

    TEST(SerializedCodecTest, EncodeStoresTheSerializedString)
    {
        const auto codec = serializedCodec<Contact>(&Serialize, &Parse);

        const StoredValue raw = codec.encode(Contact{"Ada", "Lovelace"});

        EXPECT_EQ(raw.type(), StoredType::STRING);
        EXPECT_EQ(raw.asString(), std::optional<std::string>("Ada;Lovelace"));
    }

    TEST(SerializedCodecTest, DecodeGivesBackTheEncodedObject)
    {
        const SerializedCodec<Contact> codec(&Serialize, &Parse);
        const Contact contact{"Alan", "Turing"};

        EXPECT_EQ(codec.decode(codec.encode(contact)), std::optional<Contact>(contact));
    }

    TEST(SerializedCodecTest, DeserializerFailureDecodesToNothing)
    {
        const auto codec = serializedCodec<Contact>(&Serialize, &Parse);

        EXPECT_EQ(codec.decode(StoredValue(std::string("no separator"))), std::nullopt);
    }

    TEST(SerializedCodecTest, ValueWhichIsNotAStringIsRejectedWithoutDeserializing)
    {
        int deserializeCalls = 0;
        const auto codec =
            serializedCodec<Contact>(&Serialize,
                                     [&deserializeCalls](const std::string &serialized) {
                                         ++deserializeCalls;
                                         return Parse(serialized);
                                     });

        EXPECT_EQ(codec.decode(StoredValue(std::int32_t{1})), std::nullopt);
        EXPECT_EQ(deserializeCalls, 0);

        EXPECT_TRUE(codec.decode(StoredValue(std::string("a;b"))).has_value());
        EXPECT_EQ(deserializeCalls, 1);
    }

} // namespace
} // namespace act::property
