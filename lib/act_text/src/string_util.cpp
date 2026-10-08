// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/string_util.hpp"

#include "act_foundation/constants/def_soft.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

namespace act::text::StringUtil
{

void ToUpperInPlace(std::string &str)
{
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::toupper(c); });
}

std::string ToUpper(const std::string &str)
{
    std::string copy = str;
    ToUpperInPlace(copy);
    return copy;
}

void ToLowerInPlace(std::string &str)
{
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
}

std::string ToLower(const std::string &str)
{
    std::string copy = str;
    ToLowerInPlace(copy);
    return copy;
}

int CompareNoCase(const std::string &lhs, const std::string &rhs)
{
    const std::string lhsLowCase = ToLower(lhs);
    const std::string rhsLowCase = ToLower(rhs);
    return lhsLowCase.compare(rhsLowCase);
}

bool AreStringEqualNoCase(const std::string &lhs, const std::string &rhs)
{
    return std::ranges::equal(lhs, rhs, [](char a, char b) {
        return (std::tolower(a) == std::tolower(b));
    });
}

std::string BinToHex(const std::string &bin)
{
    std::ostringstream hex;

    /* configure hex output; the width is reset by each insertion, so it is set for every byte */
    hex << std::hex << std::setfill('0');
    for (const unsigned char c : bin)
    {
        hex << std::setw(2) << static_cast<int>(c);
    }
    return hex.str();
}

std::optional<std::string> HexToBin(const std::string &hex)
{
    if (!IsHexOnly(hex) || (hex.length() % act::foundation::HexConstants::HEX_CHARS_PER_BYTE) != 0)
    {
        // Each byte is written with two digits: an odd length leaves half a byte
        return std::nullopt;
    }

    std::string binaryString;

    for (size_t i{0}; i + 1 < hex.length(); i += act::foundation::HexConstants::HEX_CHARS_PER_BYTE)
    {
        const std::string hexByte{hex.substr(i, act::foundation::HexConstants::HEX_CHARS_PER_BYTE)};
        const char byte{static_cast<char>(
            std::stoul(hexByte, nullptr, act::foundation::HexConstants::HEXADECIMAL_BASE))};
        binaryString.push_back(byte);
    }

    return binaryString;
}

bool IsHexOnly(const std::string &data)
{
    return std::ranges::all_of(data,
                               [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); });
}

bool IsValidIpAddress(const std::string &ipAddress)
{
    // IPv4 address in dotted decimal (0-255.0-255.0-255.0-255), without leading zeros: some parsers
    // (inet_aton) read a byte with a leading zero as octal, so "010" would not mean 10. Built once,
    // on first use: compiling a regular expression is costly, and a function-local static is
    // initialized where an exception can be caught.
    static const std::regex IP_ADDRESS_PATTERN(
        R"(^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\.){3})"
        R"((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$)");
    return std::regex_match(ipAddress, IP_ADDRESS_PATTERN);
}

} // namespace act::text::StringUtil
