// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <string_view>

namespace act::files
{
/** @brief Character sets a file name may be restricted to, see FilePathUtil::SanitizeFileName */
class FileNameCharset : private act::foundation::NotCopiableNotMovable
{
  public:
    enum class Enum
    {
        /**
         * @brief Printable ASCII characters (0x20 to 0x7E) except those Windows forbids in a file
         *        name: `<`, `>`, `:`, `"`, `/`, `\`, `|`, `?` and `*`
         * @note Every non ASCII byte is out of this charset, including accentuated characters.
         * @note Characters special to a shell but valid in a file name, such as `%`, `&`, `!`, `$`
         *       or a leading `-`, remains part of this charset.
         */
        WINDOWS_PRINTABLE_ASCII,
    };

    /** @brief Expose the values as FileNameCharset::WINDOWS_PRINTABLE_ASCII */
    using enum Enum;

  public:
    /**
     * @brief Test if a byte may appear in a file name restricted to a character set
     * @param byte The byte to test
     * @param charset The character set
     * @return True if the byte is part of the character set, false otherwise
     */
    [[nodiscard]] static bool IsAllowed(char byte, Enum charset);

  private:
    /** @brief Lowest printable ASCII character (space) */
    static constexpr unsigned char FIRST_PRINTABLE_ASCII = 0x20;

    /** @brief Highest printable ASCII character (tilde) */
    static constexpr unsigned char LAST_PRINTABLE_ASCII = 0x7E;

    /** @brief Printable ASCII characters Windows forbids in a file name */
    static constexpr std::string_view WINDOWS_FORBIDDEN_CHARS = "<>:\"/\\|?*";
};
} // namespace act::files
