// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_linux_led.cpp
 * @brief Unit tests of LinuxLed.
 *
 * LinuxLed always works on the LEDs sysfs folder, which cannot be relocated, so these tests drive
 * an LED whose folder does not exist and check the documented failure values: every read falls
 * back to its "off" value, every write fails, and the cached trigger is only updated by a write
 * which succeeded.
 */

#include "act_linux_io/led/linux_led.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

namespace act::linux_io
{
namespace
{

    /** @brief Name of an LED which no machine is expected to have */
    const std::string MISSING_LED_NAME{"act-linux-io-test-missing-led"};

    class LinuxLedTest : public ::testing::Test
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

    TEST_F(LinuxLedTest, KeepsTheGivenName)
    {
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_EQ(led.getName(), MISSING_LED_NAME);
    }

    TEST_F(LinuxLedTest, HasNoTriggerBeforeAnyUse)
    {
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_EQ(led.getTrigger(), std::nullopt);
        EXPECT_FALSE(led.hasAnyTrigger());
    }

    TEST_F(LinuxLedTest, ReadsFallBackToOffWhenTheLedIsMissing)
    {
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_EQ(led.getBrightness(), 0U);
        EXPECT_EQ(led.getMaxBrightness(), 0U);
        EXPECT_FALSE(led.getState());
    }

    TEST_F(LinuxLedTest, ConfReadsAreEmptyWhenTheLedIsMissing)
    {
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_EQ(led.readConfString("trigger"), std::nullopt);
        EXPECT_EQ(led.readConfInt("brightness"), std::nullopt);
        EXPECT_EQ(led.readConfUInt("brightness"), std::nullopt);
    }

    TEST_F(LinuxLedTest, ConfWritesFailWhenTheLedIsMissing)
    {
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(led.writeConfString("trigger", "none"));
        EXPECT_FALSE(led.writeConfInt("repeat", -1));
        EXPECT_FALSE(led.writeConfUInt("brightness", 1));
    }

    TEST_F(LinuxLedTest, StateAndBrightnessWritesFailWhenTheLedIsMissing)
    {
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(led.setState(true));
        EXPECT_FALSE(led.setState(false));
        EXPECT_FALSE(led.setBrightness(1));

        // Clearing the trigger is the first step of both, and it failed
        EXPECT_EQ(led.getTrigger(), std::nullopt);
    }

    TEST_F(LinuxLedTest, FailedTriggerWriteLeavesTheCachedTriggerUntouched)
    {
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(led.setTrigger("heartbeat", false));
        EXPECT_FALSE(led.setTrigger("heartbeat", true));
        EXPECT_FALSE(led.clearTrigger());

        EXPECT_EQ(led.getTrigger(), std::nullopt);
        EXPECT_FALSE(led.hasAnyTrigger());
    }

} // namespace
} // namespace act::linux_io
