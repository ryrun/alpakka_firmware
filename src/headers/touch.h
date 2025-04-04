// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

// The maximum elapsed time before the measurement is assumed infinite.
// Also the time limit to perform multiple measurements.
#define TOUCH_TIMEOUT 100  // Microseconds.

// The starting baseline threshold value when using dynamic.
#define TOUCH_AUTO_START_V0_GEN0 2  // Microseconds.
#define TOUCH_AUTO_START_V0_GEN1 10 // Microseconds.
#define TOUCH_AUTO_START_V1_GEN0 10 // Microseconds.

// Dynamic threshold algorithm tuning (Wired).
#define TOUCH_AUTO_RATIO_WIRED_PRESET1 2.0
#define TOUCH_AUTO_RATIO_WIRED_PRESET2 1.5
#define TOUCH_AUTO_RATIO_WIRED_PRESET3 1.25

// Dynamic threshold algorithm tuning (Wireless).
#define TOUCH_AUTO_RATIO_WIRELESS_PRESET1 1.25
#define TOUCH_AUTO_RATIO_WIRELESS_PRESET2 1.20
#define TOUCH_AUTO_RATIO_WIRELESS_PRESET3 1.15

// Smooting of the dynamic threshold (not the sampling).
#define TOUCH_AUTO_SMOOTH  (CFG_TICK_FREQUENCY)  // 1 second.

// Debounce.
#define TOUCH_DEBOUNCE 100  // Milliseconds.

// Debug.
#define TOUCH_DEBUG_FREQ 1000  // Milliseconds.

void touch_init();
void touch_load_from_config();
bool touch_status();
