// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

#define XINPUT_REPORT_SIZE 64

#define XINPUT_DPAD_UP        0
#define XINPUT_DPAD_UP_RIGHT  1
#define XINPUT_DPAD_RIGHT     2
#define XINPUT_DPAD_DOWN_RIGHT 3
#define XINPUT_DPAD_DOWN      4
#define XINPUT_DPAD_DOWN_LEFT 5
#define XINPUT_DPAD_LEFT      6
#define XINPUT_DPAD_UP_LEFT   7
#define XINPUT_DPAD_RELEASED  8

typedef struct __packed _XInputReport {
    uint8_t report_id;       // 0x01
    uint8_t lx;              // 0..255
    uint8_t ly;              // 0..255
    uint8_t rx;              // 0..255
    uint8_t ry;              // 0..255
    uint8_t buttons_0;       // dpad + square/cross/circle/triangle
    uint8_t buttons_1;       // shoulder/share/options/sticks/ps/touch
    uint8_t buttons_2;       // PS/touchpad and counter.
    uint8_t lz;              // L2 analog
    uint8_t rz;              // R2 analog
    uint8_t reserved[54];
} XInputReport;

bool xinput_send_report(XInputReport *report);
void xinput_receive_report();
