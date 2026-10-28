// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <memory>
#include <optional>
#include <string>

namespace act::logger
{
class LoggerHelper;
} // namespace act::logger

namespace act::linux_io
{

class AbsLedTriggerConfig;

/**
 * @brief Linux (single) LED interface
 */
class LinuxLed
{
    /* ## Types */
    /* ## Constructors */
  public:
    /**
     * @brief Default constructor
     * @param ledName Name of the LED, must match sysfs LED folder name
     * @param parentLogger Logger to use
     * @note Caller must call init before using other methods
     */
    explicit LinuxLed(std::string ledName, act::logger::LoggerHelper &parentLogger);

    /**
     * @brief Destructor
     */
    virtual ~LinuxLed() = default;

    /* ## Methods (members, then non-members) */
    /**
     * @brief Get LED name
     * @return Name of the sysfs LED folder
     */
    [[nodiscard]] const std::string &getName() const
    {
        return m_ledName;
    }

    /**
     * @brief Get Synthetic LED state
     * @return true if LED is currently ON, false if OFF
     * @note If LED has an active trigger, reading its state is likely
     *       error-prone (blinking LED for example).
     * @note False is returned on failure to read state (same as LED OFF),
     *       which is unlikely if a previous init call succeeded.
     */
    [[nodiscard]] bool getState() const
    {
        return getBrightness() > 0;
    }

    /**
     * @brief Set LED state
     * @param onOff true to turn ON the LED (max brightness),
     *              false to turn it OFF
     * @return true on success, false on failure
     * @note Touching LED state clears trigger if any
     * @note Failure is unlikely if a previous init call succeeded,
     *       therefore we allow caller to discard the return value.
     */
    bool setState(bool onOff)
    {
        return setBrightness(onOff ? getMaxBrightness() : 0);
    }

    /**
     * @brief Get LED current brightness level
     * @return Current brightness level
     * @retval 0 if LED is off
     * @retval 1 for GPIO ON LEDs
     * @retval > 1 for PWM ON LEDs
     * @note If LED has an active trigger, reading its brightness is likely
     *       error-prone (continously changing).
     * @note 0 is returned on failure to read brightness (same as LED OFF),
     *       which is unlikely if a previous init call succeeded.
     */
    [[nodiscard]] unsigned int getBrightness() const;

    /**
     * @brief Set LED brightness
     * @param brightness Brightness level to set
     *                    - 0 to turn OFF the LED
     *                    - 1 to turn ON a GPIO LED
     *                    - >= 1 turn ON a PWM LED
     * @return true on success, false on failure
     * @note Brightness value above max brightness is clamped to max brightness
     * @note Touching LED brightness clears trigger if any
     * @note Failure is unlikely if a previous init call succeeded,
     *       therefore we allow caller to discard the return value.
     */
    bool setBrightness(unsigned int brightness);

    /**
     * @brief Get LED maximum supported brightness level
     * @return 1 or GPIO LEDs, >1 for PWM LEDs
     */
    [[nodiscard]] unsigned int getMaxBrightness() const;

    /**
     * @brief Get LED trigger name
     * @return Current trigger name (please read the warning notice)
     * @warning For the shake of simplicity, trigger name is never actually read from sysfs.
     *          Instead, we cache it upon setting it and only return the cached value here.
     *          But we also ensure to set it upon @ref setState and @ref setBrightness calls,
     *          as well of course upon @ref setTrigger calls, therefore we return the proper
     *          trigger name as soon as user has initialized its LED (has set it to any state).
     *          It only remains the case of another app playing with the LED, in which case
     *          the cached value may be out-of-date.
     * @retval Empty optional at start, until setState or setBrightness or setTrigger is called
     * @see hasAnyTrigger
     * @see hasTrigger
     */
    [[nodiscard]] std::optional<std::string> getTrigger() const
    {
        return m_currentTriggerCache;
    }

    /**
     * @brief Tells if LED currently has a trigger
     * @retval True if LED has a trigger other than "none"
     * @retval False if LED has "none" trigger
     * @retval False if LED has not yet been used
     * @warning Please read @ref getTrigger warning notice
     * @see getTrigger
     */
    [[nodiscard]] bool hasAnyTrigger() const
    {
        return getTrigger().value_or(TRIGGER_NONE_VALUE) != TRIGGER_NONE_VALUE;
    }

    /**
     * @brief Tells if LED is currently configured identically to given trigger configuration
     * @return True if LED has a same trigger name and parameters than triggerConfig,
     *         False otherwise
     * @note This is a a simple convenience wrapper around @ref
     * AbsLedTriggerConfig::isLedAlreadyConfigured
     * @see getTrigger
     */
    [[nodiscard]] bool hasTrigger(const AbsLedTriggerConfig &triggerConfig) const;

    /**
     * @brief Set custom LED trigger
     * @param trigger Trigger name to set (ex: "heartbeat", "timer", "none", etc.)
     * @param force If true, force re-selecting the trigger even if already selected
     *              If false and trigger is already selected, do nothing and succeed
     * @return True on success, false on failure
     * @note Especially if your trigger has arguments, consider using a helper instead
     * @note Switching to an already-selected trigger usually resets the trigger
     * @see setTrigger(const AbsLedTriggerConfig &triggerConfig)
     */
    [[nodiscard]] bool setTrigger(const std::string &trigger, bool force);

