// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace act::logger
{
class LoggerHelper;
} // namespace act::logger

namespace gpiod
{
class chip;
class line_settings;
} // namespace gpiod

namespace act::linux_io
{

/**
 * @brief Linux (single) GPIO interface
 */
class LinuxGpio
{
    /* ## Types */
    /* ## Constructors */
  public:
    /**
     * @brief Builds a GPIO interface given GPIO chip name and line number
     * @param chipName Name of the GPIO chip (e.g. "gpiochip0")
     * @param lineNum Line number of the GPIO within the chip
     *                (0-based, usually in the range 0..31)
     * @see FindGpioByName
     */
    explicit LinuxGpio(const std::string &chipName,
                       unsigned int lineNum,
                       act::logger::LoggerHelper &parentLogger);

    /**
     * @brief Destructor
     */
    virtual ~LinuxGpio();

    /* ## Methods (members, then non-members) */
  public:
    /**
     * @brief Tells if GPIO exists
     * @return True if GPIO exists, false otherwise
     */
    [[nodiscard]] bool found() const
    {
        return m_found;
    }

    /**
     * @brief Prepares the GPIO line settings for input direction
     * @return true on success, false on failure
     * @note This will only apply upon next real GPIO request
     */
    bool prepDirectionInput();

    /**
     * @brief Prepares the GPIO line settings for pull-up bias
     * @return true on success, false on failure
     * @note This will only apply upon next real GPIO request
     */
    bool prepBiasPullUp();

    /**
     * @brief Prepares the GPIO line settings for active-low logic
     * @return true on success, false on failure
     * @note This will only apply upon next real GPIO request
     */
    bool prepActiveLow();

    /**
     * @brief Prepares the GPIO line settings for debounce
     * @param debounce Debounce time
     * @return true on success, false on failure
     * @note This will only apply upon next real GPIO request
     */
    bool prepDebounce(std::chrono::milliseconds debounce);

    /**
     * @brief Gets the name of the GPIO chip, as reported by Linux
     * @return GPIO chip name or empty string upon failure
     */
    [[nodiscard]] std::string getName() const;

    /**
     * @brief Gets the current state of the GPIO
     * @return True if GPIO is logically ON, false otherwise
     * @retval false upon failure (see @exists)
     * @note An Active-low GPIO will return true when physically LOW
     */
    [[nodiscard]] bool getValue() const;

    /**
     * @brief Listens GPIO events (edge changes)
     * @param callback Callback to invoke upon event (parameter is new GPIO state)
     * @return True if listening was successfully started, false otherwise
     * @note This method spawns a thread to listen for events,
     *       callback is invoked from that thread
     * @note Calling this method twice will spawn two listening threads
     * @note Callback receives GPIO logical state,
     *       that is will receive true when an active-low GPIO goes LOW.
     * @warning The listening thread is not stoppable for now,
     *          it will even survive the LinuxGpio object destruction
     *          and attempt to notify the callback even after that.
     * @warning It is currently not possible to stop a started listening,
     *          especially null callback is rejected and do not cancel a previous request.
     */
    bool listenEvents(const std::function<void(bool)> &callback);

    /**
     * @brief Finds a GPIO by its line name
     * @param lineName Name of the GPIO line to find
     * @param parentLogger Logger helper to use for the created LinuxGpio object
     * @return Pointer to the LinuxGpio object if found, nullptr otherwise
     * @note Name is not guaranteed to be unique, first matching is returned
     * @note Caller is responsible for deleting the returned LinuxGpio object
     */
    [[nodiscard]] static LinuxGpio *FindGpioByName(const std::string &lineName,
                                                   act::logger::LoggerHelper &parentLogger);

  private:
    /**
     * @brief Open a GPIO chip device
     * @param chipPath Path of the chip device
     * @return The opened chip, or nullptr if it cannot be opened (missing, no permission...)
     */
    [[nodiscard]] static std::unique_ptr<gpiod::chip> OpenChip(const std::string &chipPath);

    /* ## Constants */
  private:
    /** @brief Logger category for LinuxGpio */
    static const constexpr char *LOGGER_CATEGORY = "gpio";

    /** @brief Consumer name for gpiod line requests */
    static const constexpr char *GPIOD_CONSUMER_NAME = "C++";

    /** @brief Directory for GPIO chips devices */
    static const constexpr char *CHIPS_DEV_DIR = "/dev";

    /* ## Data members */
  private:
    /** @brief Underlying gpiod chip */
    std::unique_ptr<gpiod::chip> m_chip;

    /** @brief Line number within the chip (0..n) */
    unsigned int m_lineNum;

    /** @brief Underlying line settings */
    std::unique_ptr<gpiod::line_settings> m_lineSettings;

    /** @brief Indicates if the GPIO exists */
    bool m_found{false};

    /** @brief Logger */
    std::shared_ptr<act::logger::LoggerHelper> m_logger;
};

} // namespace act::linux_io
