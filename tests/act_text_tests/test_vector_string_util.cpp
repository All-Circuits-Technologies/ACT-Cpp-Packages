// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_vector_string_util.cpp
 * @brief Unit tests of VectorStringUtil.
 *
 * Covers join with empty, single and multi character separators, and split with missing,
 * consecutive, leading and trailing separators.
 */

#include "act_text/vector_string_util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace act::text::VectorStringUtil
{
namespace
{

    using ::testing::ElementsAre;
    using ::testing::IsEmpty;

    TEST(VectorStringUtilTest, JoinOfEmptyVectorIsEmpty)
    {
        EXPECT_EQ(join({}), "");
        EXPECT_EQ(join({}, ","), "");
    }

    TEST(VectorStringUtilTest, JoinOfSingleElementHasNoSeparator)
    {
        EXPECT_EQ(join({"alone"}, ","), "alone");
    }

    TEST(VectorStringUtilTest, JoinInsertsTheSeparatorBetweenElements)
    {
        EXPECT_EQ(join({"a", "b", "c"}, ","), "a,b,c");
        EXPECT_EQ(join({"a", "b", "c"}, " | "), "a | b | c");
    }

    TEST(VectorStringUtilTest, JoinWithoutSeparatorConcatenates)
    {
        EXPECT_EQ(join({"a", "b", "c"}), "abc");
    }

    TEST(VectorStringUtilTest, JoinKeepsEmptyElements)
    {
        EXPECT_EQ(join({"", "b", ""}, ","), ",b,");
    }

    TEST(VectorStringUtilTest, SplitOfEmptyStringIsEmpty)
    {
        EXPECT_THAT(split("", ","), IsEmpty());
    }

    TEST(VectorStringUtilTest, SplitWithoutSeparatorFoundGivesTheWholeString)
    {
        EXPECT_THAT(split("value", ","), ElementsAre("value"));
    }

    TEST(VectorStringUtilTest, SplitWithEmptySeparatorGivesTheWholeString)
    {
        EXPECT_THAT(split("abc", ""), ElementsAre("abc"));
        EXPECT_THAT(split("", ""), IsEmpty());
    }

    TEST(VectorStringUtilTest, SplitCutsOnEachSeparator)
    {
        EXPECT_THAT(split("a,b,c", ","), ElementsAre("a", "b", "c"));
    }

    TEST(VectorStringUtilTest, SplitKeepsEmptyFields)
    {
        EXPECT_THAT(split("a,,b", ","), ElementsAre("a", "", "b"));
        EXPECT_THAT(split(",a,", ","), ElementsAre("", "a", ""));
        EXPECT_THAT(split(",", ","), ElementsAre("", ""));
    }

    TEST(VectorStringUtilTest, SplitHandlesMultiCharacterSeparators)
    {
        EXPECT_THAT(split("a::b::c", "::"), ElementsAre("a", "b", "c"));
        EXPECT_THAT(split("a:b::c", "::"), ElementsAre("a:b", "c"));
    }

    TEST(VectorStringUtilTest, SplitReversesJoin)
    {
        const std::vector<std::string> values{"first", "", "third", "fourth"};

        EXPECT_EQ(split(join(values, ";"), ";"), values);
    }

} // namespace
} // namespace act::text::VectorStringUtil
