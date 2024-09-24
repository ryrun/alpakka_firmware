// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

// The maximum elapsed time before the measurement is assumed infinite.
#define CFG_TOUCH_TIMEOUT 100  // Microseconds.

// The starting baseline threshold value when using dynamic.
#define CFG_GEN0_TOUCH_AUTO_START 2  // Microseconds
#define CFG_GEN1_TOUCH_AUTO_START 10 // Microseconds

// Dynamic threshold algorithm tuning.
#define CFG_TOUCH_AUTO_RATIO 1.5  // Should be configurable?
#define CFG_TOUCH_AUTO_SMOOTH 250

// Debounce.
#define CFG_TOUCH_DEBOUNCE 200  // Milliseconds

// Debug.
#define DEBUG_TOUCH_ELAPSED_FREQ 250  // Ticks.

void touch_init();
void touch_update_threshold();
bool touch_status();
