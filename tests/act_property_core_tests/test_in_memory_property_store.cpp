// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_in_memory_property_store.cpp
 * @brief Unit tests of InMemoryPropertyStore.
 *
 * Covers the AbsPropertyStore contract on the in-memory implementation: absent keys, upserts
 * (including a change of type tag), erasing (an absent key included) and clearing every key.
 */

#include "act_property_core/stored_value.hpp"
#include "act_property_core/stores/in_memory_property_store.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

namespace act::property
{
namespace
{

    static_assert(!std::is_copy_constructible_v<InMemoryPropertyStore>);
    static_assert(!std::is_move_constructible_v<InMemoryPropertyStore>);

    class InMemoryPropertyStoreTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_store.init());
        }

        [[nodiscard]] InMemoryPropertyStore &getStore()
        {
            return m_store;
        }

      private:
        InMemoryPropertyStore m_store;
    };

    TEST_F(InMemoryPropertyStoreTest, AbsentKeyReadsAsNothing)
    {
        EXPECT_EQ(getStore().get("missing"), std::nullopt);
    }

    TEST_F(InMemoryPropertyStoreTest, SetValueIsReadBackWithItsTag)
    {
        ASSERT_TRUE(getStore().set("key", StoredValue(std::uint16_t{12})));

        const std::optional<StoredValue> raw = getStore().get("key");

        ASSERT_TRUE(raw.has_value());
        EXPECT_EQ(raw->type(), StoredType::UINT16);
        EXPECT_EQ(raw->asUInt16(), std::optional<std::uint16_t>(12));
    }

    TEST_F(InMemoryPropertyStoreTest, SetReplacesTheValueAndItsTag)
    {
        ASSERT_TRUE(getStore().set("key", StoredValue(std::int32_t{1})));
        ASSERT_TRUE(getStore().set("key", StoredValue(std::string("one"))));

        EXPECT_EQ(getStore().get("key"),
                  std::optional<StoredValue>(StoredValue(std::string("one"))));
    }

    TEST_F(InMemoryPropertyStoreTest, KeysAreIndependent)
    {
        ASSERT_TRUE(getStore().set("a", StoredValue(true)));
        ASSERT_TRUE(getStore().set("b", StoredValue(false)));

        EXPECT_EQ(getStore().get("a"), std::optional<StoredValue>(StoredValue(true)));
        EXPECT_EQ(getStore().get("b"), std::optional<StoredValue>(StoredValue(false)));
        EXPECT_EQ(getStore().get("A"), std::nullopt);
    }

    TEST_F(InMemoryPropertyStoreTest, EraseRemovesOnlyTheKey)
    {
        ASSERT_TRUE(getStore().set("erased", StoredValue(true)));
        ASSERT_TRUE(getStore().set("kept", StoredValue(true)));

        EXPECT_TRUE(getStore().erase("erased"));

        EXPECT_EQ(getStore().get("erased"), std::nullopt);
        EXPECT_TRUE(getStore().get("kept").has_value());
    }

    TEST_F(InMemoryPropertyStoreTest, EraseOfAnAbsentKeySucceeds)
    {
        EXPECT_TRUE(getStore().erase("missing"));
    }

    TEST_F(InMemoryPropertyStoreTest, ClearAllRemovesEveryKey)
    {
        ASSERT_TRUE(getStore().set("a", StoredValue(true)));
        ASSERT_TRUE(getStore().set("b", StoredValue(std::string("b"))));

        EXPECT_TRUE(getStore().clearAll());

        EXPECT_EQ(getStore().get("a"), std::nullopt);
        EXPECT_EQ(getStore().get("b"), std::nullopt);
        EXPECT_TRUE(getStore().clearAll());
    }

} // namespace
} // namespace act::property
