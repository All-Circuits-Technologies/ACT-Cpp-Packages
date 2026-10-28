// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_def_soft.cpp
 * @brief Unit tests of the constants of def_soft.hpp.
 */

#include "act_foundation/constants/def_soft.hpp"

#include <gtest/gtest.h>

namespace act::foundation
{
namespace
{

    TEST(DefSoftTest, HexConstantsDescribeAByte)
    {
        EXPECT_EQ(HexConstants::HEX_CHARS_PER_BYTE, 2U);
        EXPECT_EQ(HexConstants::HEXADECIMAL_BASE, 16);
    }

} // namespace
} // namespace act::foundation
