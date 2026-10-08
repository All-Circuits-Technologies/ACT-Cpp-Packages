// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_r_property.cpp
 * @brief Unit tests of RProperty.
 *
 * Covers reading: the store always comes first (no cache), an absent key falls back on the
 * default (evaluated lazily) or reads as nothing, a seed is never consulted on read, and a stored
 * value the codec cannot decode (another type tag, an unknown enum name) reads as nothing without
 * falling back on the default. Static assertions check that a read only descriptor cannot be
 * written nor copied, and that a provider or codec of another value type is rejected.
 */

#include "store_backed_manager.hpp"

#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/codecs/scalar_codec.hpp"
#include "act_property_core/properties/abs_property_registry.hpp"
#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"
#include "act_property_core/stored_value.hpp"

#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

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
    const std::string LAZY_KEY{"lazy"};
    const std::string SEEDED_KEY{"seeded"};
    const std::string WIDE_KEY{"wide"};
    const std::string LOCALE_KEY{"locale"};

    constexpr std::int32_t DEFAULT_VALUE = 50;
    constexpr std::int32_t SEED_VALUE = 3;
    constexpr std::int32_t STORED_VALUE = 7;

    /** @brief Manager declaring one read only descriptor of each kind */
    class ReadOnlyManager : public test::StoreBackedManager
    {
      public:
        ReadOnlyManager(act::logger::LoggerManager &logger,
                        AbsPropertyStore &store,
                        int &lazyDefaultCalls)
            : StoreBackedManager(logger, store),
              m_lazy(*this, LAZY_KEY, Default::from([&lazyDefaultCalls]() -> std::int32_t {
                  ++lazyDefaultCalls;
                  return DEFAULT_VALUE;
              }))
        {
        }

        [[nodiscard]] const RProperty<std::int32_t> &getPlain() const
        {
            return m_plain;
        }

        [[nodiscard]] const RProperty<std::int32_t> &getDefaulted() const
        {
            return m_defaulted;
        }

        [[nodiscard]] const RProperty<std::int32_t> &getLazy() const
        {
            return m_lazy;
        }

        [[nodiscard]] const RProperty<std::int32_t> &getSeeded() const
        {
            return m_seeded;
        }

        [[nodiscard]] const RProperty<std::int64_t> &getWide() const
        {
            return m_wide;
        }

        [[nodiscard]] const RProperty<Locale> &getLocale() const
        {
            return m_locale;
        }

      private:
        RProperty<std::int32_t> m_plain{*this, PLAIN_KEY};
        RProperty<std::int32_t> m_defaulted{*this, DEFAULTED_KEY, Default::of(DEFAULT_VALUE)};
        RProperty<std::int32_t> m_lazy;
        RProperty<std::int32_t> m_seeded{*this, SEEDED_KEY, Seed::of(SEED_VALUE)};
        RProperty<std::int64_t> m_wide{*this, WIDE_KEY};
        RProperty<Locale> m_locale{
            *this,
            LOCALE_KEY,
            Default::of(Locale::EN_GB),
            EnumCodec<Locale>({{Locale::EN_GB, "en_GB"}, {Locale::FR_FR, "fr_FR"}})};
    };

    template <class P, class V>
    concept SettableWith = requires(P &property, const V &value) { property.set(value); };

    // A read only descriptor exposes no write and cannot be duplicated
    static_assert(!SettableWith<RProperty<std::int32_t>, std::int32_t>);
    static_assert(!std::is_copy_constructible_v<RProperty<std::int32_t>>);
    static_assert(!std::is_move_constructible_v<RProperty<std::int32_t>>);

    // A provider or a codec must carry the exact value type of the descriptor
    static_assert(std::is_constructible_v<RProperty<std::uint8_t>,
                                          AbsPropertyRegistry &,
                                          std::string,
                                          DefaultProvider<std::uint8_t>>);
    static_assert(!std::is_constructible_v<RProperty<std::uint8_t>,
                                           AbsPropertyRegistry &,
                                           std::string,
                                           DefaultProvider<int>>);
    static_assert(!std::is_constructible_v<RProperty<std::string>,
                                           AbsPropertyRegistry &,
                                           std::string,
                                           DefaultProvider<const char *>>);
    static_assert(!std::is_constructible_v<RProperty<std::int64_t>,
                                           AbsPropertyRegistry &,
                                           std::string,
                                           SeedProvider<std::int32_t>>);
    static_assert(!std::is_constructible_v<RProperty<std::int64_t>,
                                           AbsPropertyRegistry &,
                                           std::string,
                                           ScalarCodec<std::int32_t>>);

    class RPropertyTest : public test::StoreBackedManagerTest
    {
    };

    TEST_F(RPropertyTest, KeyIsTheDeclaredOne)
    {
        int lazyCalls = 0;
        const ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);

        EXPECT_EQ(manager.getPlain().getKey(), PLAIN_KEY);
        EXPECT_EQ(manager.getLocale().getKey(), LOCALE_KEY);
    }

    TEST_F(RPropertyTest, GetReadsTheStoredValue)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().set(PLAIN_KEY, StoredValue(STORED_VALUE)));

        EXPECT_EQ(manager.getPlain().get(), std::optional<std::int32_t>(STORED_VALUE));
    }

    TEST_F(RPropertyTest, GetAlwaysReadsTheStore)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());

        ASSERT_TRUE(getStore().set(PLAIN_KEY, StoredValue(STORED_VALUE)));
        EXPECT_EQ(manager.getPlain().get(), std::optional<std::int32_t>(STORED_VALUE));

        ASSERT_TRUE(getStore().set(PLAIN_KEY, StoredValue(STORED_VALUE + 1)));
        EXPECT_EQ(manager.getPlain().get(), std::optional<std::int32_t>(STORED_VALUE + 1));

        ASSERT_TRUE(getStore().erase(PLAIN_KEY));
        EXPECT_EQ(manager.getPlain().get(), std::nullopt);
    }

    TEST_F(RPropertyTest, AbsentKeyWithoutProviderReadsAsNothing)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.getPlain().get(), std::nullopt);
    }

    TEST_F(RPropertyTest, AbsentKeyReadsAsTheDefault)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.getDefaulted().get(), std::optional<std::int32_t>(DEFAULT_VALUE));
        EXPECT_EQ(manager.getLocale().get(), std::optional<Locale>(Locale::EN_GB));
    }

    TEST_F(RPropertyTest, StoredValueTakesPrecedenceOverTheDefault)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().set(DEFAULTED_KEY, StoredValue(STORED_VALUE)));

        EXPECT_EQ(manager.getDefaulted().get(), std::optional<std::int32_t>(STORED_VALUE));
    }

    TEST_F(RPropertyTest, FunctionDefaultIsEvaluatedOnlyWhenTheKeyIsAbsent)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        EXPECT_EQ(lazyCalls, 0);

        ASSERT_TRUE(getStore().set(LAZY_KEY, StoredValue(STORED_VALUE)));
        EXPECT_EQ(manager.getLazy().get(), std::optional<std::int32_t>(STORED_VALUE));
        EXPECT_EQ(lazyCalls, 0);

        ASSERT_TRUE(getStore().erase(LAZY_KEY));
        EXPECT_EQ(manager.getLazy().get(), std::optional<std::int32_t>(DEFAULT_VALUE));
        EXPECT_EQ(lazyCalls, 1);
    }

    TEST_F(RPropertyTest, SeededValueIsReadAfterInit)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.getSeeded().get(), std::optional<std::int32_t>(SEED_VALUE));
    }

    TEST_F(RPropertyTest, SeedIsNotConsultedOnRead)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);

        // Before init, the seed is not in the store yet
        EXPECT_EQ(manager.getSeeded().get(), std::nullopt);

        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().erase(SEEDED_KEY));
        EXPECT_EQ(manager.getSeeded().get(), std::nullopt);
    }

    TEST_F(RPropertyTest, ValueStoredWithAnotherTypeReadsAsNothing)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().set(WIDE_KEY, StoredValue(STORED_VALUE)));

        EXPECT_EQ(manager.getWide().get(), std::nullopt);
    }

    TEST_F(RPropertyTest, UndecodableValueDoesNotFallBackOnTheDefault)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().set(DEFAULTED_KEY, StoredValue(std::string("seven"))));
        ASSERT_TRUE(getStore().set(LOCALE_KEY, StoredValue(std::string("de_DE"))));

        EXPECT_EQ(manager.getDefaulted().get(), std::nullopt);
        EXPECT_EQ(manager.getLocale().get(), std::nullopt);
    }

    TEST_F(RPropertyTest, EnumPropertyDecodesTheStoredName)
    {
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());
        ASSERT_TRUE(getStore().set(LOCALE_KEY, StoredValue(std::string("fr_FR"))));

        EXPECT_EQ(manager.getLocale().get(), std::optional<Locale>(Locale::FR_FR));
    }

    TEST_F(RPropertyTest, GetOrGivesTheReadValueOrTheCallerFallback)
    {
        constexpr std::int32_t callerFallback = -1;
        int lazyCalls = 0;
        ReadOnlyManager manager(getLoggerManager(), getStore(), lazyCalls);
        ASSERT_TRUE(manager.init());

        EXPECT_EQ(manager.getPlain().getOr(callerFallback), callerFallback);
        EXPECT_EQ(manager.getDefaulted().getOr(callerFallback), DEFAULT_VALUE);

        ASSERT_TRUE(getStore().set(PLAIN_KEY, StoredValue(STORED_VALUE)));
        EXPECT_EQ(manager.getPlain().getOr(callerFallback), STORED_VALUE);
    }

} // namespace
} // namespace act::property
