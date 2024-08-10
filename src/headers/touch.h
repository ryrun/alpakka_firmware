// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

// The baseline threshold value when using dynamic.
#define CFG_GEN0_TOUCH_DYNAMIC_MIN 3  // Microseconds
#define CFG_GEN1_TOUCH_DYNAMIC_MIN 15 // Microseconds

// The maximum elapsed time before the measurement is assumed infinite.
#define CFG_GEN0_TOUCH_TIMEOUT 20  // Microseconds.
#define CFG_GEN1_TOUCH_TIMEOUT 100  // Microseconds.

// Dynamic threshold algorithm tuning.
#define CFG_TOUCH_DYNAMIC_PEAK_RATIO 0.5
#define CFG_TOUCH_DYNAMIC_PUSHDOWN_FREQ 250  // Ticks.
#define CFG_TOUCH_DYNAMIC_PUSHDOWN_HYPERBOLIC 6

// Debounce.
#define CFG_TOUCH_DEBOUNCE 250  // Milliseconds

// Debug.
#define DEBUG_TOUCH_ELAPSED_FREQ 250  // Ticks.

void touch_init();
void touch_update_threshold();
bool touch_status();
