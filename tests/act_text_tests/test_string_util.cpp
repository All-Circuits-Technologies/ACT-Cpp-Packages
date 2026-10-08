// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_string_util.cpp
 * @brief Unit tests of StringUtil.
 *
 * Covers trimming (in place and copying), case conversion and case insensitive comparison, the
 * iterable to string conversion, the hexadecimal conversions and checks, and the IPv4 address
 * validation.
 */

#include "act_text/string_util.hpp"

#include <gtest/gtest.h>

#include <list>
#include <optional>
#include <string>
#include <vector>

namespace act::text::StringUtil
{
namespace
{

    TEST(StringUtilTest, TrimRemovesLeadingAndTrailingWhitespace)
    {
        std::string value = " \t\n value with inner spaces \r\n";

        std::string &result = Trim(value);

        EXPECT_EQ(value, "value with inner spaces");
        EXPECT_EQ(&result, &value);
    }

    TEST(StringUtilTest, LtrimRemovesOnlyLeadingWhitespace)
    {
        std::string value = "  value  ";

        Ltrim(value);

        EXPECT_EQ(value, "value  ");
    }

    TEST(StringUtilTest, RtrimRemovesOnlyTrailingWhitespace)
    {
        std::string value = "  value  ";

        Rtrim(value);

        EXPECT_EQ(value, "  value");
    }

    TEST(StringUtilTest, TrimOfOnlyWhitespaceGivesAnEmptyString)
    {
        std::string value = " \t \n ";

        Trim(value);

        EXPECT_TRUE(value.empty());
    }

    TEST(StringUtilTest, TrimCopiesLeaveTheSourceUntouched)
    {
        const std::string source = "  value  ";

        EXPECT_EQ(LtrimCopy(source), "value  ");
        EXPECT_EQ(RtrimCopy(source), "  value");
        EXPECT_EQ(TrimCopy(source), "value");
        EXPECT_EQ(source, "  value  ");
    }

    TEST(StringUtilTest, ToUpperConvertsOnlyLetters)
    {
        EXPECT_EQ(ToUpper("Hello, World 42!"), "HELLO, WORLD 42!");
        EXPECT_EQ(ToUpper(""), "");
    }

    TEST(StringUtilTest, ToLowerConvertsOnlyLetters)
    {
        EXPECT_EQ(ToLower("Hello, World 42!"), "hello, world 42!");
        EXPECT_EQ(ToLower(""), "");
    }

    TEST(StringUtilTest, InPlaceCaseConversionsAlterTheString)
    {
        std::string value = "MiXeD";

        ToUpperInPlace(value);
        EXPECT_EQ(value, "MIXED");

        ToLowerInPlace(value);
        EXPECT_EQ(value, "mixed");
    }

    TEST(StringUtilTest, CompareNoCaseIgnoresCase)
    {
        EXPECT_EQ(CompareNoCase("Value", "vALUE"), 0);
        EXPECT_LT(CompareNoCase("apple", "BANANA"), 0);
        EXPECT_GT(CompareNoCase("Banana", "apple"), 0);
        EXPECT_LT(CompareNoCase("abc", "ABCD"), 0);
    }

    TEST(StringUtilTest, AreStringEqualNoCaseIgnoresCase)
    {
        EXPECT_TRUE(AreStringEqualNoCase("Value", "vALUE"));
        EXPECT_TRUE(AreStringEqualNoCase("", ""));
        EXPECT_FALSE(AreStringEqualNoCase("value", "values"));
        EXPECT_FALSE(AreStringEqualNoCase("value", "valve"));
    }

    TEST(StringUtilTest, IterableToStringJoinsElementsWithTheDelimiter)
    {
        const std::vector<int> numbers{1, 2, 3};
        const std::list<std::string> words{"a", "b"};

        EXPECT_EQ(IterableToString(numbers), "1,2,3");
        EXPECT_EQ(IterableToString(numbers, " - "), "1 - 2 - 3");
        EXPECT_EQ(IterableToString(words, ""), "ab");
    }

    TEST(StringUtilTest, IterableToStringOfEmptyOrSingleElementHasNoDelimiter)
    {
        EXPECT_EQ(IterableToString(std::vector<int>{}), "");
        EXPECT_EQ(IterableToString(std::vector<int>{7}, ", "), "7");
    }

