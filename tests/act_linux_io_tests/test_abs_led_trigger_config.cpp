// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_abs_led_trigger_config.cpp
 * @brief Unit tests of the AbsLedTriggerConfig algorithms.
 *
 * A recording trigger configuration counts the calls of its hooks, and drives LEDs whose sysfs
 * folder does not exist, so selecting the trigger always fails. The tests check how applying a
 * trigger chains its steps: a single LED is only fired once prepared, several LEDs are all
 * prepared then all fired whatever the outcome, and an LED already configured is left untouched.
 */

#include "act_linux_io/led/linux_led.hpp"
#include "act_linux_io/led/trigger/abs_led_trigger_config.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace act::linux_io
{
namespace
{

    /** @brief Name of an LED which no machine is expected to have */
    const std::string MISSING_LED_NAME{"act-linux-io-test-missing-led"};

    /** @brief Trigger configuration recording the calls of its hooks */
    class RecordingTriggerConfig : public AbsLedTriggerConfig
    {
      public:
        explicit RecordingTriggerConfig(bool alreadyConfigured = false)
            : AbsLedTriggerConfig("timer"),
              m_alreadyConfigured(alreadyConfigured)
        {
        }

        [[nodiscard]] bool isLedAlreadyConfigured(const LinuxLed &led) const override
        {
            return m_alreadyConfigured || AbsLedTriggerConfig::isLedAlreadyConfigured(led);
        }

        bool fireOnLed(LinuxLed & /*led*/) const override
        {
            ++m_fireCount;
            return true;
        }

        [[nodiscard]] int getPrepareExtraCount() const
        {
            return m_prepareExtraCount;
        }

        [[nodiscard]] int getFireCount() const
        {
            return m_fireCount;
        }

      protected:
        bool prepareLedExtra(LinuxLed & /*led*/) const override
        {
            ++m_prepareExtraCount;
            return true;
        }

      private:
        bool m_alreadyConfigured;
        mutable int m_prepareExtraCount{0};
        mutable int m_fireCount{0};
    };

    class AbsLedTriggerConfigTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_TRUE(m_loggerManager.init());
            m_logger = m_loggerManager.createSubLogger("test");
        }

        [[nodiscard]] act::logger::LoggerHelper &getLogger() const
        {
            return *m_logger;
        }

      private:
        act::logger::LoggerManager m_loggerManager;
        std::shared_ptr<act::logger::LoggerHelper> m_logger;
    };

    TEST_F(AbsLedTriggerConfigTest, LedWithoutTriggerIsNotConfigured)
    {
        const RecordingTriggerConfig config;
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.isLedAlreadyConfigured(led));
        EXPECT_FALSE(led.hasTrigger(config));
    }

    TEST_F(AbsLedTriggerConfigTest, PrepareStopsWhenTheTriggerCannotBeSelected)
    {
        const RecordingTriggerConfig config;
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.prepareLed(led, false));
        EXPECT_FALSE(config.prepareLed(led, true));
        EXPECT_EQ(config.getPrepareExtraCount(), 0);
    }

    TEST_F(AbsLedTriggerConfigTest, ApplyToLedDoesNotFireAnUnpreparedLed)
    {
        const RecordingTriggerConfig config;
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.applyToLed(led));
        EXPECT_FALSE(led.setTrigger(config));
        EXPECT_EQ(config.getFireCount(), 0);
    }

    TEST_F(AbsLedTriggerConfigTest, ApplyToLedsFiresEveryLedEvenWhenPreparingFailed)
    {
        const RecordingTriggerConfig config;
        LinuxLed firstLed(MISSING_LED_NAME, getLogger());
        LinuxLed secondLed(MISSING_LED_NAME, getLogger());
        const std::vector<std::reference_wrapper<LinuxLed>> leds{firstLed, secondLed};

        EXPECT_FALSE(config.applyToLeds(leds));
        EXPECT_EQ(config.getPrepareExtraCount(), 0);
        EXPECT_EQ(config.getFireCount(), 2);
    }

    TEST_F(AbsLedTriggerConfigTest, ApplyToNoLedSucceeds)
    {
        const RecordingTriggerConfig config;

        EXPECT_TRUE(config.applyToLeds({}));
        EXPECT_EQ(config.getFireCount(), 0);
    }

    TEST_F(AbsLedTriggerConfigTest, AlreadyConfiguredLedIsLeftUntouched)
    {
        const RecordingTriggerConfig config(true);
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_TRUE(config.applyToLedIfNotAlreadyConfigured(led));
        EXPECT_TRUE(led.setTriggerIfNotAlreadyConfigured(config));
        EXPECT_EQ(config.getPrepareExtraCount(), 0);
        EXPECT_EQ(config.getFireCount(), 0);
        EXPECT_EQ(led.getTrigger(), std::nullopt);
    }

    TEST_F(AbsLedTriggerConfigTest, NotConfiguredLedIsApplied)
    {
        const RecordingTriggerConfig config;
        LinuxLed led(MISSING_LED_NAME, getLogger());

        // The trigger cannot be selected on a missing LED, which proves it was attempted
        EXPECT_FALSE(config.applyToLedIfNotAlreadyConfigured(led));
        EXPECT_FALSE(led.setTriggerIfNotAlreadyConfigured(config));
    }

} // namespace
} // namespace act::linux_io