    /**
     * @brief Set custom LED trigger using a trigger configuration helper
     * @param triggerConfig Trigger configuration helper
     * @param force If true, force re-selecting the trigger even if already selected
     *              If false and trigger is already selected, then it will be reconfigured
     *              and refired but not previously re-selected.
     * @return True on success, false on failure
     * @note Switching to an already-selected trigger usually resets the trigger
     * @note This is a a simple convenience wrapper around @ref AbsLedTriggerConfig::applyToLed
     * @see setTrigger(const std::string &trigger, bool force)
     * @see setTriggerIfNotAlreadyApplied(const AbsLedTriggerConfig &triggerConfig, bool force)
     */
    [[nodiscard]] bool setTrigger(const AbsLedTriggerConfig &triggerConfig, bool force = false);

    /**
     * @brief Ensure LED has given trigger applied using a trigger configuration helper
     * @param triggerConfig Trigger configuration helper
     * @param force If true, force re-selecting the trigger even if already selected
     *              If false and trigger is already selected, then it will be reconfigured
     *              and refired but not previously re-selected.
     * @return True on success, false on failure
     * @note This is a a simple convenience wrapper around
     *       @ref AbsLedTriggerConfig::applyToLedIfNotAlreadyConfigured
     * @note Do nothing if LED configuration already matches given trigger
     * @see setTrigger(const std::string &trigger, bool force)
     * @see setTrigger(const AbsLedTriggerConfig &triggerConfig, bool force)
     */
    [[nodiscard]] bool setTriggerIfNotAlreadyConfigured(const AbsLedTriggerConfig &triggerConfig,
                                                        bool force = false);

    /**
     * @brief Explicitly clear LED trigger, going back to manual brightness control
     * @return True on success, false on failure
     * @note Calling @ref setState or @ref setBrightness usually clears the trigger as well
     */
    [[nodiscard]] bool clearTrigger()
    {
        return setTrigger(TRIGGER_NONE_VALUE, false);
    }

    /**
     * @brief Get the content of a special file in the LED folder
     * @param fileName Name of the pseudo-file within LED folder (ex: "trigger")
     * @return The content of the file, or empty optional upon failure
     */
    [[nodiscard]] std::optional<std::string> readConfString(const std::string &fileName) const;

    /**
     * @brief Get the content of a special file in the LED folder, casted to int
     * @param fileName Name of the pseudo-file within LED folder (ex: "repeat")
     * @return The numerical content of the file, or empty optional upon failure
     */
    [[nodiscard]] std::optional<int> readConfInt(const std::string &fileName) const;

    /**
     * @brief Get the content of a special file in the LED folder, casted to uint
     * @param fileName Name of the pseudo-file within LED folder (ex: "brightness")
     * @return The numerical content of the file, or empty optional upon failure
     */
    [[nodiscard]] std::optional<unsigned int> readConfUInt(const std::string &fileName) const;

    /**
     * @brief Set the content of a special file in the LED folder
     * @param fileName Name of the pseudo-file within LED folder (ex: "trigger")
     * @param value Value to write into the file
     * @return True on success, false on failure
     */
    [[nodiscard]] bool writeConfString(const std::string &fileName, const std::string &value);

    /**
     * @brief Set the content of a special file in the LED folder
     * @param fileName Name of the pseudo-file within LED folder (ex: "repeat")
     * @param value Value to write into the file
     * @return True on success, false on failure
     */
    [[nodiscard]] bool writeConfInt(const std::string &fileName, int value);

    /**
     * @brief Set the content of a special file in the LED folder
     * @param fileName Name of the pseudo-file within LED folder (ex: "brightness")
     * @param value Value to write into the file
     * @return True on success, false on failure
     */
    [[nodiscard]] bool writeConfUInt(const std::string &fileName, unsigned int value);

  private:
    /**
     * @brief Get LED directory path
     */
    [[nodiscard]] std::string getLedDirPath() const;

    /**
     * @brief Get absolute file path of a given LED folder pseudo-file
     * @param fileName Name of the pseudo-file within LED folder (ex: "brightness")
     * @return Absolute file path of the given pseudo-file
     */
    [[nodiscard]] std::string getConfFilePath(const std::string &fileName) const;

    /* ## Constants */
  private:
    /** @brief Linux LEDs parent folder */
    static const std::string LEDS_DIR;

    /** @brief LED brightness file name */
    static const std::string BRIGHTNESS_FILE_NAME;

    /** @brief LED max brightness file name */
    static const std::string MAX_BRIGHTNESS_FILE_NAME;

    /** @brief LED trigger file name */
    static const std::string TRIGGER_FILE_NAME;

    /** @brief Special trigger value used to clear the trigger */
    static const std::string TRIGGER_NONE_VALUE;

    /* ## Data members */
  private:
    // Configuration
    /** @brief Linux LED (folder) name */
    const std::string m_ledName;

    /** @brief Logger helper */
    std::shared_ptr<act::logger::LoggerHelper> m_logger;

    // Cache
    /** @brief Biggest supported brightness level for this led */
    mutable unsigned int m_maxBrightnessCache{0};

    /** @brief Cache of current trigger name */
    std::optional<std::string> m_currentTriggerCache;
};

} // namespace act::linux_io
