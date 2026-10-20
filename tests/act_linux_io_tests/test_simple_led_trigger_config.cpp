// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_simple_led_trigger_config.cpp
 * @brief Unit tests of SimpleLedTriggerConfig.
 *
 * The LEDs sysfs folder cannot be relocated, so the configuration drives an LED whose folder does
 * not exist: firing is a no-op which always succeeds, while selecting the trigger fails.
 */

#include "act_linux_io/led/linux_led.hpp"
#include "act_linux_io/led/trigger/simple_led_trigger_config.hpp"
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

    class SimpleLedTriggerConfigTest : public ::testing::Test
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

    TEST_F(SimpleLedTriggerConfigTest, FiringIsANoOpWhichSucceeds)
    {
        const SimpleLedTriggerConfig config("heartbeat");
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_TRUE(config.fireOnLed(led));
    }

    TEST_F(SimpleLedTriggerConfigTest, ApplyingFailsWhenTheLedIsMissing)
    {
        const SimpleLedTriggerConfig config(SimpleLedTriggerConfig::PANIC_TRIGGER_NAME);
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.isLedAlreadyConfigured(led));
        EXPECT_FALSE(config.applyToLed(led));
        EXPECT_EQ(led.getTrigger(), std::nullopt);
    }

    TEST_F(SimpleLedTriggerConfigTest, PanicTriggerNameMatchesTheLinuxOne)
    {
        EXPECT_EQ(SimpleLedTriggerConfig::PANIC_TRIGGER_NAME, "panic");
    }

} // namespace
} // namespace act::linux_io
