// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

void wireless_init(bool host);
void wireless_device_task();
void wireless_host_task();
void wireless_esp_flash();

void wireless_send(uint8_t report_id, void *packet, uint8_t len);
