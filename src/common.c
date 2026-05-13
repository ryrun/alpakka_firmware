// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include "pico/rand.h"
#include "common.h"

uint32_t bin(uint8_t k) {
    return (k == 0 || k == 1 ? k : ((k % 2) + 10 * bin(k / 2)));
}

uint32_t bin16(uint16_t k) {
    return (k == 0 || k == 1 ? k : ((k % 2) + 10 * bin(k / 2)));
}

uint8_t random8() {
    return (uint8_t)get_rand_32();
}

void print_array(uint8_t *array, uint8_t len) {
    printf("[");
    for(uint8_t i=0; i<len; i++) printf("%i ", array[i]);
    printf("]\n");
}

uint8_t bitmask_set(uint8_t bitmask, uint8_t flag, bool value) {
    bitmask = bitmask & ~flag;  // Always reset bit to zero.
    if (value) bitmask += flag;  // Add / set to one.
    return bitmask;
}

inline bool is_between(float value, float a, float b) {
    if (a < b) {
        return value >= a && value <= b;
    } else {
        return value >= b && value <= a;
    }
}

inline float wrap_angle_180(float angle) {
    if (angle < -180.0f) return angle + 360.0f;
    if (angle > 180.0f) return angle - 360.0f;
    return angle;
}

inline float wrap_angle_360(float angle) {
    if (angle < 0.0f) return angle + 360.0f;
    if (angle > 360.0f) return angle - 360.0f;
    return angle;
}

inline float interpolate(float a, float b, float factor) {
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    return a + (b - a) * factor;
}
