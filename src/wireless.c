// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <string.h>
#include <pico/time.h>
#include "wireless.h"
#include "pin.h"
#include "led.h"
#include "bus.h"
#include "hid.h"
#include "loop.h"
#include "logging.h"
#include "common.h"

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

static void boot_task() {
    static uint32_t i = 0;
    static bool state;
    i++;
    if (i==1000) {
        i = 0;
        state = !state;
        gpio_put(PIN_ESP_ENABLE, state);
    }
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
    // uint8_t config = wireless_nrf24_config(
    //     NRF24_REG_CONFIG,
    //     host ? NRF24_CONFIG_HOST : NRF24_CONFIG_DEVICE
    // );
    // uint8_t rf_setup = wireless_nrf24_config(NRF24_REG_RFSETUP, NRF24_RFSETUP);
    // uint8_t channel = wireless_nrf24_config(NRF24_REG_CHANNEL, NRF24_CHANNEL);
    // uint8_t ack = wireless_nrf24_config(NRF24_REG_ACK, NRF24_ACK);
    // uint8_t retransmission = wireless_nrf24_config(NRF24_REG_RETRY, NRF24_RETRY);
    // uint8_t payload_size = wireless_nrf24_config(NRF24_REG_RX_PW_P0, NRF24_PAYLOAD_SIZE);
    // info("RF: config=0b%08i\n", bin(config));
    // info("RF: rf_setup=0b%08i\n", bin(rf_setup));
    // info("RF: channel=24%02i\n", channel);
    // info("RF: ack=0b%08i\n", bin(ack));
    // info("RF: retransmission=0b%08i\n", bin(retransmission));
    // info("RF: payload_size=%i\n", payload_size);

    // ESP enable.
    #if defined(DEVICE_ALPAKKA_V1) || defined(DEVICE_DONGLE)
        info("RF: ESP boot sequence\n");
        // gpio_init(PIN_UART1_TX);
        // gpio_init(PIN_UART1_RX);

        gpio_init(PIN_ESP_BOOT);
        gpio_set_dir(PIN_ESP_BOOT, GPIO_OUT);
        gpio_put(PIN_ESP_BOOT, false);

        gpio_init(PIN_ESP_ENABLE);
        gpio_set_dir(PIN_ESP_ENABLE, GPIO_OUT);
        gpio_put(PIN_ESP_ENABLE, false);
    #endif
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

    boot_task();


    while(0) {
        // uint8_t status = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_STATUS);
        // if (status & 0b10) break;  // No payloads pending in pipe 0.
        // received = true;

        // uint8_t payload[32] = {0,};
        // bus_spi_read(PIN_SPI_CS_NRF24, NRF24_R_RX_PAYLOAD, payload, 32);
        // printf("%i %i %i %i\n", payload[0], payload[1], payload[2], payload[3]);

        // hid_report_dongle(payload[0], &payload[1]);

        // Jitter.
        // static uint32_t last = 0;
        // uint32_t now = time_us_32() / 1000;
        // uint32_t elapsed = now - last;
        // last = now;

        // if (elapsed <= 6) tier0++;
        // else if (elapsed <= 12) tier1++;
        // else if (elapsed <= 24) tier2++;

        // static uint8_t i = 0;
        // static uint8_t j = 0;
        // i++;
        // j++;
        // if (elapsed > 7) {
        //     info("%lu ", elapsed);
        // }
        // if (i > 100) {
        //     i = 0;
        //     if (j % 5) info(".\n");
        //     else info("..\n");
        // }

        // Latency.
        // uint64_t now = get_system_clock();
        // uint64_t timestamp;
        // memcpy(&timestamp, payload, 8);
        // uint64_t latency = now - timestamp;
        // static double latency_sum = 0;
        // static uint16_t i = 0;
        // latency_sum += latency;
        // i++;
        // if (i == 500) {
        //     printf("latency_avg=%f ms\n", latency_sum/500);
        //     latency_sum = 0;
        //     i=0;
        // }
    }
    // if (!received) tier3++;

    // static uint64_t last_print = 0;
    // if (time_us_64() - last_print > 100000) {
    //     last_print = time_us_64();
    //     // float x = 100.0/63;
    //     while(tier0>0) {
    //         info("#");
    //         tier0 -= 1;
    //     }
    //     while(tier1>0) {
    //         info("=");
    //         tier1 -= 1;
    //     }
    //     while(tier2>0) {
    //         info("-");
    //         tier2 -= 1;
    //     }
    //     while(tier3>0) {
    //         info(".");
    //         tier3 -= 1;
    //     }
    //     info("\n");
    //     tier0 = 0;
    //     tier1 = 0;
    //     tier2 = 0;
    //     tier3 = 0;
    // }
}
