/*
 * SPDX-FileCopyrightText: 2020 Pierre-Noel Bouteville <pierre-noel.bouteville@allcircuits.com>
 * SPDX-FileCopyrightText: 2022 Damien Manceau <damien.manceau@allcircuits.com>
 *
 * SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
 */

#pragma once

#include <cstddef>

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
