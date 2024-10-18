// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

#define NRF24_REG_WRITE 0b00100000

#define NRF24_REG_CONFIG 0x00
typedef enum _NRF24_config {
    PRIM_RX = 1,
    PWR_UP = 2,
    EN_CRC = 8,
} NRF24_config;

#define NRF24_REG_RFSETUP 0x06
typedef enum _NRF24_rfsetup {
    POWER_18dB =           0b000,  // Min.
    POWER_12dB =           0b010,
    POWER_6dB =            0b100,
    POWER_0dB =            0b110,  // Max.
    PPL_LOCK =           0b10000,
    RATE_2M =           0b001000,
    RATE_1M =           0b000000,
    RATE_250K =         0b100000,
    CONTINUOUS_WAVE = 0b10000000,
} NRF24_rfsetup;

#define NRF24_REG_ACK 0x01
typedef enum _NRF24_ack {
    ACK_ENABLE_ALL =  0b00111111,
    ACK_DISABLE_ALL = 0b00000000,
} NRF24_ack;

#define NRF24_REG_RETRY 0x04
typedef enum _NRF24_retry {
    RETRY_COUNT_0 =      0b0000,
    RETRY_COUNT_1 =      0b0001,
    RETRY_COUNT_2 =      0b0010,
    RETRY_COUNT_4 =      0b0100,
    RETRY_COUNT_8 =      0b1000,
    RETRY_DELAY_250 =  0b000000,
    RETRY_DELAY_500 =  0b010000,
    RETRY_DELAY_750 =  0b100000,
    RETRY_DELAY_1000 = 0b110000,
} NRF24_retry;

// Other registers
#define NRF24_REG_CHANNEL 0x05
#define NRF24_REG_RX_PW_P0 0x11
#define NRF24_REG_STATUS 0x07
#define NRF24_REG_OBSERVE_TX 0x08

// Actual setting values.
#define NRF24_CONFIG_DEVICE  (PWR_UP | EN_CRC)
#define NRF24_CONFIG_HOST  (PWR_UP | EN_CRC | PRIM_RX)
#define NRF24_RFSETUP  (POWER_0dB | RATE_250K)
#define NRF24_ACK  (ACK_ENABLE_ALL)
#define NRF24_RETRY  (RETRY_COUNT_2 | RETRY_DELAY_1000)  // Delay in microseconds.
#define NRF24_CHANNEL 48  // 24XX MHz.
#define NRF24_PAYLOAD_SIZE 32  // Bytes.


// #define NRF24_TX_ADDR 0x10
// #define NRF24_TX_ADDR_VALUE 0x01700000

// #define NRF24_RX_ADDR_P0 0x0A
// #define NRF24_TX_ADDR_VALUE 0x01700000

#define NRF24_R_RX_PAYLOAD 0b01100001
#define NRF24_W_TX_PAYLOAD 0b10100000

#define NRF24_FLUSH_TX 0b11100001
#define NRF24_FLUSH_RX 0b11100010

void wireless_init(bool host);
void wireless_device_task();
void wireless_host_task();
void wireless_esp_flash();

void wireless_send(uint8_t report_id, void *packet, uint8_t len);
