// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_with_life_cycle.cpp
 * @brief Unit tests of AbsWithLifeCycle and of its AbsManager and AbsService subclasses.
 *
 * Covers the dispatch of init() through the life cycle interface, and the copy and move ban that
 * managers and services inherit.
 */

#include "act_foundation/services/abs_manager.hpp"
#include "act_foundation/services/abs_service.hpp"
#include "act_foundation/services/abs_with_life_cycle.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace act::foundation
{
namespace
{

    class FakeManager : public AbsManager
    {
      public:
        explicit FakeManager(bool initResult)
            : m_initResult(initResult)
        {
        }

        bool init() override
        {
            ++m_initCalls;
            return m_initResult;
        }

        [[nodiscard]] int getInitCalls() const
        {
            return m_initCalls;
        }

      private:
        bool m_initResult;
        int m_initCalls{0};
    };

    class FakeService : public AbsService
    {
      public:
        bool init() override
        {
            m_initialized = true;
            return true;
        }

        [[nodiscard]] bool isInitialized() const
        {
            return m_initialized;
        }

      private:
        bool m_initialized{false};
    };

    TEST(AbsWithLifeCycleTest, InitIsDispatchedToTheManager)
    {
        FakeManager manager(true);
        AbsWithLifeCycle &lifeCycle = manager;

        EXPECT_TRUE(lifeCycle.init());
        EXPECT_EQ(manager.getInitCalls(), 1);
    }

    TEST(AbsWithLifeCycleTest, InitFailureIsReportedToTheCaller)
    {
        FakeManager manager(false);
        AbsWithLifeCycle &lifeCycle = manager;

        EXPECT_FALSE(lifeCycle.init());
    }

    TEST(AbsWithLifeCycleTest, InitIsDispatchedToTheService)
    {
        FakeService service;
        AbsWithLifeCycle &lifeCycle = service;

        EXPECT_TRUE(lifeCycle.init());
        EXPECT_TRUE(service.isInitialized());
    }

    TEST(AbsWithLifeCycleTest, ManagersAndServicesCanNeitherBeCopiedNorMoved)
    {
        static_assert(!std::is_copy_constructible_v<FakeManager>);
        static_assert(!std::is_move_constructible_v<FakeManager>);
        static_assert(!std::is_copy_assignable_v<FakeManager>);
        static_assert(!std::is_move_assignable_v<FakeManager>);

        static_assert(!std::is_copy_constructible_v<FakeService>);
        static_assert(!std::is_move_constructible_v<FakeService>);
        static_assert(!std::is_copy_assignable_v<FakeService>);
        static_assert(!std::is_move_assignable_v<FakeService>);
    }

    TEST(AbsWithLifeCycleTest, ManagersAndServicesHavePublicDestructors)
    {
        // Checked at run time, so that a protected destructor fails the test instead of the build
        EXPECT_TRUE(std::is_destructible_v<AbsWithLifeCycle>);
        EXPECT_TRUE(std::is_destructible_v<AbsManager>);
        EXPECT_TRUE(std::is_destructible_v<AbsService>);
    }

    TEST(AbsWithLifeCycleTest, ManagersAndServicesAreLifeCycleEntities)
    {
        static_assert(std::is_base_of_v<AbsWithLifeCycle, AbsManager>);
        static_assert(std::is_base_of_v<AbsWithLifeCycle, AbsService>);
        static_assert(std::is_abstract_v<AbsManager>);
        static_assert(std::is_abstract_v<AbsService>);
    }

} // namespace
} // namespace act::foundation
