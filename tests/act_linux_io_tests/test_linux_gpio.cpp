// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_linux_gpio.cpp
 * @brief Unit tests of LinuxGpio without a GPIO chip.
 *
 * Driving a real line needs a GPIO chip, so these tests only cover a chip which does not exist:
 * the GPIO is reported as not found and every operation fails without throwing.
 */

#include "act_linux_io/gpio/linux_gpio.hpp"
#include "act_logger/helpers/logger_helper.hpp"
#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

namespace act::linux_io
{
namespace
{

    /** @brief Name of a GPIO chip which no machine is expected to have */
    const std::string MISSING_CHIP_NAME{"act-linux-io-test-missing-chip"};

    class LinuxGpioTest : public ::testing::Test
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

    TEST_F(LinuxGpioTest, MissingChipIsReportedAsNotFound)
    {
        const LinuxGpio gpio(MISSING_CHIP_NAME, 0, getLogger());

        EXPECT_FALSE(gpio.found());
    }

    TEST_F(LinuxGpioTest, OperationsOnAMissingChipFail)
    {
        LinuxGpio gpio(MISSING_CHIP_NAME, 0, getLogger());

        EXPECT_FALSE(gpio.prepDirectionInput());
        EXPECT_FALSE(gpio.prepBiasPullUp());
        EXPECT_FALSE(gpio.prepActiveLow());
        EXPECT_FALSE(gpio.prepDebounce(std::chrono::milliseconds(1)));
        EXPECT_EQ(gpio.getName(), "");
        EXPECT_FALSE(gpio.getValue());
        EXPECT_FALSE(gpio.listenEvents([](bool) {}));
    }

} // namespace
} // namespace act::linux_io
