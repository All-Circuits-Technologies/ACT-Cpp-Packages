// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_linux_io/led/linux_led.hpp"

#include "act_files/file_util.hpp"
#include "act_linux_io/led/trigger/abs_led_trigger_config.hpp"
#include "act_logger/helpers/logger_helper.hpp"

namespace act::linux_io
{

const std::string LinuxLed::LEDS_DIR = "/sys/class/leds";
const std::string LinuxLed::BRIGHTNESS_FILE_NAME = "brightness";
const std::string LinuxLed::MAX_BRIGHTNESS_FILE_NAME = "max_brightness";
const std::string LinuxLed::TRIGGER_FILE_NAME = "trigger";
const std::string LinuxLed::TRIGGER_NONE_VALUE = "none";

/* # Constructors */

LinuxLed::LinuxLed(std::string ledName, act::logger::LoggerHelper &parentLogger)
    : m_ledName(std::move(ledName)),
      m_logger{parentLogger.createSubLogger(m_ledName)}
{
}

/* # Methods */

unsigned int LinuxLed::getBrightness() const
{
    return readConfUInt(BRIGHTNESS_FILE_NAME).value_or(0);
}

bool LinuxLed::setBrightness(unsigned int brightness)
{
    return clearTrigger() &&
           writeConfUInt(BRIGHTNESS_FILE_NAME, std::min(brightness, getMaxBrightness()));
}

unsigned int LinuxLed::getMaxBrightness() const
{
    if (m_maxBrightnessCache == 0)
    {
        // Not yet cached
        // Reminder: value 0 can't be a valid max brightness
        m_maxBrightnessCache = readConfUInt(MAX_BRIGHTNESS_FILE_NAME).value_or(0);
    }

    return m_maxBrightnessCache;
}

bool LinuxLed::hasTrigger(const AbsLedTriggerConfig &triggerConfig) const
{
    return triggerConfig.isLedAlreadyConfigured(*this);
}

bool LinuxLed::setTrigger(const std::string &trigger, bool force)
{
    if (!force && m_currentTriggerCache == trigger)
    {
        // Already set, no need to re-set
        return true;
    }

    const bool result = writeConfString(TRIGGER_FILE_NAME, trigger);
    if (result)
    {
        m_currentTriggerCache = trigger;
    }

    return result;
}

bool LinuxLed::setTrigger(const AbsLedTriggerConfig &triggerConfig, bool force)
{
    return triggerConfig.applyToLed(*this, force);
}

bool LinuxLed::setTriggerIfNotAlreadyConfigured(const AbsLedTriggerConfig &triggerConfig,
                                                bool force)
{
    return triggerConfig.applyToLedIfNotAlreadyConfigured(*this, force);
}

std::optional<std::string> LinuxLed::readConfString(const std::string &fileName) const
{
    return act::files::FileUtil::ReadFile(getConfFilePath(fileName), *m_logger);
}

std::optional<int> LinuxLed::readConfInt(const std::string &fileName) const
{
    return act::files::FileUtil::ReadFileAsInt(getConfFilePath(fileName), *m_logger);
}

std::optional<unsigned int> LinuxLed::readConfUInt(const std::string &fileName) const
{
    std::optional<int> optInt = readConfInt(fileName);

    if (!optInt.has_value())
    {
        // Log: already reported by readConfInt
        return std::nullopt;
    }

    if (optInt.value() < 0)
    {
        m_logger->errorStream() << "Negative value read from " << fileName;
        return std::nullopt;
    }

    return static_cast<unsigned int>(optInt.value());
}

bool LinuxLed::writeConfString(const std::string &fileName, const std::string &value)
{
    return act::files::FileUtil::WriteFile(getConfFilePath(fileName), value, *m_logger);
}

bool LinuxLed::writeConfInt(const std::string &fileName, int value)
{
    return act::files::FileUtil::WriteFile(getConfFilePath(fileName), value, *m_logger);
}

bool LinuxLed::writeConfUInt(const std::string &fileName, unsigned int value)
{
    return act::files::FileUtil::WriteFile(getConfFilePath(fileName), value, *m_logger);
}

/* ### Private methods */

std::string LinuxLed::getLedDirPath() const
{
    return LEDS_DIR + "/" + m_ledName;
}

std::string LinuxLed::getConfFilePath(const std::string &fileName) const
{
    return getLedDirPath() + "/" + fileName;
}

} // namespace act::linux_io
