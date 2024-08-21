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
        // printf("%i\n", led_state);
        // printf("stat1=%i stat2=%i\n", gpio_get(BAT_STAT_1), gpio_get(BAT_STAT_2));
        // info("bootsel=%i\n", gpio_get(2));
        led_board_set(led_state);
        led_state = !led_state;
        i = 0;
    }
}

void wireless_send(uint8_t report_id, void *packet, uint8_t len) {
    // uint8_t payload[32] = {0,};
    // payload[0] = report_id;
    // memcpy(&payload[1], packet, len);
    // bus_spi_write_32(13, NRF24_W_TX_PAYLOAD, payload);
}

void wireless_device_init() {
    printf("INIT: RF device\n");
    // Set config.
    uint8_t config_set = NRF24_REG_CONFIG_PWR_UP | NRF24_REG_CONFIG_EN_CRC;
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_CONFIG, config_set);
    uint8_t config_get = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_CONFIG);
    if (config_get != config_set) error( "RF: NRF24 configuration mismatch\n");
    // Set frequency.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_CHANNEL, 48);
    uint8_t channel = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_CHANNEL);
    // Set retransmission.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_RETRANSMISSION, 0b00000000);
    uint8_t retransmission = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_RETRANSMISSION);
    // Set auto-acknowledge.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_ACK, 0b00000000);
    uint8_t ack = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_ACK);
    // Confirm.
    printf("RF: config=0b%08i\n", bin(config_get));
    printf("RF: channel=24%02i\n", channel);
    printf("RF: retransmission=0b%08i\n", bin(retransmission));
    printf("RF: ack=0b%08i\n", bin(ack));
}

void wireless_host_init() {
    printf("INIT: RF host\n");
    // Set config.
    uint8_t config_set = NRF24_REG_CONFIG_PWR_UP | NRF24_REG_CONFIG_EN_CRC | NRF24_REG_CONFIG_PRIM_RX;
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_CONFIG, config_set);
    uint8_t config_get = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_CONFIG);
    if (config_get != config_set) error( "RF: NRF24 configuration mismatch\n");
    // Set payload size
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_RX_PW_P0, 32);
    uint8_t payload_size = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_RX_PW_P0);
    // Set frequency.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_CHANNEL, 48);
    uint8_t channel = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_CHANNEL);
    // Set retransmission.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_RETRANSMISSION, 0b00000000);
    uint8_t retransmission = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_RETRANSMISSION);
    // Set auto-acknowledge.
    bus_spi_write(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_ACK, 0b00000000);
    uint8_t ack = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_ACK);
    // Confirm.
    printf("RF: config=0b%08i\n", bin(config_get));
    printf("RF: payload_size=%i\n", payload_size);
    printf("RF: channel=24%02i\n", channel);
    printf("RF: retransmission=0b%08i\n", bin(retransmission));
    printf("RF: ack=0b%08i\n", bin(ack));
}

void wireless_device_task() {
    led_task();

    static uint8_t index = 0;
    index++;

    bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_REG_W | NRF24_REG_STATUS, 0);

    // uint8_t payload[32] = {0,};
    // uint64_t timestamp = get_system_clock();
    // memcpy(payload, &timestamp, 8);

    uint8_t payload[32] = {index};

    bus_spi_write_32(PIN_SPI_CS_NRF24, NRF24_W_TX_PAYLOAD, payload);
    // printf("%i %i %i %i\n", payload[0], payload[1], payload[2], payload[3]);

    // uint8_t status = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_STATUS);
    // printf("0b%08i\n", bin(status));

    // uint8_t observe = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_OBSERVE_TX);
    // printf("0b%08i\n", bin(observe));
}

void wireless_host_task() {
    led_task();

    static uint16_t tier0 = 0;
    static uint16_t tier1 = 0;

    while(true) {
        uint8_t status = bus_spi_read_one(PIN_SPI_CS_NRF24, NRF24_REG_STATUS);
        if (status & 0b10) break;  // No payloads pending in pipe 0.

        uint8_t payload[32] = {0,};
        bus_spi_read(PIN_SPI_CS_NRF24, NRF24_R_RX_PAYLOAD, payload, 32);
        // printf("%i %i %i %i\n", payload[0], payload[1], payload[2], payload[3]);

        // Jitter.
        static uint32_t last = 0;
        uint32_t now = time_us_32() / 1000;
        uint32_t elapsed = now - last;
        last = now;

        if (elapsed <= 8) tier0++;
        else if (elapsed <= 16) tier1++;

        // info("%lu ", elapsed);
        // static uint8_t i = 0;
        // i++;
        // if (i > 40) {
        //     i = 0;
        //     info("\n");
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
    static uint64_t last_print = 0;
    if (time_us_64() - last_print > 100000) {
        last_print = time_us_64();
        // float x = 100.0/63;
        while(tier0>0) {
            info("#");
            tier0 -= 1;
        }
        while(tier1>0) {
            info("+");
            tier1 -= 1;
        }
        info("\n");
        tier0 = 0;
        tier1 = 0;
    }
}
