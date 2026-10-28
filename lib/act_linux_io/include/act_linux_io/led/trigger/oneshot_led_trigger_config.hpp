// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "abs_led_trigger_config.hpp"

#include <chrono>

namespace act::linux_io
{

/**
 * @brief "oneshot" Linux LED trigger configurator
 *
 * This trigger let you trigger a led flash once when fired,
 * whose characteristics (duration, etc) are previously defined.
 */
class OneShotLedTriggerConfig : public AbsLedTriggerConfig
{
    /* ## Constructors */
  public:
    /**
     * @brief Constructor
     * @param delayOn One-shot blink duration when fired
     * @param delayOff Minimal duration between blinks, added after the blink
     * @param invert Should blink turn led ON (true) or OFF (false)
     * @note A minimum value of 1ms is internally applied to delayOff
     *       since otherwise the LED never stop shining
     */
    explicit OneShotLedTriggerConfig(std::chrono::milliseconds delayOn,
                                     std::chrono::milliseconds delayOff = {},
                                     bool invert = false);

    /**
     * @brief Destructor
     */
    ~OneShotLedTriggerConfig() override = default;

    /* ## Methods (members, then non-members) */
  public:
    /**
     * @brief Check if the trigger is currently applied to the given LED
     * @return True if the trigger is applied to the LED, false otherwise
     * @retval False upon issues reading LED state
     */
    [[nodiscard]] bool isLedAlreadyConfigured(const LinuxLed &led) const override;

    /**
     * @brief Initiate a one-time blink on the given LED
     * @param led LED to fire trigger onto
     * @return True upon success, false on failure
     */
    bool fireOnLed(LinuxLed &led) const override;

  protected:
    /**
     * @brief Configure delays and inversion
     * @param led LED to configure
     * @return True upon success, false on failure
     */
    bool prepareLedExtra(LinuxLed &led) const override;

    /* ## Constants */
  private:
    /** @brief Name of the "oneshot" trigger */
    static inline const std::string ONESHOT_TRIGGER_NAME{"oneshot"};

    /** @brief Name of the "delay_on" file in sysfs */
    static inline const std::string DELAY_ON_FILE_NAME{"delay_on"};

    /** @brief Name of the "delay_off" file in sysfs */
    static inline const std::string DELAY_OFF_FILE_NAME{"delay_off"};

    /** @brief Name of the "invert" file in sysfs */
    static inline const std::string INVERT_FILE_NAME{"invert"};

    /** @brief Name of the "shot" file in sysfs */
    static inline const std::string SHOT_FILE_NAME{"shot"};

    /**
     * @brief Minimum accepted delay_off value
     * @note Linux will not turn the LED off if delay_off is set to 0.
     *       Therefore we internally enforce a minimum of 1ms here.
     */
    static inline const std::chrono::milliseconds MIN_DELAY_OFF{1};

    /* ## Data members */
  private:
    /** @brief One-shot blink duration when fired */
    const std::chrono::milliseconds m_delayOn;

    /** @brief Minimal duration between blinks, added after the blink */
    const std::chrono::milliseconds m_delayOff;

    /** @brief Should blink turn led ON (true) or OFF (false) */
    const bool m_invert;
};

} // namespace act::linux_io
