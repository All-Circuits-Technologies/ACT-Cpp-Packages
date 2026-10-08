// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_rw_property.cpp
 * @brief Unit tests of RWProperty.
 *
 * Covers writing (the encoded value reaches the store, takes precedence over the default, and a
 * store failure is reported) and resetting (to the default, to the seed, or by removing the key
 * when there is no provider). A read and write descriptor is also usable through a read only
 * reference, and a runtime write of a seeded descriptor is realigned at the next start.
 */

#include "store_backed_manager.hpp"

#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/properties/rw_property.hpp"
#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"
#include "act_property_core/stored_value.hpp"
#include "act_property_core/types/stored_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

namespace act::property
{
namespace
{

    enum class Locale
    {
        EN_GB,
        FR_FR,
    };

    const std::string PLAIN_KEY{"plain"};
    const std::string DEFAULTED_KEY{"defaulted"};
    const std::string SEEDED_KEY{"seeded"};
    const std::string LOCALE_KEY{"locale"};

    constexpr std::int32_t DEFAULT_VALUE = 50;
    constexpr std::int32_t SEED_VALUE = 3;
    constexpr std::int32_t WRITTEN_VALUE = 7;

    /** @brief Manager declaring one read and write descriptor of each kind */
    class ReadWriteManager : public test::StoreBackedManager
    {
      public:
        using StoreBackedManager::StoreBackedManager;

        [[nodiscard]] RWProperty<std::int32_t> &getPlain()
        {
            return m_plain;
        }

        [[nodiscard]] RWProperty<std::int32_t> &getDefaulted()
        {
            return m_defaulted;
        }

        [[nodiscard]] RWProperty<std::int32_t> &getSeeded()
        {
            return m_seeded;
        }

        [[nodiscard]] RWProperty<Locale> &getLocale()
        {
            return m_locale;
        }

      private:
        RWProperty<std::int32_t> m_plain{*this, PLAIN_KEY};
        RWProperty<std::int32_t> m_defaulted{*this, DEFAULTED_KEY, Default::of(DEFAULT_VALUE)};
        RWProperty<std::int32_t> m_seeded{*this, SEEDED_KEY, Seed::of(SEED_VALUE)};
        RWProperty<Locale> m_locale{
            *this,
            LOCALE_KEY,
            Default::of(Locale::EN_GB),
            EnumCodec<Locale>({{Locale::EN_GB, "en_GB"}, {Locale::FR_FR, "fr_FR"}})};
    };

    class RWPropertyTest : public test::StoreBackedManagerTest
    {
    };

    TEST_F(RWPropertyTest, SetWritesTheEncodedValue)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());

        ASSERT_TRUE(manager.getPlain().set(WRITTEN_VALUE));
        ASSERT_TRUE(manager.getLocale().set(Locale::FR_FR));

        EXPECT_EQ(getStore().get(PLAIN_KEY),
                  std::optional<StoredValue>(StoredValue(WRITTEN_VALUE)));
        EXPECT_EQ(getStore().get(LOCALE_KEY),
                  std::optional<StoredValue>(StoredValue(std::string("fr_FR"))));
    }

    TEST_F(RWPropertyTest, WrittenValueIsReadBack)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());

        ASSERT_TRUE(manager.getPlain().set(WRITTEN_VALUE));
        ASSERT_TRUE(manager.getLocale().set(Locale::FR_FR));

        EXPECT_EQ(manager.getPlain().get(), std::optional<std::int32_t>(WRITTEN_VALUE));
        EXPECT_EQ(manager.getLocale().get(), std::optional<Locale>(Locale::FR_FR));
    }

    TEST_F(RWPropertyTest, WrittenValueTakesPrecedenceOverTheDefault)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());

        ASSERT_TRUE(manager.getDefaulted().set(WRITTEN_VALUE));

        EXPECT_EQ(manager.getDefaulted().get(), std::optional<std::int32_t>(WRITTEN_VALUE));
    }

    TEST_F(RWPropertyTest, SetFailureIsReportedAndLeavesTheStoreUnchanged)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        getStore().setFailSet(true);

        EXPECT_FALSE(manager.getDefaulted().set(WRITTEN_VALUE));

        EXPECT_EQ(manager.getDefaulted().get(), std::optional<std::int32_t>(DEFAULT_VALUE));
    }

    TEST_F(RWPropertyTest, ResetWritesTheDefault)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(manager.getDefaulted().set(WRITTEN_VALUE));

        ASSERT_TRUE(manager.getDefaulted().reset());

        EXPECT_EQ(getStore().get(DEFAULTED_KEY),
                  std::optional<StoredValue>(StoredValue(DEFAULT_VALUE)));
    }

    TEST_F(RWPropertyTest, ResetWritesTheSeed)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(manager.getSeeded().set(WRITTEN_VALUE));

        ASSERT_TRUE(manager.getSeeded().reset());

        EXPECT_EQ(manager.getSeeded().get(), std::optional<std::int32_t>(SEED_VALUE));
    }

    TEST_F(RWPropertyTest, ResetWithoutProviderRemovesTheKey)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(manager.getPlain().set(WRITTEN_VALUE));

        ASSERT_TRUE(manager.getPlain().reset());

        EXPECT_EQ(getStore().get(PLAIN_KEY), std::nullopt);
        EXPECT_EQ(manager.getPlain().get(), std::nullopt);
        EXPECT_EQ(getStore().getEraseCount(), 1);
    }

    TEST_F(RWPropertyTest, ResetFailureIsReported)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(manager.getPlain().set(WRITTEN_VALUE));

        getStore().setFailErase(true);
        EXPECT_FALSE(manager.getPlain().reset());
        EXPECT_EQ(manager.getPlain().get(), std::optional<std::int32_t>(WRITTEN_VALUE));

        getStore().setFailSet(true);
        EXPECT_FALSE(manager.getDefaulted().reset());
    }

    TEST_F(RWPropertyTest, IsReadableThroughAReadOnlyReference)
    {
        ReadWriteManager manager(getLoggerManager(), getStore());
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(manager.getPlain().set(WRITTEN_VALUE));

        const RProperty<std::int32_t> &readOnly = manager.getPlain();

        EXPECT_EQ(readOnly.get(), std::optional<std::int32_t>(WRITTEN_VALUE));
    }

    TEST_F(RWPropertyTest, RuntimeWriteOfASeededPropertyIsRealignedAtTheNextStart)
    {
        {
            ReadWriteManager firstStart(getLoggerManager(), getStore());
            ASSERT_TRUE(firstStart.init());
            ASSERT_TRUE(firstStart.getSeeded().set(WRITTEN_VALUE));
            ASSERT_EQ(firstStart.getSeeded().get(), std::optional<std::int32_t>(WRITTEN_VALUE));
        }

        ReadWriteManager nextStart(getLoggerManager(), getStore());
        ASSERT_TRUE(nextStart.init());

        EXPECT_EQ(nextStart.getSeeded().get(), std::optional<std::int32_t>(SEED_VALUE));
    }

} // namespace
} // namespace act::property
