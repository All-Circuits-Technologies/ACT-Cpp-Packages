// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "abs_led_trigger_config.hpp"

#include <string>

namespace act::linux_io
{

/**
 * @brief Linux LED trigger configurator for all simple triggers featuring no extra parameter
 */
class SimpleLedTriggerConfig : public AbsLedTriggerConfig
{
    /* ## Constructors */
  public:
    /**
     * @brief Constructor
     * @param pattern Name of the basic trigger, must match Linux LED trigger name
     * @note See static constants of this class for a set of known trigger names
     */
    explicit SimpleLedTriggerConfig(std::string name);

    /**
     * @brief Destructor
     */
    ~SimpleLedTriggerConfig() override = default;

    /* ## Methods (members, then non-members) */
  public:
    /**
     * @brief Actually fire or fire again the trigger on the given LED
     * @param led LED to fire trigger onto
     * @return True
     * @note Simple triggers have no "fire" action, so this is a no-op
     */
    bool fireOnLed(LinuxLed & /*led*/) const override
    {
        return true;
    };

  protected:
    /**
     * @brief Configure trigger parameters onto LED, without starting the trigger
     * @param led LED to configure
     * @return True
     * @note Simple triggers have no extra parameter to configure
     */
    bool prepareLedExtra(LinuxLed & /*led*/) const override
    {
        return true;
    };

    /* ## Constants */
  public:
    /** @brief Name of the "panic" trigger */
    static inline const std::string PANIC_TRIGGER_NAME{"panic"};

    /* ## Data members */
};

} // namespace act::linux_io
