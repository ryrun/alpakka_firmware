// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <tusb.h>
#include "loop.h"
#include "config.h"
#include "wireless.h"
#include "led.h"
#include "bus.h"
#include "profile.h"
#include "touch.h"
#include "imu.h"
#include "hid.h"
#include "uart.h"
#include "logging.h"
#include "version.h"
#include "common.h"
#include "pin.h"

static DeviceMode device_mode = WIRED;
static uint64_t system_clock = 0;

DeviceMode loop_get_device_mode() {
    return device_mode;
}

uint64_t has_system_clock() {
    return !!system_clock;
}

uint64_t get_system_clock() {
    return system_clock + (time_us_64() / 1000);
}

void set_system_clock(uint64_t time) {
    system_clock = time - (time_us_32() / 1000);
    info("LOOP: System_clock=%llu\n", system_clock);
}

void wait_for_system_clock() {
    info("Waiting for system clock\n");
    while (!has_system_clock()) {
        tud_task();
        if (tud_ready()) {
            webusb_read();
            webusb_flush();
        }
        sleep_ms(1);
    }
}

static void title(char *label) {
    info("╔====================╗\n");
    info("║ Input Labs Oy.     ║\n");
    info("║ %s ║\n"         , label);
    info("╚====================╝\n");
    info("Firmware version: %s\n", VERSION);
}

static void set_wired() {
    info("LOOP: Wired\n");
    if (device_mode != WIRED) config_reboot();
    device_mode = WIRED;
}

static void set_wireless() {
    info("LOOP: Wireless\n");
    wireless_set_uart_data_mode(true);
    device_mode = WIRELESS;
}

static void battery_init() {
    gpio_init(PIN_BATT_STAT_1);
    gpio_init(PIN_BATT_STAT_2);
    gpio_pull_up(PIN_BATT_STAT_1);
    gpio_pull_up(PIN_BATT_STAT_2);
    gpio_set_dir(PIN_BATT_STAT_1, GPIO_IN);
    gpio_set_dir(PIN_BATT_STAT_2, GPIO_IN);
}

static void board_led() {
    static uint8_t i = 0;
    static bool blink = false;
    i++;
    if (i == 100) {
        i = 0;
        bool stat1 = gpio_get(PIN_BATT_STAT_1);
        bool stat2 = gpio_get(PIN_BATT_STAT_2);
        // info("%i %i\n", stat1, stat2);
        if (!stat1 && stat2) {
            if (device_mode == WIRED) {
                gpio_put(PIN_LED_BOARD, true);
            }
            if (device_mode == WIRELESS) {
                blink = !blink;
                gpio_put(PIN_LED_BOARD, blink);
            }

        } else {
            gpio_put(PIN_LED_BOARD, false);
        }
    }
}

void loop_controller_init() {
    led_init();
    stdio_uart_init();
    stdio_init_all();
    logging_init();
    title(LABEL_CONTROLLER);
    config_init();
    tusb_init();
    bool usb = usb_wait_for_init(USB_WAIT_FOR_INIT_MS);
    // wait_for_system_clock();
    bus_init();
    hid_init();
    thumbstick_init();
    touch_init();
    rotary_init();
    imu_init();
    profile_init();
    battery_init();
    wireless_init(false);
    if (usb) {
        set_wired();
    } else {
        set_wireless();
    }
    loop_cycle();
}

void loop_dongle_init() {
    led_init();
    stdio_uart_init();
    stdio_init_all();
    logging_init();
    title(LABEL_DONGLE);
    config_init();
    tusb_init();
    usb_wait_for_init(-1);  // Negative number = no timeout.
    // wait_for_system_clock();
    bus_init();
    hid_init();
    wireless_init(true);
    loop_cycle();
}

void loop_controller_task() {
    // Write flash if needed.
    config_sync();
    // Gather values for input sources.
    profile_report_active();
    // Report to the correct channel.
    if (device_mode == WIRED) {
        // Report to USB.
        bool reported = hid_report_wired();
        // Switch to wireless if USB is disconnected.
        if (!reported) set_wireless();
    }
    if (device_mode == WIRELESS) {
        wireless_controller_task();
        // Switch to wired if USB is connected (check once per second).
        static uint16_t i = 0;
        i++;
        if ((!(i % CFG_TICK_FREQUENCY)) && usb_is_connected()) set_wired();
    }
    // Listen to UART commands.
    uart_listen();
    // Update state of board LED.
    board_led();
}

void loop_dongle_task() {
    wireless_dongle_task();
    tud_task();
    if (tud_ready()) {
        webusb_read();
        webusb_flush();
    }
    uart_listen();
}

void loop_cycle() {
    info("LOOP: Main loop start\n");
    uint16_t i = 0;
    logging_set_onloop(true);
    while (true) {
        i++;
        // Start timer.
        uint32_t start = time_us_32();
        // Task.
        #if defined DEVICE_ALPAKKA_V0 || defined DEVICE_ALPAKKA_V1
            loop_controller_task();
        #endif
        #ifdef DEVICE_DONGLE
            loop_dongle_task();
        #endif
        // Calculate used time.
        uint32_t used = time_us_32() - start;
        int32_t unused = CFG_TICK_INTERVAL_IN_US - (int32_t)used;
        // Timing stats.
        if (logging_get_level() >= LOG_DEBUG) {
            static float average = 0;
            static float max = 0;
            average += used;
            if (used > max) max = used;
            if (!(i % CFG_TICK_FREQUENCY)) {
                info("Loop: avg=%.0f max=%.0f\n", average/1000, max);
                average = max = 0;
            }
        }
        // Idling control.
        if (unused > 0) sleep_us((uint32_t)unused);
        else {
            info("+");
            sleep_us(0);  // Allow IRQ to take over even if the controller is overwhelmed.
        };
    }
}
