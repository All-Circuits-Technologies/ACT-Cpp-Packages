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

/* ## Methods (members, then non-members) */

/**
 * @brief Trim from start (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
    return s;
}

/**
 * @brief Trim from end (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(),
            s.end());
    return s;
}

/**
 * @brief Trim from both ends (in place)
 * @param s The string to alter
 * @return The trimmed string
 */
inline std::string &Trim(std::string &s)
{
    Rtrim(s);
    Ltrim(s);
    return s;
}

/**
 * @brief Trim from the start (copying)
 * @param s The string to trim
 * @return A new string with the leading whitespace removed
 */
inline std::string LtrimCopy(std::string s)
{
    Ltrim(s);
    return s;
}

/**
 * @brief Trim from the end (copying)
 * @param s The string to trim
 * @return A new string with the trailing whitespace removed
 */
inline std::string RtrimCopy(std::string s)
{
    Rtrim(s);
    return s;
}

/**
 * @brief Trim from both ends (copying)
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

/* ## Constants */

} // namespace act::text::StringUtil
