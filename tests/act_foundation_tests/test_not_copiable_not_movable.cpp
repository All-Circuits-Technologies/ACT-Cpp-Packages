// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_not_copiable_not_movable.cpp
 * @brief Unit tests of NotCopiableNotMovable.
 *
 * The class has no behavior: the checks are compile-time type traits on a class inheriting it.
 */

#include "act_foundation/not_copiable_not_movable.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace act::foundation
{
namespace
{

    class Resource : private NotCopiableNotMovable
    {
    };

    TEST(NotCopiableNotMovableTest, InheritingClassCanNeitherBeCopiedNorMoved)
    {
        static_assert(std::is_default_constructible_v<Resource>);
        static_assert(!std::is_copy_constructible_v<Resource>);
        static_assert(!std::is_move_constructible_v<Resource>);
        static_assert(!std::is_copy_assignable_v<Resource>);
        static_assert(!std::is_move_assignable_v<Resource>);
    }

    TEST(NotCopiableNotMovableTest, BaseCannotBeInstantiatedAlone)
    {
        static_assert(!std::is_default_constructible_v<NotCopiableNotMovable>);
    }

    TEST(NotCopiableNotMovableTest, BaseHasAPublicDestructor)
    {
        // Checked at run time, so that a protected destructor fails the test instead of the build
        EXPECT_TRUE(std::is_destructible_v<NotCopiableNotMovable>);
    }

} // namespace
} // namespace act::foundation
