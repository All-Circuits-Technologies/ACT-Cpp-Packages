// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <optional>

/** @brief Set of helpers to make float values from double ones */
namespace act::num::FloatUtil
{
/* ## Methods */

/**
 * @brief Convert a double value to a float, or nullopt when the value is out of the float range
 * @param value The double value to convert
 * @return A float value if the conversion is successful, std::nullopt otherwise
 * @note NaN and the infinities are converted as they are: a float can hold them.
 * @note A float is less precise than a double, so the result may be slightly different from the
 *       input value, and a value too close to zero for a float becomes zero (it is not refused).
 */
std::optional<float> FromDoubleOpt(double value);

/**
 * @brief Convert a double value to a float, clamped to the float range when the value is out of it
 * @param value The double value to convert
 * @return A float value, the lowest or the largest finite float when the value is out of the float
 *         range, NaN or an infinity as they are
 * @note A float is less precise than a double, so the result may be slightly different from the
 *       input value.
 */
float FromDoubleClamp(double value);

/**
 * @brief Check whether a double value is inside the float range, so that it can be converted to a
 *        float without being refused by FromDoubleOpt
 * @param value The double value to check
 * @return True when the value is inside the float range (limits included), NaN or an infinity;
 *         false when it is below the lowest or above the largest finite float
 */
bool CanConvertToFloat(double value);

} // namespace act::num::FloatUtil
