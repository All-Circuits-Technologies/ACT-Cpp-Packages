// SPDX-FileCopyrightText: 2026 Ghislain Mangé <ghislain.mange@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_float_util.cpp
 * @brief Unit tests of FloatUtil.
 *
 * Covers the conversion of a double to a float: a value of the float range, with its limits, is
 * converted; a value out of it is refused or clamped; NaN and the infinities are kept as they are;
 * and a value too close to zero for a float is not refused.
 */

#include "act_num/float_util.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <optional>

namespace act::num::FloatUtil
{
namespace
{

    constexpr double FLOAT_MAX{static_cast<double>(std::numeric_limits<float>::max())};
    constexpr double FLOAT_LOWEST{static_cast<double>(std::numeric_limits<float>::lowest())};

    TEST(FloatUtilTest, FromDoubleOptConvertsAValueOfTheFloatRange)
    {
        const std::optional<float> convertedOpt{FloatUtil::FromDoubleOpt(1.5)};

        ASSERT_TRUE(convertedOpt.has_value());
        const float converted{convertedOpt.value()};
        EXPECT_EQ(converted, 1.5F);
    }

    TEST(FloatUtilTest, FromDoubleOptConvertsTheLimitsOfTheFloatRange)
    {
        const std::optional<float> largestOpt{FloatUtil::FromDoubleOpt(FLOAT_MAX)};
        const std::optional<float> lowestOpt{FloatUtil::FromDoubleOpt(FLOAT_LOWEST)};

        ASSERT_TRUE(largestOpt.has_value());
        ASSERT_TRUE(lowestOpt.has_value());
        const float largest{largestOpt.value()};
        const float lowest{lowestOpt.value()};
        EXPECT_EQ(largest, std::numeric_limits<float>::max());
        EXPECT_EQ(lowest, std::numeric_limits<float>::lowest());
    }

    TEST(FloatUtilTest, FromDoubleOptRefusesAValueAboveTheFloatRange)
    {
        EXPECT_FALSE(FloatUtil::FromDoubleOpt(std::nextafter(FLOAT_MAX, HUGE_VAL)).has_value());
        EXPECT_FALSE(FloatUtil::FromDoubleOpt(std::numeric_limits<double>::max()).has_value());
    }

    TEST(FloatUtilTest, FromDoubleOptRefusesAValueBelowTheFloatRange)
    {
        EXPECT_FALSE(FloatUtil::FromDoubleOpt(std::nextafter(FLOAT_LOWEST, -HUGE_VAL)).has_value());
        EXPECT_FALSE(FloatUtil::FromDoubleOpt(std::numeric_limits<double>::lowest()).has_value());
    }

    TEST(FloatUtilTest, FromDoubleOptKeepsNotANumberAndTheInfinities)
    {
        const std::optional<float> notANumberOpt{
            FloatUtil::FromDoubleOpt(std::numeric_limits<double>::quiet_NaN())};
        const std::optional<float> infinityOpt{
            FloatUtil::FromDoubleOpt(std::numeric_limits<double>::infinity())};
        const std::optional<float> negativeInfinityOpt{
            FloatUtil::FromDoubleOpt(-std::numeric_limits<double>::infinity())};

        ASSERT_TRUE(notANumberOpt.has_value());
        ASSERT_TRUE(infinityOpt.has_value());
        ASSERT_TRUE(negativeInfinityOpt.has_value());
        const float notANumber{notANumberOpt.value()};
        const float infinity{infinityOpt.value()};
        const float negativeInfinity{negativeInfinityOpt.value()};
        EXPECT_TRUE(std::isnan(notANumber));
        EXPECT_EQ(infinity, std::numeric_limits<float>::infinity());
        EXPECT_EQ(negativeInfinity, -std::numeric_limits<float>::infinity());
    }

    TEST(FloatUtilTest, FromDoubleOptDoesNotRefuseAValueTooCloseToZeroForAFloat)
    {
        // The value is not refused, it is rounded to zero
        const std::optional<float> convertedOpt{FloatUtil::FromDoubleOpt(1.0e-300)};

        ASSERT_TRUE(convertedOpt.has_value());
        const float converted{convertedOpt.value()};
        EXPECT_EQ(converted, 0.0F);
    }

    TEST(FloatUtilTest, FromDoubleClampKeepsAValueOfTheFloatRange)
    {
        EXPECT_EQ(FloatUtil::FromDoubleClamp(-2.25), -2.25F);
        EXPECT_EQ(FloatUtil::FromDoubleClamp(FLOAT_MAX), std::numeric_limits<float>::max());
        EXPECT_EQ(FloatUtil::FromDoubleClamp(FLOAT_LOWEST), std::numeric_limits<float>::lowest());
    }

    TEST(FloatUtilTest, FromDoubleClampLimitsAValueOutOfTheFloatRangeToItsLimit)
    {
        EXPECT_EQ(FloatUtil::FromDoubleClamp(std::numeric_limits<double>::max()),
                  std::numeric_limits<float>::max());
        EXPECT_EQ(FloatUtil::FromDoubleClamp(std::numeric_limits<double>::lowest()),
                  std::numeric_limits<float>::lowest());
    }

    TEST(FloatUtilTest, FromDoubleClampKeepsNotANumberAndTheInfinities)
    {
        EXPECT_TRUE(
            std::isnan(FloatUtil::FromDoubleClamp(std::numeric_limits<double>::quiet_NaN())));
        EXPECT_EQ(FloatUtil::FromDoubleClamp(std::numeric_limits<double>::infinity()),
                  std::numeric_limits<float>::infinity());
        EXPECT_EQ(FloatUtil::FromDoubleClamp(-std::numeric_limits<double>::infinity()),
                  -std::numeric_limits<float>::infinity());
    }

    TEST(FloatUtilTest, CanConvertToFloatTellsTheFloatRange)
    {
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(0.0));
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(FLOAT_MAX));
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(FLOAT_LOWEST));
        EXPECT_FALSE(FloatUtil::CanConvertToFloat(std::nextafter(FLOAT_MAX, HUGE_VAL)));
        EXPECT_FALSE(FloatUtil::CanConvertToFloat(std::nextafter(FLOAT_LOWEST, -HUGE_VAL)));
    }

    TEST(FloatUtilTest, CanConvertToFloatAcceptsNotANumberAndTheInfinities)
    {
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(std::numeric_limits<double>::quiet_NaN()));
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(std::numeric_limits<double>::infinity()));
        EXPECT_TRUE(FloatUtil::CanConvertToFloat(-std::numeric_limits<double>::infinity()));
    }

} // namespace
} // namespace act::num::FloatUtil
