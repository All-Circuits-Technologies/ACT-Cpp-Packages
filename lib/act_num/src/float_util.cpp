// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_num/float_util.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace act::num::FloatUtil
{

std::optional<float> FromDoubleOpt(double value)
{
    if (!CanConvertToFloat(value))
    {
        return std::nullopt;
    }

    return static_cast<float>(value);
}

float FromDoubleClamp(double value)
{
    if (std::isnan(value) || std::isinf(value))
    {
        // NaN, -inf, +inf: return as is
        return static_cast<float>(value);
    }

    if (value < std::numeric_limits<float>::lowest())
    {
        // Below the float range
        return std::numeric_limits<float>::lowest();
    }

    if (value > std::numeric_limits<float>::max())
    {
        // Above the float range
        return std::numeric_limits<float>::max();
    }

    // In-range case
    return static_cast<float>(value);
}

bool CanConvertToFloat(double value)
{
    if (std::isnan(value) || std::isinf(value))
    {
        // NaN, -inf, +inf: can be converted
        return true;
    }

    if (value < std::numeric_limits<float>::lowest() || value > std::numeric_limits<float>::max())
    {
        // Out of the float range
        return false;
    }

    // In-range case
    return true;
}

} // namespace act::num::FloatUtil
