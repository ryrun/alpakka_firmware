// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

#define NRF24_REG_W 0b00100000

#define NRF24_REG_CONFIG 0x00
#define NRF24_REG_CONFIG_PRIM_RX 0b00000001
#define NRF24_REG_CONFIG_PWR_UP  0b00000010
#define NRF24_REG_CONFIG_EN_CRC  0b00001000

#define NRF24_REG_CHANNEL 0x05
#define NRF24_REG_STATUS 0x07
#define NRF24_REG_OBSERVE_TX 0x08
#define NRF24_REG_RX_PW_P0 0x11

// #define NRF24_TX_ADDR 0x10
// #define NRF24_TX_ADDR_VALUE 0x01700000

// #define NRF24_RX_ADDR_P0 0x0A
// #define NRF24_TX_ADDR_VALUE 0x01700000

#define NRF24_R_RX_PAYLOAD 0b01100001
#define NRF24_W_TX_PAYLOAD 0b10100000

#define NRF24_FLUSH_TX 0b11100001
#define NRF24_FLUSH_RX 0b11100010

void wireless_device_init();
void wireless_device_task();
void wireless_host_init();
void wireless_host_task();

void wireless_send(uint8_t report_id, void *packet, uint8_t len);
