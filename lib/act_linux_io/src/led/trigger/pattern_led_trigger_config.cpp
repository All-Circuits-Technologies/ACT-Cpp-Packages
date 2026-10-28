// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_linux_io/led/trigger/pattern_led_trigger_config.hpp"

#include "act_linux_io/led/linux_led.hpp"

#include <numeric>

namespace act::linux_io
{

const int PatternLedTriggerConfig::REPEAT_INFINITE = -1;
const std::string PatternLedTriggerConfig::PATTERN_TRIGGER_NAME = "pattern";
const std::string PatternLedTriggerConfig::PATTERN_FILE_NAME = "pattern";
const std::string PatternLedTriggerConfig::REPEAT_FILE_NAME = "repeat";

/* # Constructors */

PatternLedTriggerConfig::PatternLedTriggerConfig(Pattern pattern, int repeatCount)
    : AbsLedTriggerConfig(PATTERN_TRIGGER_NAME),
      m_pattern(std::move(pattern)),
      m_repeatCount(repeatCount)
{
}

/* # Methods */

bool PatternLedTriggerConfig::isLedAlreadyConfigured(const LinuxLed &led) const
{
    return AbsLedTriggerConfig::isLedAlreadyConfigured(led) &&
           (led.readConfString(PATTERN_FILE_NAME) == getPatternString()) &&
           (led.readConfInt(REPEAT_FILE_NAME) == m_repeatCount);
}

void PatternLedTriggerConfig::setPattern(const Pattern &pattern)
{
    m_pattern = pattern;

    // Invalidate cache
    m_patternStringCache.clear();
}

bool PatternLedTriggerConfig::fireOnLed(LinuxLed &led) const
{
    return led.writeConfString(PATTERN_FILE_NAME, getPatternString());
}

bool PatternLedTriggerConfig::prepareLedExtra(LinuxLed &led) const
{
    return led.writeConfInt(REPEAT_FILE_NAME, m_repeatCount);
}

std::string PatternLedTriggerConfig::getPatternString() const
{
    if (m_patternStringCache.empty())
    {
        m_patternStringCache = computePatternString();
    }

    return m_patternStringCache;
}

std::string PatternLedTriggerConfig::computePatternString() const
{
    return std::accumulate(m_pattern.begin(),
                           m_pattern.end(),
                           std::string{},
                           [](const std::string &acc, const PatternStep &step) {
                               return acc + (acc.empty() ? "" : " ") + std::to_string(step.first) +
                                      " " + std::to_string(step.second.count());
                           });
}

} // namespace act::linux_io
