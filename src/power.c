// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <pico/time.h>
#include <pico/bootrom.h>
#include <hardware/watchdog.h>
#include <hardware/gpio.h>
#include <hardware/xosc.h>
#include <hardware/regs/io_bank0.h>
#include "pin.h"
#include "led.h"
#include "esp.h"
#include "loop.h"
#include "logging.h"

void power_restart() {
    watchdog_enable(1, false);  // Reboot after 1 millisecond.
    sleep_ms(10);  // Stall the exexution to avoid resetting the timer.
}

void power_bootsel() {
    if (loop_get_device_mode() == WIRELESS) {
        warn("POWER: Unable to go into bootsel while wireless\n");
        return;
    }
    reset_usb_boot(0, 0);
}

void power_dormant() {
    // Turn off ESP.
    esp_enable(false);
    // Turn off leds immediately.
    led_idle_mask(0b0000);
    led_set_mode(LED_MODE_IDLE);
    // Ensure home button is released (after shortcut press).
    while(!gpio_get(PIN_HOME)) {
        sleep_ms(100);
    }
    // Set wake-up button interrupt.
    uint32_t event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_LOW_BITS;
    gpio_set_dormant_irq_enabled(PIN_HOME, event, true);
    // Set watchdog to restart the controller after wake up.
    watchdog_enable(100, false);
    // Go sleep.
    xosc_dormant();
    // Don't do anything, let the watchdog restarts the controller.
    sleep_ms(10000);
}
