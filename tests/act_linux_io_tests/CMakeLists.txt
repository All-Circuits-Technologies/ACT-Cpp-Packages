# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_linux_io. The LEDs sysfs folder cannot be relocated, so the LED tests drive an
# LED which does not exist; LinuxGpio is only covered without a GPIO chip.
act_add_unit_test(act_linux_io_tests
    SOURCES
    test_abs_led_trigger_config.cpp
    test_linux_gpio.cpp
    test_linux_led.cpp
    test_oneshot_led_trigger_config.cpp
    test_pattern_led_trigger_config.cpp
    test_simple_led_trigger_config.cpp
    LIBS
    act_linux_io
)
