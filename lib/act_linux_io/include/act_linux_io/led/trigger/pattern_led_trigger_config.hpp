// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "abs_led_trigger_config.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace act::linux_io
{

/**
 * @brief "pattern" Linux LED trigger configurator
 *
 * This trigger allows to configure a LED to blink following a user-defined pattern,
 * given as a serie of (brightness, duration) pairs, and an optional repeat count.
 */
class PatternLedTriggerConfig : public AbsLedTriggerConfig
{
    /* ## Types */
  public:
    /** @brief Type defining a (brightness, duration) pair
     *  - brightness: Brightness raw value
     *      - 0: OFF
     *      - 1: max brightness of GPIO LEDs
     *      - >=1: PWM brightness level for PWM LEDs
     *      - this brightness must not exceed targeted LED(s) max brightness
     *  - duration: Duration in milliseconds before next step.
     *              Note that next step is the first step if pattern is repeating.
     */
    using PatternStep = std::pair<unsigned int, std::chrono::milliseconds>;

    /** @brief Type defining a pattern as a series of PatternStep */
    using Pattern = std::vector<PatternStep>;

    /* ## Constructors */
  public:
    /**
     * @brief Constructor
     * @param pattern Pattern defining the blinking sequence
     * @param repeatCount Number of times the pattern must be repeated,
     *                    or REPEAT_INFINITE for infinite repetition
     */
    explicit PatternLedTriggerConfig(Pattern pattern, int repeatCount);

    /**
     * @brief Destructor
     */
    ~PatternLedTriggerConfig() override = default;

    /* ## Methods (members, then non-members) */
  public:
    /**
     * @brief Check if the trigger is currently applied to the given LED
     * @return True if the trigger is applied to the LED, false otherwise
     * @retval False upon issues reading LED state
     * @note Default implementation only compares trigger names, subclasses
     *       must call it and extend it to compare their arguments if any
     */
    [[nodiscard]] bool isLedAlreadyConfigured(const LinuxLed &led) const override;

    /**
     * @brief Change pattern
     * @param pattern New pattern defining the blinking sequence
     */
    void setPattern(const Pattern &pattern);

    /**
     * @brief Change repeat count
     * @param repeatCount New number of times the pattern must be repeated,
     *                    or REPEAT_INFINITE for infinite repetition
     */
    void setRepeatCount(int repeatCount)
    {
        m_repeatCount = repeatCount;
    }

    /**
     * @brief Actually fire or fire again the trigger on the given LED
     * @param led LED to fire trigger onto
     * @return True if useless, True on success, false on failure
     * @note LED must have been previously prepared with @ref applyToLed or @ref prepareLed
     *       and must still be prepared. Especially its trigger must not have been changed
     *       or cleared directly or through explicit state/brightness setting.
     * @retval If current trigger has no "fire" action (ex: "heartbeat" or "timer" trigger),
     *         true is always returned.
     */
    bool fireOnLed(LinuxLed &led) const override;

  protected:
    /**
     * @brief Configure trigger parameters onto LED, without starting the trigger
     * @param led LED to configure
     * @return True on success, false on failure
     * @note At this point, the trigger name has already been written to the LED
     * @note Since we better like to not start the trigger yet (for syncness reasons,
     *       see @ref applyToLeds), sub-classes may need to move some configurations
     *       to their @ref fireOnLed implementation.
     */
    bool prepareLedExtra(LinuxLed &led) const override;

  private:
    /**
     * @brief Return or generate and cache stringified version of the pattern for sysfs writing
     * @return A string representing the pattern in Linux format
     */
    [[nodiscard]] std::string getPatternString() const;

    /**
     * @brief Generate stringified version of the pattern for sysfs writing
     * @return A string representing the pattern in Linux sysfs format
     */
    [[nodiscard]] std::string computePatternString() const;

    /* ## Constants */
  public:
    /** @brief Constant indicating infinite repeat count */
    static const int REPEAT_INFINITE;

  private:
    /** @brief Name of the "pattern" trigger */
    static const std::string PATTERN_TRIGGER_NAME;

    /** @brief Name of the "pattern" file in sysfs */
    static const std::string PATTERN_FILE_NAME;

    /** @brief Name of the "repeat" file in sysfs */
    static const std::string REPEAT_FILE_NAME;

    /* ## Data members */
  private:
    // Configuration
    /** @brief Pattern defining the blinking sequence */
    Pattern m_pattern;

    /** @brief Number of times the pattern must be repeated,
     *         or REPEAT_INFINITE for infinite repetition */
    int m_repeatCount;

    // Cache
    /** @brief Cached stringified version of the pattern for quicker sysfs writing */
    mutable std::string m_patternStringCache;
};

} // namespace act::linux_io