    TEST(StringUtilTest, BinToHexWritesLowerCaseDigits)
    {
        EXPECT_EQ(BinToHex(""), "");
        EXPECT_EQ(BinToHex("\xAB"), "ab");
        EXPECT_EQ(BinToHex("\x0F"), "0f");
        EXPECT_EQ(BinToHex("Hello"), "48656c6c6f");
    }

    TEST(StringUtilTest, BinToHexPadsEveryByteToTwoDigits)
    {
        EXPECT_EQ(BinToHex(std::string("\x01\x02\xFF\x0A", 4)), "0102ff0a");
        EXPECT_EQ(BinToHex(std::string("\x00\x00", 2)), "0000");
    }

    TEST(StringUtilTest, HexToBinDecodesBothCases)
    {
        EXPECT_EQ(HexToBin("48656c6c6f"), std::optional<std::string>("Hello"));
        EXPECT_EQ(HexToBin("48656C6C6F"), std::optional<std::string>("Hello"));
        EXPECT_EQ(HexToBin("00ff"), std::optional<std::string>(std::string("\x00\xFF", 2)));
    }

    TEST(StringUtilTest, HexToBinOfEmptyStringIsEmpty)
    {
        EXPECT_EQ(HexToBin(""), std::optional<std::string>(""));
    }

    TEST(StringUtilTest, HexToBinRejectsNonHexadecimalCharacters)
    {
        EXPECT_EQ(HexToBin("4g"), std::nullopt);
        EXPECT_EQ(HexToBin("0x41"), std::nullopt);
        EXPECT_EQ(HexToBin("41 42"), std::nullopt);
    }

    TEST(StringUtilTest, HexToBinRejectsAnOddNumberOfDigits)
    {
        EXPECT_EQ(HexToBin("abc"), std::nullopt);
        EXPECT_EQ(HexToBin("0"), std::nullopt);
    }

    TEST(StringUtilTest, HexToBinReversesBinToHex)
    {
        const std::string binary("\x00\x01\x12\x0F\xAB\xCD\xEF", 7);

        EXPECT_EQ(HexToBin(BinToHex(binary)), std::optional<std::string>(binary));
    }

    TEST(StringUtilTest, IsHexOnlyAcceptsOnlyHexadecimalDigits)
    {
        EXPECT_TRUE(IsHexOnly("0123456789abcdefABCDEF"));
        EXPECT_TRUE(IsHexOnly(""));
        EXPECT_FALSE(IsHexOnly("12g4"));
        EXPECT_FALSE(IsHexOnly("12 34"));
        EXPECT_FALSE(IsHexOnly("-1"));
    }

    TEST(StringUtilTest, IsValidIpAddressAcceptsDottedQuads)
    {
        EXPECT_TRUE(IsValidIpAddress("0.0.0.0"));
        EXPECT_TRUE(IsValidIpAddress("192.168.1.1"));
        EXPECT_TRUE(IsValidIpAddress("255.255.255.255"));
        EXPECT_TRUE(IsValidIpAddress("10.0.200.249"));
    }

    TEST(StringUtilTest, IsValidIpAddressRejectsOutOfRangeBytes)
    {
        EXPECT_FALSE(IsValidIpAddress("256.0.0.1"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3.300"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3.1000"));
    }

    TEST(StringUtilTest, IsValidIpAddressRejectsMalformedAddresses)
    {
        EXPECT_FALSE(IsValidIpAddress(""));
        EXPECT_FALSE(IsValidIpAddress("1.2.3"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3.4.5"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3."));
        EXPECT_FALSE(IsValidIpAddress("a.b.c.d"));
        EXPECT_FALSE(IsValidIpAddress(" 1.2.3.4"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3.4 "));
        EXPECT_FALSE(IsValidIpAddress("1..3.4"));
    }

    TEST(StringUtilTest, IsValidIpAddressRejectsLeadingZeros)
    {
        EXPECT_FALSE(IsValidIpAddress("001.2.3.4"));
        EXPECT_FALSE(IsValidIpAddress("1.02.3.4"));
        EXPECT_FALSE(IsValidIpAddress("1.2.3.010"));
        EXPECT_TRUE(IsValidIpAddress("100.20.3.0"));
    }

} // namespace
} // namespace act::text::StringUtil
