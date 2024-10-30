// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

#define ESP_BOOTLOADER_ADDR 0x0
#define ESP_PARTITION_ADDR 0x8000
#define ESP_FW_ADDR 0x10000
#define ESP_BOOTLOADER_BAUD 74880
#define ESP_FLASHER_BAUD 115200
#define ESP_FLASHER_BAUD_MAX 115200 * 16
#define ESP_RESTART_SETTLE 100  // Milliseconds.

void wireless_init(bool host);
void wireless_device_task();
void wireless_host_task();
void wireless_esp_flash();

void wireless_send(uint8_t report_id, void *packet, uint8_t len);
