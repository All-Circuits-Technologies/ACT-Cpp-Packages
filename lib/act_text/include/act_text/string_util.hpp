// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

/** @brief Set of String helpers */
namespace act::text::StringUtil
{
/* ## Types */

/* ## Constants */

/**
 * @brief The characters Ltrim, Rtrim, Trim and their copying variants remove: the whitespace
 *        characters, as std::isspace in the default locale
 * @note Declared before the methods, which use it.
 */
constexpr const char *WHITESPACE_CHARS = " \t\n\v\f\r";

/* ## Methods (members, then non-members) */

/**
 * @brief Trim the given characters from start (in place)
 * @param s The string to alter
 * @param chars The characters to remove
 * @return The trimmed string
 * @note The string is processed byte by byte, regardless of the locale: a multi-byte UTF-8
 *       character is never altered as long as @p chars is ASCII.
 * @note Not an overload of Ltrim(), which could no longer be passed to std::for_each
 */
inline std::string &LtrimChars(std::string &s, const std::string &chars)
{
    s.erase(0, s.find_first_not_of(chars));
    return s;
}

/**
 * @brief Trim the whitespace from start (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Ltrim(std::string &s)
{
    return LtrimChars(s, WHITESPACE_CHARS);
}

/**
 * @brief Trim the given characters from end (in place)
 * @param s The string to alter
 * @param chars The characters to remove
 * @return The trimmed string
 * @note The string is processed byte by byte, regardless of the locale: a multi-byte UTF-8
 *       character is never altered as long as @p chars is ASCII.
 * @note Not an overload of Rtrim(), which could no longer be passed to std::for_each
 */
inline std::string &RtrimChars(std::string &s, const std::string &chars)
{
    const std::size_t lastKept = s.find_last_not_of(chars);
    s.erase(lastKept == std::string::npos ? 0 : lastKept + 1);
    return s;
}

/**
 * @brief Trim the whitespace from end (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Rtrim(std::string &s)
{
    return RtrimChars(s, WHITESPACE_CHARS);
}

/**
 * @brief Trim the given characters from both ends (in place)
 * @param s The string to alter
 * @param chars The characters to remove
 * @return The trimmed string
 * @note Not an overload of Trim(), which could no longer be passed to std::for_each
 */
inline std::string &TrimChars(std::string &s, const std::string &chars)
{
    RtrimChars(s, chars);
    LtrimChars(s, chars);
    return s;
}

/**
 * @brief Trim the whitespace from both ends (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Trim(std::string &s)
{
    return TrimChars(s, WHITESPACE_CHARS);
}

/**
 * @brief Trim the given characters from the start (copying)
 * @param s The string to trim
 * @param chars The characters to remove
 * @return A new string with the leading characters removed
 * @note Not an overload of LtrimCopy(), which could no longer be passed to std::for_each
 */
inline std::string LtrimCharsCopy(std::string s, const std::string &chars)
{
    LtrimChars(s, chars);
    return s;
}

/**
 * @brief Trim the whitespace from the start (copying)
 * @param s The string to trim
 * @return A new string with the leading whitespace removed
 */
inline std::string LtrimCopy(std::string s)
{
    Ltrim(s);
    return s;
}

/**
 * @brief Trim the given characters from the end (copying)
 * @param s The string to trim
 * @param chars The characters to remove
 * @return A new string with the trailing characters removed
 * @note Not an overload of RtrimCopy(), which could no longer be passed to std::for_each
 */
inline std::string RtrimCharsCopy(std::string s, const std::string &chars)
{
    RtrimChars(s, chars);
    return s;
}

/**
 * @brief Trim the whitespace from the end (copying)
 * @param s The string to trim
 * @return A new string with the trailing whitespace removed
 */
inline std::string RtrimCopy(std::string s)
{
    Rtrim(s);
    return s;
}

/**
 * @brief Trim the given characters from both ends (copying)
 * @param s The string to trim
 * @param chars The characters to remove
 * @return A new string with the leading and trailing characters removed
 * @note Not an overload of TrimCopy(), which could no longer be passed to std::for_each
 */
inline std::string TrimCharsCopy(std::string s, const std::string &chars)
{
    TrimChars(s, chars);
    return s;
}

/**
 * @brief Trim the whitespace from both ends (copying)
 * @param s The string to trim
 * @return A new string with the whitespace removed
 */
inline std::string TrimCopy(std::string s)
{
    Trim(s);
    return s;
}

/**
 * @brief Convert a string to uppercase in place
 * @param str The string to alter
 */
void ToUpperInPlace(std::string &str);

/** @brief Convert a string to uppercase
 * @param str The string to convert
 * @return A new string that is the uppercase version of the input
 */
std::string ToUpper(const std::string &str);

/** @brief Convert a string to lowercase in place
 * @param str The string to alter
 */
void ToLowerInPlace(std::string &str);

/** @brief Convert a string to lowercase
 * @param str The string to convert
 * @return A new string that is the lowercase version of the input
 */
std::string ToLower(const std::string &str);

/** @brief Compare two strings without case sensitivity
 * @param lhs The left-hand side string
 * @param rhs The right-hand side string
 * @return A negative value if lhs < rhs, a positive value if lhs > rhs, and 0 if they are equal
 * @note use AreStringEqualNoCase for a simple equality check without case sensitivity,
 * and CompareNoCase for a full lexicographical comparison without case sensitivity
 * (for sorting purposes).
 */
int CompareNoCase(const std::string &lhs, const std::string &rhs);

/** @brief Check if two strings are equal without case sensitivity
 * @param lhs The left-hand side string
 * @param rhs The right-hand side string
 * @return true if the strings are equal (ignoring case), false otherwise
 */
bool AreStringEqualNoCase(const std::string &lhs, const std::string &rhs);

/** @brief Convert an iterable to a string
 * @param iterable The iterable to convert
 * @param delimiter The delimiter to use between elements
 * @return A string representation of the iterable
 * @note T must implement stream operator<<.
 */
template <typename T>
std::string IterableToString(const T &iterable, const std::string &delimiter = ",")
{
    std::ostringstream oss;
    for (const auto &item : iterable)
    {
        oss << item << delimiter;
    }
    std::string result = oss.str();
    if (!result.empty())
    {
        // Remove trailing delimiter
        result.erase(result.size() - delimiter.size());
    }
    return result;
}

/**
 * @brief Convert a double to its text, whatever the locale is
 * @note The text is the shortest one which is read back as the same value, with a dot as decimal
 *       separator and without exponent (ex: "21.5", "100000", "0.0001"). Special values will
 *       give "nan", "inf", "-0", "-nan" and "-inf".
 * @param value The value to convert
 * @return The text of the value
 */
std::string DoubleToString(double value);

/**
 * @brief Convert a binary string to its hexadecimal representation
 * @param bin The binary string to convert
 * @return A hexadecimal string representation of the input
 */
std::string BinToHex(const std::string &bin);

/**
 * @brief Convert a hexadecimal string to its binary representation
 * @param hex The hexadecimal string to convert, two digits per byte
 * @return A binary string representation of the input, or an empty optional if it holds a
 *         character which is not an hexadecimal digit or an odd number of digits
 */
std::optional<std::string> HexToBin(const std::string &hex);

/** @brief Check if a string contains only hexadecimal characters
 * @param data The string to check
 * @return true if the string contains only hexadecimal characters (0-9, A-F, a-f), false
 * otherwise
 */
bool IsHexOnly(const std::string &data);

/**
 * @brief Validate if a string is a valid IPv4 address
 * @note The address must be written in dotted decimal, each byte without leading zeros
 * @param ipAddress The string to validate
 * @return true if the string is a valid IPv4 address, false otherwise
 */
bool IsValidIpAddress(const std::string &ipAddress);

/**
 * @brief Make an untrusted text printable, to insert it in a log or an error message
 * @details Each control character is written as `\xNN` (upper case hexadecimal digits), so that
 *          the text can neither forge a line nor send a terminal escape sequence:
 *          - the C0 controls (0x00 to 0x1F, tabulation and line feed included) and DEL (0x7F);
 *          - the C1 controls (U+0080 to U+009F), whose two UTF-8 bytes are both escaped.
 * @param text The text, in UTF-8 or not
 * @return The text, its control characters escaped; the other bytes, `\` included, are kept
 */
std::string ToPrintableText(const std::string &text);

} // namespace act::text::StringUtil
