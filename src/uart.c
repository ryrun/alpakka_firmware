// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <pico/stdio.h>
#include <pico/bootrom.h>
#include <hardware/watchdog.h>
#include "uart.h"
#include "config.h"
#include "self_test.h"
#include "logging.h"
#include "power.h"
#include "esp.h"

void uart_listen_do(bool limited) {
    char input = getchar_timeout_us(0);
    if (input == 'R') {
        info("UART: Restart\n");
        power_restart();
    }
    if (input == 'B') {
        info("UART: Bootsel mode\n");
        power_bootsel();
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

// Ring buffer since RP2040 hardware FIFO is only 32 bytes.
uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t write_pos;
static volatile uint16_t read_pos;

void uart_rx_buffer_init() {
    write_pos = 0;
    read_pos = 0;
}

bool uart_rx_buffer_is_empty() {
    // printf("W %i R %i\n", write_pos, read_pos);
    return write_pos == read_pos;
}

bool uart_rx_buffer_is_full() {
    return ((write_pos + 1) % UART_RX_BUFFER_SIZE) == read_pos;
}

bool uart_rx_buffer_put(uint8_t byte) {
    if (uart_rx_buffer_is_full()) {
        // warn("UART: RX buffer full\n");
        return false;
    }
    rx_buffer[write_pos] = byte;
    write_pos = (write_pos + 1) % UART_RX_BUFFER_SIZE;
    return true;
}

uint8_t uart_rx_buffer_get() {
if (uart_rx_buffer_is_empty()) return false;
    uint8_t byte = rx_buffer[read_pos];
    read_pos = (read_pos + 1) % UART_RX_BUFFER_SIZE;
    return byte;
}

void uart_rx_irq_callback() {
    while(uart_is_readable(ESP_UART)) {
        char c = uart_getc(ESP_UART);
        uart_rx_buffer_put(c);
    }
}

// LlamaMessage uart_llama_parse() {
//     static uint8_t command[8] = {0,};
//     static uint8_t payload[32] = {0,};
//     static uint8_t i = 0;
//     LlamaMessage llama = {};
//     while(!uart_rx_buffer_is_empty()) {
//         char c = uart_rx_buffer_get();
//         if (i < 4) {
//             // Check control bytes.
//             if ((i==0 && c==16) || (i==1 && c==32) || (i==2 && c==64) || (i==3 && c==128))  {
//                 i += 1;
//             } else {
//                 i = 0;
//             }
//         } else if (i < 12) {
//             command[i-4] = c;
//             i += 1;
//         } else {
//             payload[i-4-8] = c;
//             i += 1;
//             if (i == 4+8+32) {
//                 i = 0;
//                 llama.command
//                 return
//             }
//         }
//     }
// }
