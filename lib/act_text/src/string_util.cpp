// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/string_util.hpp"

#include "act_foundation/constants/def_soft.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <iterator>
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

/** @brief Largest ASCII character code */
constexpr unsigned char ASCII_MAX_CHAR{0x7FU};

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

/** @brief Largest code point UTF-8 writes on a single byte, which is ASCII */
constexpr unsigned char UTF8_MAX_ONE_BYTE_CODE_POINT{0x7FU};

/** @brief Mask of the bits telling a lead byte of a 2 bytes UTF-8 sequence (110xxxxx) */
constexpr unsigned char UTF8_TWO_BYTES_LEAD_MASK{0xE0U};

/** @brief Bits telling a lead byte of a 2 bytes UTF-8 sequence, once masked */
constexpr unsigned char UTF8_TWO_BYTES_LEAD{0xC0U};

/** @brief Mask of the bits telling a lead byte of a 3 bytes UTF-8 sequence (1110xxxx) */
constexpr unsigned char UTF8_THREE_BYTES_LEAD_MASK{0xF0U};

/** @brief Bits telling a lead byte of a 3 bytes UTF-8 sequence, once masked */
constexpr unsigned char UTF8_THREE_BYTES_LEAD{0xE0U};

/** @brief Mask of the bits telling a lead byte of a 4 bytes UTF-8 sequence (11110xxx) */
constexpr unsigned char UTF8_FOUR_BYTES_LEAD_MASK{0xF8U};

/** @brief Bits telling a lead byte of a 4 bytes UTF-8 sequence, once masked */
constexpr unsigned char UTF8_FOUR_BYTES_LEAD{0xF0U};

/** @brief Mask of the bits telling a continuation byte of a UTF-8 sequence (10xxxxxx) */
constexpr unsigned char UTF8_CONTINUATION_MASK{0xC0U};

/** @brief Bits telling a continuation byte of a UTF-8 sequence, once masked */
constexpr unsigned char UTF8_CONTINUATION{0x80U};

/** @brief Number of code point bits a continuation byte holds */
constexpr unsigned int UTF8_CONTINUATION_BITS{6U};

/** @brief Smallest code point a 2 bytes UTF-8 sequence may hold */
constexpr char32_t UTF8_MIN_TWO_BYTES_CODE_POINT{0x80U};

/** @brief Smallest code point a 3 bytes UTF-8 sequence may hold */
constexpr char32_t UTF8_MIN_THREE_BYTES_CODE_POINT{0x800U};

/** @brief Smallest code point a 4 bytes UTF-8 sequence may hold */
constexpr char32_t UTF8_MIN_FOUR_BYTES_CODE_POINT{0x10000U};

/** @brief First UTF-16 surrogate, a code point UTF-8 may not hold */
constexpr char32_t UTF16_MIN_SURROGATE{0xD800U};

/** @brief Last UTF-16 surrogate, a code point UTF-8 may not hold */
constexpr char32_t UTF16_MAX_SURROGATE{0xDFFFU};

/** @brief Largest Unicode code point */
constexpr char32_t UNICODE_MAX_CODE_POINT{0x10FFFFU};

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
    // std::tolower() is undefined for the negative values a char takes beyond ASCII
    return std::ranges::equal(lhs, rhs, [](unsigned char a, unsigned char b) {
        return (std::tolower(a) == std::tolower(b));
    });
}

std::string DoubleToString(double value)
{
    std::array<char, DOUBLE_TEXT_BUFFER_SIZE> buffer{};

    const std::to_chars_result result =
        std::to_chars(buffer.data(),
                      std::next(buffer.data(), DOUBLE_TEXT_BUFFER_SIZE),
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

bool IsValidUtf8(const std::string &str)
{
    std::size_t position{0};
    while (position < str.size())
    {
        const auto leadByte{static_cast<unsigned char>(str[position])};
        if (leadByte <= UTF8_MAX_ONE_BYTE_CODE_POINT)
        {
            ++position;
            continue;
        }

        /* The lead byte tells the sequence length, and gives the first bits of the code point;
         * each length has its own smallest code point, a smaller one is an overlong encoding */
        std::size_t length{0};
        char32_t codePoint{0};
        char32_t minCodePoint{0};
        if ((leadByte & UTF8_TWO_BYTES_LEAD_MASK) == UTF8_TWO_BYTES_LEAD)
        {
            length = 2;
            codePoint = leadByte & static_cast<unsigned char>(~UTF8_TWO_BYTES_LEAD_MASK);
            minCodePoint = UTF8_MIN_TWO_BYTES_CODE_POINT;
        }
        else if ((leadByte & UTF8_THREE_BYTES_LEAD_MASK) == UTF8_THREE_BYTES_LEAD)
        {
            length = 3;
            codePoint = leadByte & static_cast<unsigned char>(~UTF8_THREE_BYTES_LEAD_MASK);
            minCodePoint = UTF8_MIN_THREE_BYTES_CODE_POINT;
        }
        else if ((leadByte & UTF8_FOUR_BYTES_LEAD_MASK) == UTF8_FOUR_BYTES_LEAD)
        {
            length = 4;
            codePoint = leadByte & static_cast<unsigned char>(~UTF8_FOUR_BYTES_LEAD_MASK);
            minCodePoint = UTF8_MIN_FOUR_BYTES_CODE_POINT;
        }
        else
        {
            /* A continuation byte without lead byte, or a lead byte no valid sequence starts with
             */
            return false;
        }

        if ((position + length) > str.size())
        {
            return false;
        }

        for (std::size_t index{1}; index < length; ++index)
        {
            const auto continuationByte{static_cast<unsigned char>(str[position + index])};
            if ((continuationByte & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION)
            {
                return false;
            }
            codePoint = (codePoint << UTF8_CONTINUATION_BITS) |
                        (continuationByte & static_cast<unsigned char>(~UTF8_CONTINUATION_MASK));
        }

        const bool isSurrogate{(codePoint >= UTF16_MIN_SURROGATE) &&
                               (codePoint <= UTF16_MAX_SURROGATE)};
        if ((codePoint < minCodePoint) || isSurrogate || (codePoint > UNICODE_MAX_CODE_POINT))
        {
            return false;
        }

        position += length;
    }

    return true;
}

bool IsAscii(const std::string &str)
{
    return std::ranges::all_of(str, [](char byte) {
        return static_cast<unsigned char>(byte) <= ASCII_MAX_CHAR;
    });
}

} // namespace act::text::StringUtil
