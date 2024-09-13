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

uint8_t sens_from_config = 0;
float threshold_ratio = 0;
float low = 10;
uint64_t disengaged_last = 0;

void touch_update_threshold() {
    uint8_t preset = config_get_touch_sens_preset();
    sens_from_config = config_get_touch_sens_value(preset);
    threshold_ratio = CFG_TOUCH_DYNAMIC_RATIO;
    // if (config_get_pcb_gen() == 0) {
    //      // PCB gen 0.
    //     timeout = CFG_GEN0_TOUCH_TIMEOUT;
    //     threshold_ratio = CFG_TOUCH_DYNAMIC_RATIO_GEN0;
    // } else {
    //     // PCB gen 1+.
    //     timeout = CFG_GEN1_TOUCH_TIMEOUT;
    //     threshold_ratio = CFG_TOUCH_DYNAMIC_RATIO_GEN1;
    // }
}

uint8_t touch_get_elapsed() {
    bool timedout = false;
    // Make sure it is down.
    uint32_t timer_start = time_us_32();
    gpio_put(PIN_TOUCH_OUT, false);
    while(gpio_get(PIN_TOUCH_IN) != false) {
        if ((time_us_32() - timer_start) > CFG_TOUCH_TIMEOUT) {
            timedout = true;
            break;
        }
    }
    // Request up and measure.
    uint32_t timer_settled = time_us_32();
    gpio_put(PIN_TOUCH_OUT, true);
    while(gpio_get(PIN_TOUCH_IN) == false) {
        if ((time_us_32() - timer_start) > CFG_TOUCH_TIMEOUT) {
            timedout = true;
            break;
        }
    };
    // Request down for next cycle.
    gpio_put(PIN_TOUCH_OUT, false);
    // Calculate elapsed (ignore settling time).
    uint32_t elapsed;
    if (!timedout) elapsed = time_us_32() - timer_settled;
    else elapsed = CFG_TOUCH_TIMEOUT;
    return elapsed;
}

float touch_get_elapsed_multisample() {
    float total = 0;
    float expected_next = 0;
    uint8_t samples = 0;
    while (expected_next < CFG_TOUCH_TIMEOUT) {
        uint8_t elapsed = touch_get_elapsed();
        total += elapsed;
        expected_next = total + elapsed;
        samples++;
    }
    return total / samples;
}

float touch_get_dynamic_threshold(float elapsed) {
    float mid = low * threshold_ratio;
    if (elapsed < mid && (time_us_64() - disengaged_last > 1000000)) {
        low = smooth(low, elapsed, CFG_TOUCH_DYNAMIC_SMOOTH);
    }
    return mid;
}

bool touch_status() {
    static bool report_last = false;
    static uint64_t report_changed = 0;
    static float elapsed_last = 0;
    // Measure and smooth.
    float elapsed = touch_get_elapsed_multisample();
    float smoothed = (elapsed + elapsed_last) / 2;
    // Determine threshold.
    float threshold = (
        sens_from_config > 0 ?
        sens_from_config :
        touch_get_dynamic_threshold(smoothed)
    );
    // Determine if the surface is considered touched.
    bool report = smoothed >= threshold;
    // Periodic debug log.
    if (logging_has_mask(LOG_TOUCH_SENS)) {
        static uint16_t x = 0;
        x++;
        if (!(x % DEBUG_TOUCH_ELAPSED_FREQ)) {
            info("%.1f / %.1f  L%.1f\n", smoothed, threshold, low);
        }
    }
    // Debounce and report.
    if (report != report_last) {
        uint64_t now = time_us_64();
        if ((now - report_changed) > (CFG_TOUCH_DEBOUNCE * 1000)) {
            report_changed = now;
            report_last = report;
            if (!report) {
                disengaged_last = time_us_64();
            }
            // On-event debug log.
            if (logging_has_mask(LOG_TOUCH_SENS)) {
                info("%.1f / %.1f  L%.1f ", smoothed, threshold, low);
                if (report) info(" TOUCH\n");
                else info(" LIFT\n");
            }
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
