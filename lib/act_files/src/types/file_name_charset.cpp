// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_files/types/file_name_charset.hpp"

namespace act::files
{
bool FileNameCharset::IsAllowed(char byte, Enum charset)
{
    const auto unsignedByte = static_cast<unsigned char>(byte);

    switch (charset)
    {
        case Enum::WINDOWS_PRINTABLE_ASCII:
            return unsignedByte >= FIRST_PRINTABLE_ASCII && unsignedByte <= LAST_PRINTABLE_ASCII &&
                   WINDOWS_FORBIDDEN_CHARS.find(byte) == std::string_view::npos;
    }

    return false;
}
} // namespace act::files
