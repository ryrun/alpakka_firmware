// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <pico/stdio.h>
#include <pico/bootrom.h>
#include <hardware/watchdog.h>
#include "config.h"
#include "self_test.h"
#include "logging.h"
#include "wireless.h"

void uart_listen_do(bool limited) {
    char input = getchar_timeout_us(0);
    if (input == 'R') {
        info("UART: Restart\n");
        config_reboot();
    }
    if (input == 'B') {
        info("UART: Bootsel mode\n");
        config_bootsel();
    }

    if (limited) {
        return;
    }

    if (input == 'C') {
        info("UART: Calibrate\n");
        config_calibrate();
    }
    if (input == 'F') {
        info("UART: Reset to factory settings\n");
        config_reset_factory();
    }
    if (input == 'D') {
        info("UART: Reset config\n");
        config_reset_config();
    }
    if (input == 'P') {
        info("UART: Reset profiles\n");
        config_reset_profiles();
    }
    if (input == 'T') {
        info("UART: Self-test\n");
        self_test();
    }
    if (input == 'E') {
        info("UART: ESP flash\n");
        wireless_esp_flash();
    }
}

void uart_listen() {
    static uint16_t i = 0;
    i += 1;
    // Execute only once per second.
    if (i % CFG_TICK_FREQUENCY) return;
    uart_listen_do(false);
}

void uart_listen_char_limited() {
    uart_listen_do(true);
}
