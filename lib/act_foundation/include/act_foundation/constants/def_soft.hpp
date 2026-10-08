/*
 * SPDX-FileCopyrightText: 2020 Pierre-Noel Bouteville <pierre-noel.bouteville@allcircuits.com>
 * SPDX-FileCopyrightText: 2022 Damien Manceau <damien.manceau@allcircuits.com>
 *
 * SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
 */

#pragma once

#include <cstddef>

#define OFF 0
#define ON 1

#define RELEASED 0
#define PRESSED 1

#define DOWN 0
#define UP 1

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#define STATIC_ASSERT(COND, MSG) static_assert(COND, MSG)
#define LEN(x) (sizeof(x) / sizeof(*(x)))
#define CONST_STRLEN(x) ((sizeof(x) / sizeof(*(x))) - 1) /* -1 for null char */

/// @brief Indicate that the length or size parameter don't have to be used
#define NO_SIZE std::size_t(0)

#define sftTB_OVERFLOW(elem, nbr) ((nbr) != ((sizeof(elem) * (nbr)) / sizeof(elem)))

#define sftIS_STR_START_WITH(buf, const_str) (memcmp(buf, const_str, CONST_STRLEN(const_str)) == 0)

#define sftSTR_SAFE(str) (str)[LEN(str) - 1] = 0

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

/**
 * @brief This macro is used to define a pointer type that can be used as a unique key.
 *
 * @note Do not use this macro to cast pointers. It is only for defining unique key types.
 */
// TODO(brolandeau): find a better way to define unique key types
#define OBJ_PTR_AS_UNIQUE_KEY void *

/** @brief Identity multiplication factor for Modbus (or other protocols) */
constexpr float IDENTITY_MULTIPLICATION_FACTOR{1.0F};

/**
 * @brief This namespace contains hexadecimal conversion constants
 */
namespace act::foundation::HexConstants
{
/** @brief Number of hexadecimal characters per byte */
constexpr size_t HEX_CHARS_PER_BYTE = 2;

/** @brief Base used for hexadecimal conversion */
constexpr int HEXADECIMAL_BASE = 16;
} // namespace act::foundation::HexConstants
