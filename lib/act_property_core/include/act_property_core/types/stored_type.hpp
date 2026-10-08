// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::property
{

/**
 * @brief The closed set of native types a store can persist.
 *
 * @note The underlying integer values are part of the on-disk contract (they are the type tag
 *       written next to every value); they must never be reordered or reused.
 */
enum class StoredType : int
{
    BOOL = 0,
    INT8 = 1,
    INT16 = 2,
    INT32 = 3,
    INT64 = 4,
    UINT8 = 5,
    UINT16 = 6,
    UINT32 = 7,
    UINT64 = 8,
    FLOAT = 9,
    DOUBLE = 10,
    STRING = 11,
};

} // namespace act::property
