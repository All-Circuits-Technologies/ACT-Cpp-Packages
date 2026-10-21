// SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_files/file_path_util.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_text/string_util.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <vector>

namespace
{

/** @brief Replacement character used when the requested one is invalid */
constexpr char DEFAULT_REPLACEMENT_CHAR = '_';

/** @brief Dot, separating the stem of a file name from its extension */
constexpr char DOT = '.';

/** @brief Space */
constexpr char SPACE = ' ';

/** @brief Lowest printable ASCII character, for the logs */
constexpr char FIRST_PRINTABLE_ASCII = ' ';

/** @brief Highest printable ASCII character, for the logs */
constexpr char LAST_PRINTABLE_ASCII = '~';

} // namespace

namespace act::files::FilePathUtil
{
bool ArePathsEqual(const std::string &path1,
                   const std::string &path2,
                   const act::foundation::AbsLogger &logger)
{
    if (path1 == path2)
    {
        // No need to check further if the strings are identical
        return true;
    }

    bool isEqual = false;
    try
    {
        // A relative path is resolved against the current directory
        isEqual = std::filesystem::equivalent(path1, path2);
    }
    catch (const std::exception &e)
    {
        logger.warningStream() << "ArePathsEqual: Exception occurred while comparing paths '"
                               << path1 << "' and '" << path2 << "': " << e.what();
    }

    return isEqual;
}

std::string GetFilename(const std::string &path)
{
    if (path.empty())
    {
        return "";
    }

    std::filesystem::path fsPath(path);
    return fsPath.filename().string();
}

bool IsWindowsReservedName(const std::string &fileName)
{
    // DOS device names (CON is the console, COM1 a serial port...), built on the first call only
    static const std::vector<std::string> WINDOWS_RESERVED_NAMES = [] {
        std::vector<std::string> names = {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"};

        // Serial and parallel ports: a digit, or one of the superscript digits ¹, ² and ³ (here
        // in UTF-8), after their prefix
        const std::array<std::string, 2> portPrefixes = {"COM", "LPT"};
        const std::array<std::string, 13> portNumbers =
            {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3"};
        for (const std::string &prefix : portPrefixes)
        {
            for (const std::string &number : portNumbers)
            {
                names.push_back(prefix + number);
            }
        }

        return names;
    }();

    // Only the stem counts, the part before the first dot, without its trailing spaces: Windows
    // takes "con.txt" and "con .txt" for the console too
    std::string stem = fileName.substr(0, fileName.find(DOT));
    act::text::StringUtil::RtrimChars(stem, std::string(1, SPACE));

    return std::ranges::any_of(WINDOWS_RESERVED_NAMES, [&stem](const std::string &reserved) {
        return act::text::StringUtil::AreStringEqualNoCase(stem, reserved);
    });
}

std::string SanitizeFileName(const std::string &name,
                             FileNameCharset::Enum charset,
                             const act::foundation::AbsLogger &logger,
                             const SanitizeFileNameOptions &options)
{
    char replacementChar = options.replacementChar;
    if (!FileNameCharset::IsAllowed(replacementChar, charset) || replacementChar == DOT ||
        replacementChar == SPACE)
    {
        // A control or non ASCII character would be unreadable in the logs: give its code too
        const bool isPrintable =
            replacementChar >= FIRST_PRINTABLE_ASCII && replacementChar <= LAST_PRINTABLE_ASCII;
        logger.warningStream() << "SanitizeFileName: invalid replacement character "
                               << (isPrintable ? std::string{'\'', replacementChar, '\'', ' '}
                                               : std::string())
                               << "(code "
                               << static_cast<int>(static_cast<unsigned char>(replacementChar))
                               << "), '" << DEFAULT_REPLACEMENT_CHAR << "' used instead";
        replacementChar = DEFAULT_REPLACEMENT_CHAR;
    }

    std::size_t maxLength = options.maxLength;
    if (maxLength == 0)
    {
        logger.warningStream() << "SanitizeFileName: a maximum length of 0 is handled as 1";
        maxLength = 1;
    }

    // Replace each run of forbidden bytes by a single replacement character
    std::string result;
    result.reserve(name.size());
    bool lastWasReplaced = false;
    for (const char byte : name)
    {
        if (FileNameCharset::IsAllowed(byte, charset))
        {
            result += byte;
            lastWasReplaced = false;
        }
        else if (!lastWasReplaced)
        {
            result += replacementChar;
            lastWasReplaced = true;
        }
    }

    // Spaces are unwelcome at both ends. Windows forbids a name ending with a dot or a space,
    // and "." and ".." are not names; leading dots are kept, for hidden files such as ".bashrc"
    const std::string charsToTrim{DOT, SPACE, replacementChar};
    act::text::StringUtil::Trim(result);
    act::text::StringUtil::RtrimChars(result, charsToTrim);

    if (result.size() > maxLength)
    {
        result.resize(maxLength);
        act::text::StringUtil::RtrimChars(result, charsToTrim);
    }

    std::string fallbackName = options.fallbackName.substr(0, maxLength);
    if (result.empty())
    {
        return fallbackName;
    }

    // Windows reserves some device names, whatever their extension: "con.txt" is the console
    if (IsWindowsReservedName(result))
    {
        // Right after the stem, before the spaces which may end it
        std::string stem = result.substr(0, result.find(DOT));
        act::text::StringUtil::RtrimChars(stem, std::string(1, SPACE));

        if (result.size() < maxLength)
        {
            result.insert(stem.size(), 1, replacementChar);
        }
        else
        {
            result[stem.size() - 1] = replacementChar;
        }

        // A letter, a digit or a '$' as replacement character may give a reserved name again:
        // "COM1" at its maximum length gives "COM0" with '0'
        if (IsWindowsReservedName(result))
        {
            return fallbackName;
        }
    }

    return result;
}
} // namespace act::files::FilePathUtil
