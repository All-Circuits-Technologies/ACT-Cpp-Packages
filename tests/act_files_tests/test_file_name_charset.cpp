// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_file_name_charset.cpp
 * @brief Unit tests of FileNameCharset.
 *
 * Covers the bytes each character set allows: every byte value is checked against the expected
 * rule, and the values are reachable without naming the inner enum.
 */

#include "act_files/types/file_name_charset.hpp"

#include <gtest/gtest.h>

#include <string>

namespace act::files
{
namespace
{

    TEST(FileNameCharsetTest, ValuesAreReachableWithoutNamingTheInnerEnum)
    {
        EXPECT_EQ(FileNameCharset::WINDOWS_PRINTABLE_ASCII,
                  FileNameCharset::Enum::WINDOWS_PRINTABLE_ASCII);
    }

    TEST(FileNameCharsetTest, WindowsPrintableAsciiAllowsPrintableAsciiButWindowsForbidden)
    {
        const std::string forbidden = "<>:\"/\\|?*";
        constexpr int FIRST_PRINTABLE = 0x20;
        constexpr int LAST_PRINTABLE = 0x7E;
        constexpr int BYTE_VALUES = 256;

        for (int value = 0; value < BYTE_VALUES; ++value)
        {
            const auto byte = static_cast<char>(value);
            const bool isPrintable = value >= FIRST_PRINTABLE && value <= LAST_PRINTABLE;
            const bool isForbidden = forbidden.find(byte) != std::string::npos;

            EXPECT_EQ(FileNameCharset::IsAllowed(byte, FileNameCharset::WINDOWS_PRINTABLE_ASCII),
                      isPrintable && !isForbidden)
                << "byte " << value;
        }
    }

    TEST(FileNameCharsetTest, WindowsPrintableAsciiAllowsSpacesDotsAndShellCharacters)
    {
        for (const char byte : std::string(" .-_%&!$`'~#()[]{};,=+@^"))
        {
            EXPECT_TRUE(FileNameCharset::IsAllowed(byte, FileNameCharset::WINDOWS_PRINTABLE_ASCII))
                << byte;
        }
    }

} // namespace
} // namespace act::files
