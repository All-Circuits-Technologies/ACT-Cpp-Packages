// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_linux_io/led/trigger/abs_led_trigger_config.hpp"

#include "act_linux_io/led/linux_led.hpp"

#include <optional>

namespace act::linux_io
{

/* # Constructors */

AbsLedTriggerConfig::AbsLedTriggerConfig(std::string name)
    : m_name(std::move(name))
{
}

/* # Methods */

bool AbsLedTriggerConfig::isLedAlreadyConfigured(const LinuxLed &led) const
{
    return led.hasAnyTrigger() && (led.getTrigger().value() == m_name);
}

bool AbsLedTriggerConfig::applyToLeds(const std::vector<std::reference_wrapper<LinuxLed>> &leds,
                                      bool force) const
{
    bool success = true;

    // Prepare all LEDs first
    for (const auto &ledRef : leds)
    {
        success &= prepareLed(ledRef.get(), force);
    }

    // Then fire trigger over all LEDs
    for (const auto &ledRef : leds)
    {
        success &= fireOnLed(ledRef.get());
    }

    return success;
}

bool AbsLedTriggerConfig::prepareLed(LinuxLed &led, bool force) const
{
    return led.setTrigger(m_name, force) && prepareLedExtra(led);
}

} // namespace act::linux_io
