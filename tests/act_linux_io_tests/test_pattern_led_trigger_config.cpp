// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_pattern_led_trigger_config.cpp
 * @brief Unit tests of PatternLedTriggerConfig.
 *
 * The LEDs sysfs folder cannot be relocated, so the configuration drives an LED whose folder does
 * not exist: it is never reported as configured, and both firing and applying fail.
 */

#include "act_linux_io/led/linux_led.hpp"
#include "act_linux_io/led/trigger/pattern_led_trigger_config.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace act::linux_io
{
namespace
{

    /** @brief Name of an LED which no machine is expected to have */
    const std::string MISSING_LED_NAME{"act-linux-io-test-missing-led"};

    /** @brief Duration of a pattern step */
    constexpr std::chrono::milliseconds STEP_DURATION{100};

    class PatternLedTriggerConfigTest : public ::testing::Test
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

    TEST_F(PatternLedTriggerConfigTest, InfiniteRepeatIsTheLinuxValue)
    {
        EXPECT_EQ(PatternLedTriggerConfig::REPEAT_INFINITE, -1);
    }

    TEST_F(PatternLedTriggerConfigTest, MissingLedIsNotConfigured)
    {
        const PatternLedTriggerConfig config({{1, STEP_DURATION}},
                                             PatternLedTriggerConfig::REPEAT_INFINITE);
        const LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.isLedAlreadyConfigured(led));
    }

    TEST_F(PatternLedTriggerConfigTest, FiringAndApplyingFailWhenTheLedIsMissing)
    {
        PatternLedTriggerConfig config({{1, STEP_DURATION}, {0, STEP_DURATION}}, 2);
        config.setRepeatCount(PatternLedTriggerConfig::REPEAT_INFINITE);
        config.setPattern({{0, STEP_DURATION / 2}});
        LinuxLed led(MISSING_LED_NAME, getLogger());

        EXPECT_FALSE(config.fireOnLed(led));
        EXPECT_FALSE(config.applyToLed(led));
        EXPECT_EQ(led.getTrigger(), std::nullopt);
    }

} // namespace
} // namespace act::linux_io
