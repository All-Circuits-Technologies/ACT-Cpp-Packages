// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

// Note: Algorithms are strongly inspired by libgpiod C++ examples

#include "act_linux_io/gpio/linux_gpio.hpp"

#include "act_logger/helpers/logger_helper.hpp"

#include <gpiod.hpp>
#include <system_error>
#include <thread>

/* # Macros */
/** @brief Return X if GPIO not found */
#define LINUXGPIO_RETURN_X_IF_NOT_FOUND(X)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!found())                                                                              \
        {                                                                                          \
            m_logger->warning("GPIO not usable");                                                  \
            return X;                                                                              \
        }                                                                                          \
    } while (0)

namespace act::linux_io
{

/* # Constructors */

/* # Methods */

LinuxGpio::LinuxGpio(const std::string &chipName,
                     unsigned int lineNum,
                     act::logger::LoggerHelper &parentLogger)
    : m_chip(OpenChip(std::string(CHIPS_DEV_DIR) + "/" + chipName)),
      m_lineNum(lineNum),
      m_lineSettings(std::make_unique<gpiod::line_settings>()),
      m_found(m_chip && *m_chip &&
              (m_lineNum < static_cast<unsigned int>(m_chip->get_info().num_lines())))
{
    // Compute an accurate and user-friendly logger category, then create logger
    std::string gpioLogName = chipName + ":" + std::to_string(lineNum);
    if (m_found)
    {
        const std::string lineName = getName();
        if (!lineName.empty())
        {
            gpioLogName = lineName;
        }
    }
    m_logger = parentLogger.createSubLogger(std::string(LOGGER_CATEGORY) + "/" + gpioLogName);

    // Log issues if any
    if (!m_found)
    {
        m_logger->warning("GPIO not found");
    }
}

// Destructor implementation needs to be in CPP file since gpiod::chip is forward decared in HPP
// and can't be freed from HPP. Default destructor still fits our needs.
LinuxGpio::~LinuxGpio() = default;

bool LinuxGpio::prepDirectionInput()
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    m_lineSettings->set_direction(::gpiod::line::direction::INPUT);
    m_lineSettings->set_edge_detection(::gpiod::line::edge::BOTH);

    return true;
}

bool LinuxGpio::prepBiasPullUp()
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    m_lineSettings->set_bias(::gpiod::line::bias::PULL_UP);

    return true;
}

bool LinuxGpio::prepActiveLow()
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    m_lineSettings->set_active_low(true);

    return true;
}

bool LinuxGpio::prepDebounce(std::chrono::milliseconds debounce)
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    m_lineSettings->set_debounce_period(debounce);

    return true;
}
std::string LinuxGpio::getName() const
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND({});

    return m_chip->get_line_info(m_lineNum).name();
}

bool LinuxGpio::getValue() const
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    auto request = m_chip->prepare_request()
                       .set_consumer("get-line-value")
                       .add_line_settings(m_lineNum, *m_lineSettings)
                       .do_request();

    return (request.get_value(m_lineNum) == ::gpiod::line::value::ACTIVE);
}

bool LinuxGpio::listenEvents(const std::function<void(bool)> &callback)
{
    LINUXGPIO_RETURN_X_IF_NOT_FOUND(false);

    if (callback == nullptr)
    {
        m_logger->error("Cannot listen to GPIO events: callback is null");
        return false;
    }

    // Start a thread to listen for events
    std::thread task = std::thread([this, callback = callback]() {
        /*
         * Assume a button connecting the pin to ground, so pull it up and
         * provide some debounce.
         */
        auto request = m_chip->prepare_request()
                           .set_consumer("watch-line-value")
                           .add_line_settings(m_lineNum, *m_lineSettings)
                           .do_request();

        /*
         * A larger buffer is an optimisation for reading bursts of events from
         * the kernel, but that is not necessary in this case, so 1 is fine.
         */
        ::gpiod::edge_event_buffer buffer(1);

        for (;;)
        {
            /* Blocks until at least one event is available. */
            request.read_edge_events(buffer);

            for (const auto &event : buffer)
            {
                const bool value = (event.type() == gpiod::edge_event::event_type::RISING_EDGE);
                m_logger->traceStream() << (value ? "Rising edge" : "Falling edge");

                try
                {
                    callback(value);
                }
                catch (const std::exception &e)
                {
                    m_logger->warningStream() << "Exception in GPIO event callback: " << e.what();
                }
            }
        }
    });
    task.detach();

    return true;
}

LinuxGpio *LinuxGpio::FindGpioByName(const std::string &lineName,
                                     act::logger::LoggerHelper &parentLogger)
{
    for (const auto &entry : ::std::filesystem::directory_iterator(CHIPS_DEV_DIR))
    {
        if (::gpiod::is_gpiochip_device(entry.path()))
        {
            const auto chip = OpenChip(entry.path());
            if (!chip)
            {
                // A chip which cannot be opened cannot hold the line we look for
                continue;
            }

            auto offset = chip->get_line_offset_from_name(lineName);
            if (offset >= 0)
            {
                return new LinuxGpio(chip->get_info().name(),
                                     static_cast<unsigned int>(offset),
                                     parentLogger);
            }
        }
    }

    parentLogger.warningStream() << "GPIO '" << lineName << "' not found";
    return nullptr;
}

std::unique_ptr<gpiod::chip> LinuxGpio::OpenChip(const std::string &chipPath)
{
    try
    {
        return std::make_unique<gpiod::chip>(chipPath);
    }
    catch (const std::system_error &)
    {
        // libgpiod throws when the device cannot be opened
        return nullptr;
    }
}

} // namespace act::linux_io
