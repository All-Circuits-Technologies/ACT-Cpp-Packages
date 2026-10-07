// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_property_manager.cpp
 * @brief Unit tests of AbsPropertyManager.
 *
 * Covers init: it initializes the store, writes the seed of every registered descriptor when the
 * key is absent, realigns it when the stored value differs (another value or another type tag),
 * skips the write when it is already aligned, never writes a default, and reports a store which
 * cannot be initialized or written. Building a second manager on the same store simulates the
 * next start of an application. Static assertions check that outside code can neither reach the
 * store nor register or seed a descriptor.
 */

#include "recording_property_store.hpp"
#include "store_backed_manager.hpp"

#include "act_property_core/properties/abs_property_registry.hpp"
#include "act_property_core/properties/abs_registered_property.hpp"
#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/properties/rw_property.hpp"
#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace act::property
{
namespace
{

    const std::string RETRIES_KEY{"net.retries"};
    const std::string NAME_KEY{"device.name"};
    const std::string BRIGHTNESS_KEY{"ui.brightness"};
    const std::string LAST_USER_KEY{"session.lastUser"};

    constexpr std::int32_t RETRIES_SEED = 3;
    constexpr std::int32_t BRIGHTNESS_DEFAULT = 50;

    /** @brief Manager without any descriptor */
    class EmptyManager : public test::StoreBackedManager
    {
      public:
        using StoreBackedManager::StoreBackedManager;
    };

    /** @brief Manager with two seeded descriptors, whose seeds are chosen by the test */
    class SeededManager : public test::StoreBackedManager
    {
      public:
        SeededManager(act::logger::LoggerManager &logger,
                      AbsPropertyStore &store,
                      std::int32_t retries,
                      std::string name)
            : StoreBackedManager(logger, store),
              m_retries(*this, RETRIES_KEY, Seed::of(retries)),
              m_name(*this, NAME_KEY, Seed::of(std::move(name)))
        {
        }

        [[nodiscard]] const RProperty<std::int32_t> &getRetries() const
        {
            return m_retries;
        }

        [[nodiscard]] const RProperty<std::string> &getName() const
        {
            return m_name;
        }

      private:
        RProperty<std::int32_t> m_retries;
        RProperty<std::string> m_name;
    };

    /** @brief Manager with a default backed descriptor and a descriptor without provider */
    class DefaultManager : public test::StoreBackedManager
    {
      public:
        using StoreBackedManager::StoreBackedManager;

        [[nodiscard]] RWProperty<std::int32_t> &getBrightness()
        {
            return m_brightness;
        }

        [[nodiscard]] RWProperty<std::string> &getLastUser()
        {
            return m_lastUser;
        }

      private:
        RWProperty<std::int32_t> m_brightness{
            *this, BRIGHTNESS_KEY, Default::of(BRIGHTNESS_DEFAULT)};
        RWProperty<std::string> m_lastUser{*this, LAST_USER_KEY};
    };

    /** @brief Manager whose seed is produced by a function supplied by the test */
    class FunctionSeedManager : public test::StoreBackedManager
    {
      public:
        FunctionSeedManager(act::logger::LoggerManager &logger,
                            AbsPropertyStore &store,
                            std::function<std::int32_t()> seedFn)
            : StoreBackedManager(logger, store),
              m_retries(*this, RETRIES_KEY, Seed::from(std::move(seedFn)))
        {
        }

        [[nodiscard]] const RProperty<std::int32_t> &getRetries() const
        {
            return m_retries;
        }

      private:
        RProperty<std::int32_t> m_retries;
    };

    template <class M>
    concept CanAccessStore = requires(M &manager) { manager.accessStore(); };

    template <class P>
    concept CanSeed = requires(P &property) { property.seedIntoStore(); };

    // The capabilities are protected bases: only the manager and its descriptors see each other
    static_assert(!std::is_convertible_v<SeededManager &, AbsPropertyRegistry &>);
    static_assert(!CanAccessStore<SeededManager>);
    static_assert(!std::is_convertible_v<RProperty<std::int32_t> &, AbsRegisteredProperty &>);
    static_assert(!CanSeed<RProperty<std::int32_t>>);

    class AbsPropertyManagerTest : public test::StoreBackedManagerTest
    {
    };

    TEST_F(AbsPropertyManagerTest, InitInitializesTheStore)
    {
        EmptyManager manager(getLoggerManager(), getStore());

        EXPECT_TRUE(manager.init());
        EXPECT_EQ(getStore().getInitCount(), 1);
    }

    TEST_F(AbsPropertyManagerTest, InitFailsWhenTheStoreCannotBeInitialized)
    {
        getStore().setFailInit(true);
        SeededManager manager(getLoggerManager(), getStore(), RETRIES_SEED, "device");

        EXPECT_FALSE(manager.init());
        EXPECT_EQ(getStore().getSetCount(), 0);
    }

    TEST_F(AbsPropertyManagerTest, InitWritesTheSeedOfEveryAbsentKey)
    {
        SeededManager manager(getLoggerManager(), getStore(), RETRIES_SEED, "device");

        ASSERT_TRUE(manager.init());

        EXPECT_EQ(getStore().get(RETRIES_KEY),
                  std::optional<StoredValue>(StoredValue(RETRIES_SEED)));
        EXPECT_EQ(getStore().get(NAME_KEY),
                  std::optional<StoredValue>(StoredValue(std::string("device"))));
        EXPECT_EQ(manager.getRetries().get(), std::optional<std::int32_t>(RETRIES_SEED));
        EXPECT_EQ(manager.getName().get(), std::optional<std::string>("device"));
    }

    TEST_F(AbsPropertyManagerTest, InitSkipsTheWriteOfAnAlignedSeed)
    {
        {
            SeededManager firstStart(getLoggerManager(), getStore(), RETRIES_SEED, "device");
            ASSERT_TRUE(firstStart.init());
        }
        const int setCountAfterFirstStart = getStore().getSetCount();

        SeededManager nextStart(getLoggerManager(), getStore(), RETRIES_SEED, "device");
        ASSERT_TRUE(nextStart.init());

        EXPECT_EQ(getStore().getSetCount(), setCountAfterFirstStart);
    }

    TEST_F(AbsPropertyManagerTest, InitRealignsAChangedSeed)
    {
        constexpr std::int32_t newSeed = RETRIES_SEED + 1;
        {
            SeededManager firstStart(getLoggerManager(), getStore(), RETRIES_SEED, "device");
            ASSERT_TRUE(firstStart.init());
        }

        SeededManager nextStart(getLoggerManager(), getStore(), newSeed, "device");
        ASSERT_TRUE(nextStart.init());

        EXPECT_EQ(nextStart.getRetries().get(), std::optional<std::int32_t>(newSeed));
    }

    TEST_F(AbsPropertyManagerTest, InitRealignsAValueWrittenOutsideTheManager)
    {
        ASSERT_TRUE(getStore().set(RETRIES_KEY, StoredValue(RETRIES_SEED + 10)));

        SeededManager manager(getLoggerManager(), getStore(), RETRIES_SEED, "device");
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.getRetries().get(), std::optional<std::int32_t>(RETRIES_SEED));
    }

    TEST_F(AbsPropertyManagerTest, InitRealignsASeedStoredWithAnotherType)
    {
        // Same number, but with the type tag of another width
        ASSERT_TRUE(getStore().set(RETRIES_KEY, StoredValue(std::int64_t{RETRIES_SEED})));

        SeededManager manager(getLoggerManager(), getStore(), RETRIES_SEED, "device");
        ASSERT_TRUE(manager.init());

        const std::optional<StoredValue> raw = getStore().get(RETRIES_KEY);
        ASSERT_TRUE(raw.has_value());
        EXPECT_EQ(raw->type(), StoredType::INT32);
        EXPECT_EQ(manager.getRetries().get(), std::optional<std::int32_t>(RETRIES_SEED));
    }

    TEST_F(AbsPropertyManagerTest, InitFailsWhenASeedCannotBeWritten)
    {
        getStore().setFailSet(true);
        SeededManager manager(getLoggerManager(), getStore(), RETRIES_SEED, "device");

        EXPECT_FALSE(manager.init());
        EXPECT_EQ(getStore().get(RETRIES_KEY), std::nullopt);
    }

    TEST_F(AbsPropertyManagerTest, InitNeverWritesADefault)
    {
        DefaultManager manager(getLoggerManager(), getStore());

        ASSERT_TRUE(manager.init());

        EXPECT_EQ(getStore().getSetCount(), 0);
        EXPECT_EQ(getStore().get(BRIGHTNESS_KEY), std::nullopt);
        EXPECT_EQ(getStore().get(LAST_USER_KEY), std::nullopt);
        EXPECT_EQ(manager.getBrightness().get(), std::optional<std::int32_t>(BRIGHTNESS_DEFAULT));
    }

    TEST_F(AbsPropertyManagerTest, InitPreservesRuntimeWritesOfDefaultBackedProperties)
    {
        constexpr std::int32_t written = BRIGHTNESS_DEFAULT + 30;
        {
            DefaultManager firstStart(getLoggerManager(), getStore());
            ASSERT_TRUE(firstStart.init());
            ASSERT_TRUE(firstStart.getBrightness().set(written));
            ASSERT_TRUE(firstStart.getLastUser().set("operator"));
        }

        DefaultManager nextStart(getLoggerManager(), getStore());
        ASSERT_TRUE(nextStart.init());

        EXPECT_EQ(nextStart.getBrightness().get(), std::optional<std::int32_t>(written));
        EXPECT_EQ(nextStart.getLastUser().get(), std::optional<std::string>("operator"));
    }

    TEST_F(AbsPropertyManagerTest, FunctionSeedIsEvaluatedAtInitOnly)
    {
        int calls = 0;
        FunctionSeedManager manager(getLoggerManager(), getStore(), [&calls]() {
            ++calls;
            return RETRIES_SEED;
        });
        EXPECT_EQ(calls, 0);

        ASSERT_TRUE(manager.init());
        EXPECT_EQ(calls, 1);

        EXPECT_EQ(manager.getRetries().get(), std::optional<std::int32_t>(RETRIES_SEED));
        EXPECT_EQ(calls, 1);
    }

} // namespace
} // namespace act::property
