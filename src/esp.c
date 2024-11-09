// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include "esp.h"
#include "config.h"
#include "pin.h"


void esp_enable(bool state) {
    gpio_put(PIN_ESP_ENABLE, state);
}

void esp_restart() {
    esp_enable(false);
    gpio_put(PIN_ESP_BOOT, true);
    sleep_ms(ESP_RESTART_SETTLE);
    esp_enable(true);
}

void esp_bootsel() {
    esp_enable(false);
    gpio_put(PIN_ESP_BOOT, false);
    sleep_ms(ESP_RESTART_SETTLE);
    esp_enable(true);
}

#ifdef DEVICE_LLAMA
    #include <esp-serial-flasher/include/esp_loader.h>
    #include <esp-serial-flasher/port/pi_pico_port.h>
    #include <esp-serial-flasher/examples/common/example_common.h>
    #include "../../esp_llama/build/llama.bin.c"
    // #include "../../esp_llama/build/llama_empty.bin.c"  // Empty version for faster development.

    void esp_flash() {
        info("RF: ESP flash start\n");
        uart_set_baudrate(ESP_UART, ESP_FLASHER_BAUD);
        const loader_pi_pico_config_t config = {
            .uart_inst = ESP_UART,
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
        printf("RF: Going into bootsel mode...\n");
        config_bootsel();
    }
#endif
