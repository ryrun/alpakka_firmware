// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

// The starting baseline threshold value when using dynamic.
#define CFG_GEN0_TOUCH_DYNAMIC_MIN 2  // Microseconds
#define CFG_GEN1_TOUCH_DYNAMIC_MIN 10 // Microseconds

// The maximum elapsed time before the measurement is assumed infinite.
#define CFG_TOUCH_TIMEOUT 100  // Microseconds.

// Dynamic threshold algorithm tuning.
#define CFG_TOUCH_DYNAMIC_RATIO 2.0
#define CFG_TOUCH_DYNAMIC_SMOOTH 250

#define CFG_TOUCH_DYNAMIC_PEAK_RATIO 0.7
#define CFG_TOUCH_DYNAMIC_PUSHDOWN_FACTOR 0.5
#define CFG_TOUCH_DYNAMIC_PUSHDOWN_FREQ 250  // Ticks.
#define CFG_TOUCH_DYNAMIC_MIN_AVG_SAMPLES 100

// Debounce.
#define CFG_TOUCH_DEBOUNCE 200  // Milliseconds

// Debug.
#define DEBUG_TOUCH_ELAPSED_FREQ 100  // Ticks.

void touch_init();
void touch_update_threshold();
bool touch_status();
