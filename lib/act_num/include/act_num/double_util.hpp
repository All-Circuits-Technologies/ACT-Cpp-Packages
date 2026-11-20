// SPDX-FileCopyrightText: 2026 Ghislain Mangé <ghislain.mange@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <cmath>

/** @brief Set of helpers for double values */
namespace act::num::DoubleUtil
{
/* ## Methods */

/**
 * @brief Check whether two double values are the same, give or take a tolerance
 * @param lhs The first value
 * @param rhs The second value
 * @param epsilon The largest difference which still makes the values the same (not negative)
 * @return True when the values differ by at most @p epsilon, false otherwise, and false whenever
 *         one of the values is NaN or infinite
 * @note Comparing doubles with == is fragile: the same number can differ slightly once it went
 *       through a conversion or a computation (0.1 + 0.2 and 0.3 for instance).
 * @note The tolerance is absolute, not relative to the magnitude of the values: choose it from the
 *       finest step the compared values have.
 */
inline bool AreAlmostEqual(double lhs, double rhs, double epsilon)
{
    return std::abs(lhs - rhs) <= epsilon;
}

} // namespace act::num::DoubleUtil
