// SPDX-FileCopyrightText: 2026 Ghislain Mangé <ghislain.mange@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_double_util.cpp
 * @brief Unit tests of DoubleUtil.
 *
 * Covers the comparison with a tolerance: values closer than the tolerance are the same, also when
 * they only differ by their coding; values further apart are not; the tolerance itself still makes
 * the same; and NaN or an infinity is never the same as anything.
 */

#include "act_num/double_util.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace act::num::DoubleUtil
{
namespace
{

    /** @brief A tolerance far below the 0.1 step of the values these tests compare */
    constexpr double EPSILON{0.001};

    constexpr double NO_TOLERANCE{0.0};

    TEST(DoubleUtilTest, AreAlmostEqualTakesForTheSameTwoDoublesWhichOnlyDifferByTheirCoding)
    {
        const double computed{0.1 + 0.2};
        const double written{0.3};

        // The premise: the two are not the same double, whatever they look like
        EXPECT_NE(computed, written);

        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(computed, written, EPSILON));
    }

    TEST(DoubleUtilTest, AreAlmostEqualTakesForTheSameTwoDoublesCloserThanTheTolerance)
    {
        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(25.50000001, 25.49999999, EPSILON));
        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(-25.50000001, -25.49999999, EPSILON));
    }

    TEST(DoubleUtilTest, AreAlmostEqualTakesForDifferentTwoDoublesFurtherApartThanTheTolerance)
    {
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(21.7, 21.8, EPSILON));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(15.0, 16.0, EPSILON));
    }

    TEST(DoubleUtilTest, AreAlmostEqualIncludesTheToleranceItself)
    {
        // Whole numbers and halves, so that the differences are exact
        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(1.0, 1.5, 0.5));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(1.0, 1.75, 0.5));
    }

    TEST(DoubleUtilTest, AreAlmostEqualDoesNotDependOnTheOrderOfTheValues)
    {
        EXPECT_EQ(DoubleUtil::AreAlmostEqual(25.5, 25.5004, EPSILON),
                  DoubleUtil::AreAlmostEqual(25.5004, 25.5, EPSILON));
        EXPECT_EQ(DoubleUtil::AreAlmostEqual(25.5, 25.6, EPSILON),
                  DoubleUtil::AreAlmostEqual(25.6, 25.5, EPSILON));
    }

    TEST(DoubleUtilTest, AreAlmostEqualWithoutToleranceOnlyAcceptsTheSameDouble)
    {
        const double next{std::nextafter(1.0, 2.0)};

        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(1.0, 1.0, NO_TOLERANCE));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(1.0, next, NO_TOLERANCE));
    }

    TEST(DoubleUtilTest, AreAlmostEqualAcceptsZeroAndNegativeZero)
    {
        EXPECT_TRUE(DoubleUtil::AreAlmostEqual(0.0, -0.0, NO_TOLERANCE));
    }

    TEST(DoubleUtilTest, AreAlmostEqualNeverAcceptsNotANumber)
    {
        const double notANumber{std::numeric_limits<double>::quiet_NaN()};

        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(notANumber, notANumber, EPSILON));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(notANumber, 1.0, EPSILON));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(1.0, notANumber, EPSILON));
    }

    TEST(DoubleUtilTest, AreAlmostEqualNeverAcceptsInfinity)
    {
        const double infinity{std::numeric_limits<double>::infinity()};

        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(infinity, infinity, EPSILON));
        EXPECT_FALSE(
            DoubleUtil::AreAlmostEqual(infinity, std::numeric_limits<double>::max(), EPSILON));
        EXPECT_FALSE(DoubleUtil::AreAlmostEqual(-infinity, infinity, EPSILON));
    }

} // namespace
} // namespace act::num::DoubleUtil
