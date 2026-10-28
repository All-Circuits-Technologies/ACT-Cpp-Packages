// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_singleton.cpp
 * @brief Unit tests of Singleton and of its helper macros.
 *
 * Builds a singleton the way the macros intend (a static instance pointer, a creator and a
 * getter) and checks that it can neither be copied, moved nor built from outside.
 */

#include "act_foundation/singleton.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace act::foundation
{
namespace
{

    class Registry : public Singleton<Registry>
    {
      public:
        ~Registry() override = default;

      public:
        SINGLETON_PROTO_INSTANCE_CREATOR_VOID(Registry);
        SINGLETON_PROTO_INSTANCE_GETTER(Registry);

        /** @brief Delete the instance, so that each test starts without one */
        static void DestroyInstance()
        {
            delete _Instance;
            _Instance = nullptr;
        }

        [[nodiscard]] static bool HasInstance()
        {
            return _Instance != nullptr;
        }

      private:
        Registry() = default;

      private:
        SINGLETON_PROTO_INSTANCE_POINTER(Registry);
    };

    SINGLETON_IMPL_INSTANCE_POINTER(Registry);

    bool Registry::CreateInstance()
    {
        if (_Instance != nullptr)
        {
            return false;
        }

        _Instance = new Registry();
        return true;
    }

    Registry &Registry::Instance()
    {
        return *_Instance;
    }

    class SingletonTest : public ::testing::Test
    {
      protected:
        void TearDown() override
        {
            Registry::DestroyInstance();
        }
    };

    TEST_F(SingletonTest, CreatorBuildsTheUniqueInstance)
    {
        EXPECT_FALSE(Registry::HasInstance());

        ASSERT_TRUE(Registry::CreateInstance());

        EXPECT_TRUE(Registry::HasInstance());
        EXPECT_EQ(&Registry::Instance(), &Registry::Instance());
    }

    TEST_F(SingletonTest, SecondCreationIsRefused)
    {
        ASSERT_TRUE(Registry::CreateInstance());
        Registry *first = &Registry::Instance();

        EXPECT_FALSE(Registry::CreateInstance());
        EXPECT_EQ(&Registry::Instance(), first);
    }

    TEST_F(SingletonTest, InstanceCanNeitherBeBuiltCopiedNorMovedFromOutside)
    {
        static_assert(!std::is_default_constructible_v<Registry>);
        static_assert(!std::is_copy_constructible_v<Registry>);
        static_assert(!std::is_move_constructible_v<Registry>);
        static_assert(!std::is_copy_assignable_v<Registry>);
        static_assert(!std::is_move_assignable_v<Registry>);
    }

} // namespace
} // namespace act::foundation
