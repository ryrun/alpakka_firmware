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

void wireless_send(uint8_t report_id, void *packet, uint8_t len) {
    uint8_t control[4] = UART_CONTROL_BYTES;
    uint8_t command[8] = {0,};
    uint8_t payload[32] = {0,};
    payload[0] = report_id;
    memcpy(&payload[1], packet, len);
    uart_write_blocking(ESP_UART, control, 4);
    uart_write_blocking(ESP_UART, payload, 32);
}

static void cross_logging() {
    // Redirect incomming ESP UART logging to main UART.
    while(uart_is_readable(ESP_UART)) {
        char character = uart_getc(ESP_UART);
        printf("%c", character);
    }
}

void wireless_set_uart_data_mode(bool mode) {
    info("RF: data_mode=%i\n", mode);
    uart_data_mode = mode;
    if (mode) {
        esp_restart();
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
    #ifdef DEVICE_HAS_MARMOTA
        if (dongle) {
            info("INIT: RF dongle\n");
            led_board_set(true);
        } else {
            info("INIT: RF controller\n");
        }
        // Prepare ESP.
        esp_init();
        // Secondary UART.
        info("RF: UART1 init (%i)\n", ESP_BOOTLOADER_BAUD);
        uart_init(ESP_UART, ESP_BOOTLOADER_BAUD);
        gpio_set_function(PIN_UART1_TX, GPIO_FUNC_UART);
        gpio_set_function(PIN_UART1_RX, GPIO_FUNC_UART);
    #endif
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
    // led_task();
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
