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
#include "config.h"

// ESP serial flasher
#include <esp-serial-flasher/include/esp_loader.h>
#include <esp-serial-flasher/port/pi_pico_port.h>
#include <esp-serial-flasher/examples/common/example_common.h>

#include "../../esp_llama/build/llama.bin.c"
// #include "../../esp_llama/build/llama_empty.bin.c"  // Empty version for faster development.

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

static void esp_init() {
    info("RF: ESP init\n");
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
    uart_init(uart1, ESP_BOOTLOADER_BAUD);
    gpio_set_function(PIN_UART1_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART1_RX, GPIO_FUNC_UART);
}

static void esp_enable(bool state) {
    gpio_put(PIN_ESP_ENABLE, state);
}

static void esp_boot(bool state) {
    gpio_put(PIN_ESP_BOOT, state);
}

static void esp_restart(bool bootmode) {
    esp_enable(false);
    esp_boot(!bootmode);
    sleep_ms(ESP_RESTART_SETTLE);
    esp_enable(true);
}

void wireless_esp_flash() {
    printf("RF: ESP flash start\n");
    uart_set_baudrate(uart1, ESP_FLASHER_BAUD);
    const loader_pi_pico_config_t config = {
        .uart_inst = uart1,
        .baudrate = ESP_FLASHER_BAUD,
        .uart_tx_pin_num = PIN_UART1_TX,
        .uart_rx_pin_num = PIN_UART1_RX,
        .reset_trigger_pin_num = PIN_ESP_ENABLE,
        .boot_pin_num = PIN_ESP_BOOT,
        .dont_initialize_peripheral = true,
    };
    loader_port_pi_pico_init(&config);
    uint32_t connect = connect_to_target_with_stub(ESP_FLASHER_BAUD, ESP_FLASHER_BAUD_MAX);
    if (connect != ESP_LOADER_SUCCESS) {
        error("RF: ESP flasher cannot connect (error=%li)\n", connect);
        return;
    }
    flash_binary(ESP_BOOTLOADER, sizeof(ESP_BOOTLOADER), ESP_BOOTLOADER_ADDR);
    flash_binary(ESP_PARTITION, sizeof(ESP_PARTITION), ESP_PARTITION_ADDR);
    flash_binary(ESP_FIRMWARE, sizeof(ESP_FIRMWARE), ESP_FW_ADDR);
    // loader_port_pi_pico_deinit();
    printf("RF: ESP flash done\n");
    config_reboot();
}

void wireless_send(uint8_t report_id, void *packet, uint8_t len) {
    uint8_t payload[32] = {0,};
    payload[0] = report_id;
    memcpy(&payload[1], packet, len);
    // bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_REG_WRITE | NRF24_REG_STATUS, 0);
    // bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_W_TX_PAYLOAD, payload);
}

// uint8_t wireless_nrf24_config(uint8_t address, uint8_t value) {
//     bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_WRITE | address, value);
//     uint8_t value_onchip = bus_spi_read_one(PIN_SPI_CS_NRF24, address);
//     if (value_onchip != value) error("RF: NRF24 configuration mismatch\n");
//     return value_onchip;
// }

void wireless_init(bool host) {
    if (host) info("INIT: RF host\n");
    else info("INIT: RF device\n");
    esp_init();
}

void wireless_device_task() {
    led_task();

    // static uint8_t index = 0;
    // index++;

    // bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_REG_WRITE | NRF24_REG_STATUS, 0);

    // uint8_t payload[32] = {0,};
    // uint64_t timestamp = get_system_clock();
    // memcpy(payload, &timestamp, 8);

    // uint8_t payload[32] = {index};
    // bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_W_TX_PAYLOAD, payload);


    // printf("%i %i %i %i\n", payload[0], payload[1], payload[2], payload[3]);

    // uint8_t status = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_STATUS);
    // printf("0b%08i\n", bin(status));

    // uint8_t observe = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_OBSERVE_TX);
    // printf("0b%08i\n", bin(observe));
}

void wireless_host_task() {
    led_task();
    while(uart_is_readable(uart1)) {
        static uint8_t index = 0;
        static bool char_mode = false;

        char c = uart_getc(uart1);
        printf("%c", c);

        // if      (c == 'H') printf("\n%c", c);
        // else if (c == 'I') printf("%c", c);
        // else if (c == 'D') printf("%c", c);
        // else if (c == ':') printf("%c", c);
        // else if (c > 0) printf(" %i", c);
    }
}
