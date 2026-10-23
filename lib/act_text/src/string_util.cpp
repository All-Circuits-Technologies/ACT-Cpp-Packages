// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/string_util.hpp"

#include "act_foundation/constants/def_soft.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <optional>
#include <regex>
#include <string_view>
#include <system_error>

namespace
{

/**
 * @brief Size of the buffer which receives the text of a double: the longest text of a double
 *        written without exponent (1.7976931348623157e308, which has 309 digits) is shorter
 */
constexpr std::size_t DOUBLE_TEXT_BUFFER_SIZE{400U};

/** @brief Last C0 control character, the ones before it being control characters too */
constexpr unsigned char LAST_C0_CONTROL_CHAR{0x1FU};

/** @brief The DEL control character */
constexpr unsigned char DELETE_CHAR{0x7FU};

/** @brief First byte of the UTF-8 encoding of the C1 control characters (U+0080 to U+009F) */
constexpr unsigned char C1_CONTROL_LEAD_BYTE{0xC2U};

/** @brief Range of the second byte of the UTF-8 encoding of the C1 control characters */
constexpr unsigned char FIRST_C1_CONTROL_TRAIL_BYTE{0x80U};
constexpr unsigned char LAST_C1_CONTROL_TRAIL_BYTE{0x9FU};

/**
 * @brief IPv4 address in dotted decimal (0-255.0-255.0-255.0-255), without leading zeros: some
 *        parsers (inet_aton) read a byte with a leading zero as octal, so "010" would not mean 10
 * @note Built once, at load time: compiling a regular expression is costly.
 */
const std::regex IP_ADDRESS_PATTERN(R"(^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\.){3})"
                                    R"((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$)");

/** @brief Append a byte as `\xNN`, with upper case hexadecimal digits */
void AppendEscapedByte(std::string &text, unsigned char byte)
{
    constexpr std::string_view HEX_DIGITS{"0123456789ABCDEF"};
    constexpr unsigned int NIBBLE_BITS{4U};
    constexpr unsigned int NIBBLE_MASK{0x0FU};

    text += "\\x";
    text += HEX_DIGITS[byte >> NIBBLE_BITS];
    text += HEX_DIGITS[byte & NIBBLE_MASK];
}

} // namespace

namespace act::text::StringUtil
{

void ToUpperInPlace(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
}

std::string ToUpper(const std::string &str)
{
    std::string copy = str;
    ToUpperInPlace(copy);
    return copy;
}

void ToLowerInPlace(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
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
    // std::tolower() is undefined for the negative values a char takes beyond ASCII
    return std::ranges::equal(lhs, rhs, [](unsigned char a, unsigned char b) {
        return (std::tolower(a) == std::tolower(b));
    });
}

std::string DoubleToString(double value)
{
    std::array<char, DOUBLE_TEXT_BUFFER_SIZE> buffer{};

    const std::to_chars_result result = std::to_chars(buffer.data(),
                                                      buffer.data() + buffer.size(),
                                                      value,
                                                      std::chars_format::fixed);
    if (result.ec != std::errc())
    {
        /* Can't happen: the buffer is longer than the longest text of a double */
        return {};
    }

    return {buffer.data(), result.ptr};
}

std::string BinToHex(const std::string &bin)
{
    std::ostringstream hex;

    /* configure hex output; the width is reset by each insertion, so it is set for every byte */
    hex << std::hex << std::setfill('0');
    for (unsigned char c : bin)
    {
        hex << std::setw(2) << (int)c;
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
        std::string hexByte{hex.substr(i, act::foundation::HexConstants::HEX_CHARS_PER_BYTE)};
        char byte{static_cast<char>(
            std::stoul(hexByte, nullptr, act::foundation::HexConstants::HEXADECIMAL_BASE))};
        binaryString.push_back(byte);
    }

    return binaryString;
}

bool IsHexOnly(const std::string &data)
{
    return std::all_of(data.begin(), data.end(), [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c));
    });
}

bool IsValidIpAddress(const std::string &ipAddress)
{
    return std::regex_match(ipAddress, IP_ADDRESS_PATTERN);
}

std::string ToPrintableText(const std::string &text)
{
    std::string printableText;
    printableText.reserve(text.size());

    for (std::size_t index{0}; index < text.size(); ++index)
    {
        const auto byte = static_cast<unsigned char>(text[index]);
        if (byte <= LAST_C0_CONTROL_CHAR || byte == DELETE_CHAR)
        {
            AppendEscapedByte(printableText, byte);
            continue;
        }

        const bool isC1Control =
            byte == C1_CONTROL_LEAD_BYTE && (index + 1) < text.size() &&
            static_cast<unsigned char>(text[index + 1]) >= FIRST_C1_CONTROL_TRAIL_BYTE &&
            static_cast<unsigned char>(text[index + 1]) <= LAST_C1_CONTROL_TRAIL_BYTE;
        if (isC1Control)
        {
            AppendEscapedByte(printableText, byte);
            ++index;
            AppendEscapedByte(printableText, static_cast<unsigned char>(text[index]));
            continue;
        }

        printableText += text[index];
    }

    return printableText;
}

} // namespace act::text::StringUtil
