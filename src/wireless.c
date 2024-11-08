// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <string.h>
#include <pico/time.h>
#include <hardware/uart.h>
#include "wireless.h"
#include "pin.h"
#include "led.h"
#include "bus.h"
#include "hid.h"
#include "loop.h"
#include "logging.h"
#include "common.h"
#include "uart.h"
#include "esp.h"

static bool uart_data_mode = false;


static void led_task() {
    static uint8_t i = 0;
    static bool led_state;
    i++;
    if (i==100) {
        led_board_set(led_state);
        led_state = !led_state;
        i = 0;
    }
}

void wireless_send(uint8_t report_id, void *packet, uint8_t len) {
    uint8_t control[4] = {16, 32, 64, 128};
    uint8_t command[8] = {0,};
    uint8_t payload[32] = {0,};
    payload[0] = report_id;
    memcpy(&payload[1], packet, len);
    uart_write_blocking(ESP_UART, control, 4);
    uart_write_blocking(ESP_UART, payload, 32);
    // printf(">");
}

static void cross_logging() {
    // Redirect incomming ESP UART logging to main UART.
    while(uart_is_readable(ESP_UART)) {
        char character = uart_getc(ESP_UART);
        printf("%c", character);
    }
}

// static void uart_rx() {
//     while(uart_is_readable(ESP_UART)) {
//         char c = uart_getc(ESP_UART);
//         printf("%i ", c);
//     }
// }

void wireless_set_uart_data_mode(bool mode) {
    info("RF: data_mode=%i\n", mode);
    uart_data_mode = mode;
    if (mode) {
        esp_restart(true);
        uart_deinit(ESP_UART);
        uart_init(ESP_UART, ESP_DATA_BAUD);
        info("RF: UART1 init (%i)\n", ESP_DATA_BAUD);
        uart_rx_buffer_init();
        irq_set_exclusive_handler(UART1_IRQ, uart_rx_irq_callback);
        irq_set_enabled(UART1_IRQ, true);
        uart_set_irq_enables(ESP_UART, true, false);  // RX - TX
    } else {
        uart_deinit(ESP_UART);
        uart_init(ESP_UART, ESP_BOOTLOADER_BAUD);
        info("RF: UART1 init (%i)\n", ESP_BOOTLOADER_BAUD);
    }
}

void wireless_init(bool dongle) {
    if (dongle) info("INIT: RF dongle\n");
    else info("INIT: RF controller\n");
    // Boot pin.
    bool boot = false;
    info("RF: ESP boot=%i\n", boot);
    gpio_init(PIN_ESP_BOOT);
    gpio_set_dir(PIN_ESP_BOOT, GPIO_OUT);
    gpio_put(PIN_ESP_BOOT, boot);
    // Power enable pin.
    bool enable = false;
    info("RF: ESP enable=%i\n", enable);
    gpio_init(PIN_ESP_ENABLE);
    gpio_set_dir(PIN_ESP_ENABLE, GPIO_OUT);
    gpio_put(PIN_ESP_ENABLE, enable);
    // Secondary UART.
    info("RF: UART1 init (%i)\n", ESP_BOOTLOADER_BAUD);
    uart_init(ESP_UART, ESP_BOOTLOADER_BAUD);
    gpio_set_function(PIN_UART1_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART1_RX, GPIO_FUNC_UART);
    //
    wireless_set_uart_data_mode(true);
    // esp_restart(true);
}

void wireless_controller_task() {
    // if (!uart_data_mode) cross_logging();
    hid_report_wireless();
    while(!uart_rx_buffer_is_empty()) {
        char c = uart_rx_buffer_get();
        info("%c", c);  // Redirect to RP2040 uart log.
    }
}

void wireless_dongle_task() {
    led_task();
    if (!uart_data_mode) {
        cross_logging();
    } else {
        // Receive UART data from ESP.
        static uint8_t payload[32] = {0,};
        static uint8_t i = 0;
        while(!uart_rx_buffer_is_empty()) {
            char c = uart_rx_buffer_get();
            // printf("%i=%i ", i, c);
            if (i < 4) {
                // Check control bytes.
                if ((i==0 && c==16) || (i==1 && c==32) || (i==2 && c==64) || (i==3 && c==128))  {
                    // printf("C\n");
                    i += 1;
                } else {
                    // printf("X\n");
                    // warn("RF: UART misaligned\n");
                    i = 0;
                    info("%c", c);  // Redirect to RP2040 uart log.
                }
            } else {
                // Get payload.
                // printf("P\n");
                payload[i-4] = c;
                i += 1;
                // Payload complete.
                if (i == 32+4) {
                    // printf("\n");
                    // print_array(payload, 32);
                    // printf("\n");
                    hid_report_dongle(payload[0], &payload[1]);
                    i = 0;
                }
            }
        }
    }
}
