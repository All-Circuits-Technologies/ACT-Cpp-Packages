// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_def_soft.cpp
 * @brief Unit tests of the helper macros and constants of def_soft.hpp.
 *
 * Covers the array length helpers (LEN, CONST_STRLEN), the multiplication overflow check
 * (sftTB_OVERFLOW), the string prefix check (sftIS_STR_START_WITH), the forced null termination
 * (sftSTR_SAFE) and the hexadecimal constants.
 */

#include "act_foundation/constants/def_soft.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace act::foundation
{
namespace
{

    TEST(DefSoftTest, LenGivesTheNumberOfElementsOfAnArray)
    {
        const std::int32_t values[] = {1, 2, 3, 4, 5};
        const char text[] = "abc";

        EXPECT_EQ(LEN(values), 5U);
        EXPECT_EQ(LEN(text), 4U);
    }

    TEST(DefSoftTest, ConstStrlenExcludesTheNullTerminator)
    {
        EXPECT_EQ(CONST_STRLEN("abc"), 3U);
        EXPECT_EQ(CONST_STRLEN(""), 0U);
    }

    TEST(DefSoftTest, TbOverflowDetectsAnOverflowingAllocationSize)
    {
        const std::uint64_t element = 0;
        const std::size_t small = 16;
        const std::size_t huge = (std::numeric_limits<std::size_t>::max() / sizeof(element)) + 1;

        EXPECT_FALSE(sftTB_OVERFLOW(element, small));
        EXPECT_TRUE(sftTB_OVERFLOW(element, huge));
    }

    TEST(DefSoftTest, IsStrStartWithComparesOnlyThePrefix)
    {
        const char buffer[] = "AT+OK\r\n";

        EXPECT_TRUE(sftIS_STR_START_WITH(buffer, "AT"));
        EXPECT_TRUE(sftIS_STR_START_WITH(buffer, "AT+OK"));
        EXPECT_FALSE(sftIS_STR_START_WITH(buffer, "OK"));
    }

    TEST(DefSoftTest, StrSafeTerminatesTheLastCharacter)
    {
        char buffer[4] = {'a', 'b', 'c', 'd'};

        sftSTR_SAFE(buffer);

        EXPECT_EQ(buffer[3], '\0');
        EXPECT_STREQ(buffer, "abc");
    }

    TEST(DefSoftTest, HexConstantsDescribeAByte)
    {
        EXPECT_EQ(HexConstants::HEX_CHARS_PER_BYTE, 2U);
        EXPECT_EQ(HexConstants::HEXADECIMAL_BASE, 16);
    }

} // namespace
} // namespace act::foundation
