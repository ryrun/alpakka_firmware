// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <math.h>
#include <pico/stdlib.h>
#include "config.h"
#include "touch.h"
#include "pin.h"
#include "common.h"
#include "logging.h"

uint8_t loglevel = 0;
uint8_t sens_from_config = 0;
uint8_t dynamic_min = 0;
uint8_t timeout = 0;
uint8_t samples = 0;
float threshold = 0;

void touch_update_threshold() {
    uint8_t preset = config_get_touch_sens_preset();
    sens_from_config = config_get_touch_sens_value(preset);
    if (config_get_pcb_gen() == 0) {
         // PCB gen 0.
        timeout = CFG_GEN0_TOUCH_TIMEOUT;
        dynamic_min = CFG_GEN0_TOUCH_DYNAMIC_MIN;
    } else {
        // PCB gen 1+.
        timeout = CFG_GEN1_TOUCH_TIMEOUT;
        dynamic_min = CFG_GEN1_TOUCH_DYNAMIC_MIN;
    }
}

uint8_t touch_get_elapsed() {
    uint32_t time_start = time_us_32();
    bool timedout = false;
    // Make sure it is down.
    gpio_put(PIN_TOUCH_OUT, false);
    while(gpio_get(PIN_TOUCH_IN) != false) {}
    // Request up and measure.
    gpio_put(PIN_TOUCH_OUT, true);
    while(gpio_get(PIN_TOUCH_IN) == false) {
        if ((time_us_32() - time_start) > timeout) {
            timedout = true;
            break;
        }
    };
    // Request down for next cycle.
    gpio_put(PIN_TOUCH_OUT, false);
    // Calculate elapsed.
    uint32_t elapsed = time_us_32() - time_start;
    if (timedout) elapsed = timeout;
    return elapsed;
}

uint8_t touch_get_elapsed_multisample() {
    uint16_t total = 0;
    samples = 0;
    while (total < timeout) {
        samples++;
        total += touch_get_elapsed();
    }
    return total / samples;
}

float touch_get_dynamic_threshold(uint8_t elapsed) {
    static float peak = 0;
    static uint8_t elapsed_prev = 0;
    static uint16_t ticks = 0;
    ticks++;
    // Push down:
    // A periodic but slow decrease of the peak, to avoid ever-growing peaks
    // in long gaming sessions. The hyperbolic function makes it so the
    // decrease is faster the more it deviates from the minimum.
    if (!(ticks % CFG_TOUCH_DYNAMIC_PUSHDOWN_FREQ)) {
        float x = dynamic_min / peak;
        float factor = tanhf(x * CFG_TOUCH_DYNAMIC_PUSHDOWN_HYPERBOLIC);
        peak = max(dynamic_min, peak * factor);
    }
    // Push up:
    // Raise the peak as soon as the current peak has been exceeded twice.
    // (Twice to avoid fluke peaks).
    if (elapsed > peak && elapsed_prev > peak) {
        peak = min(elapsed, elapsed_prev);
    }
    // Return.
    elapsed_prev = elapsed;
    return max(dynamic_min, peak * CFG_TOUCH_DYNAMIC_PEAK_RATIO);
}

bool touch_status() {
    static bool report_last = false;
    static uint64_t last_change = 0;
    static uint8_t elapsed_last = 0;
    // Measure and smooth.
    uint8_t elapsed = touch_get_elapsed_multisample();
    uint8_t smoothed = (elapsed + elapsed_last) / 2;
    // Determine threshold.
    threshold = (
        sens_from_config > 0 ?
        sens_from_config :
        touch_get_dynamic_threshold(smoothed)
    );
    // Determine if the surface is considered touched.
    bool report = smoothed >= threshold;
    // Debug.
    if (1 || loglevel >= 2) {  //////
        static uint16_t x = 0;
        x++;
        if (!(x % DEBUG_TOUCH_ELAPSED_FREQ)) {
            info("%i (%i %i)  T=%.0f M=%i\n", smoothed, elapsed, elapsed_last, threshold, samples);
        }
    }
    // Debounce and report.
    if (report != report_last) {
        uint64_t now = time_us_64();
        if ((now - last_change) > (CFG_TOUCH_DEBOUNCE * 1000)) {
            last_change = now;
            report_last = report;
            // TEST MORE DEBUG.
            info("%i (%i %i)  T=%.0f M=%i ", smoothed, elapsed, elapsed_last, threshold, samples);
            if (report)  info("TOUCH\n");
            if (!report) info("LIFT\n");
            // Report.
            elapsed_last = elapsed;
            return report;
        }
    }
    elapsed_last = elapsed;
}

void touch_log_baseline() {
    uint8_t t0 = touch_get_elapsed();
    sleep_ms(CFG_TICK_INTERVAL);
    uint8_t t1 = touch_get_elapsed();
    sleep_ms(CFG_TICK_INTERVAL);
    uint8_t t2 = touch_get_elapsed();
    sleep_ms(CFG_TICK_INTERVAL);
    uint8_t t3 = touch_get_elapsed();
    info("  Touch readings: %ius %ius %ius %ius\n", t0, t1, t2, t3);
}

void touch_init() {
    info("INIT: Touch\n");
    gpio_init(PIN_TOUCH_OUT);
    gpio_set_dir(PIN_TOUCH_OUT, GPIO_OUT);
    gpio_init(PIN_TOUCH_IN);
    gpio_set_dir(PIN_TOUCH_IN, GPIO_IN);
    gpio_set_pulls(PIN_TOUCH_IN, false, false);
    touch_update_threshold();
    touch_log_baseline();
}
