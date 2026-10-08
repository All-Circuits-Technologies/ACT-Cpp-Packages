// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_linux_io/led/trigger/oneshot_led_trigger_config.hpp"

#include "act_linux_io/led/linux_led.hpp"
#include <algorithm>
#include <chrono>

namespace act::linux_io
{

/* # Constructors */

OneShotLedTriggerConfig::OneShotLedTriggerConfig(std::chrono::milliseconds delayOn,
                                                 std::chrono::milliseconds delayOff,
                                                 bool invert)
    : AbsLedTriggerConfig(ONESHOT_TRIGGER_NAME),
      m_delayOn(delayOn),
      m_delayOff(std::max(delayOff, MIN_DELAY_OFF)),
      m_invert(invert)
{
}

bool OneShotLedTriggerConfig::isLedAlreadyConfigured(const LinuxLed &led) const
{
    return AbsLedTriggerConfig::isLedAlreadyConfigured(led) &&
           (led.readConfUInt(DELAY_ON_FILE_NAME).value_or(0) == m_delayOn.count()) &&
           (led.readConfUInt(DELAY_OFF_FILE_NAME).value_or(0) == m_delayOff.count()) &&
           (led.readConfInt(INVERT_FILE_NAME).value_or(0) == (m_invert ? 1 : 0));
}

bool OneShotLedTriggerConfig::fireOnLed(LinuxLed &led) const
{
    // Writing anything to the shot file, except an empty string, triggers the one-shot blink
    // once
    return led.writeConfString(SHOT_FILE_NAME, "x");
}

bool OneShotLedTriggerConfig::prepareLedExtra(LinuxLed &led) const
{
    return led.writeConfUInt(DELAY_ON_FILE_NAME, m_delayOn.count()) &&
           led.writeConfUInt(DELAY_OFF_FILE_NAME, m_delayOff.count()) &&
           led.writeConfInt(INVERT_FILE_NAME, m_invert ? 1 : 0);
}

} // namespace act::linux_io
