// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <tusb.h>
#include <device/usbd_pvt.h>
#include "webusb.h"
#include "ctrl.h"
#include "config.h"
#include "profile.h"
#include "hid.h"
#include "tusb_config.h"
#include "common.h"
#include "logging.h"
#include "power.h"
#include "loop.h"
#include "wireless.h"
#include "thumbstick.h"
#include "touch.h"
#include "imu.h"
#include "led.h"

uint8_t webusb_buffer[WEBUSB_BUFFER_SIZE] = {0,};
uint16_t webusb_ptr_in = 0;
uint16_t webusb_ptr_out = 0;
bool webusb_timedout = false;

static bool webusb_pending_empty = false;
static bool webusb_pending_status_share = false;
static uint8_t webusb_pending_config_share = 0;
static uint8_t webusb_pending_profile_share = 0;
static uint8_t webusb_pending_section_share = 0;
static bool webusb_input_stream_enabled = false;
static uint8_t webusb_input_stream_interval_ms = 16;
static uint64_t webusb_input_stream_last_ts = 0;
static bool webusb_pending_input_stream_share = false;
static CtrlInputStream webusb_pending_input_stream = {0,};

#define WEBUSB_INPUT_STREAM_INTERVAL_MS_MIN 8
#define WEBUSB_INPUT_STREAM_INTERVAL_MS_MAX 50
#define WEBUSB_INPUT_STREAM_ROTARY_WINDOW_US 120000

static int8_t webusb_pack_axis(float value) {
    value = constrain(value, -1, 1);
    return (int8_t)(value * BIT_7);
}

static uint8_t webusb_pack_radius(float value) {
    value = constrain(value, 0, 1);
    return (uint8_t)(value * BIT_8);
}

static int16_t webusb_pack_gyro(double value) {
    value = constrain(value, -32767, 32767);
    return (int16_t)value;
}

static int16_t webusb_pack_accel(double value) {
    value = constrain(value, -32767, 32767);
    return (int16_t)value;
}

static ThumbstickPosition webusb_input_stream_get_dhat_position(Profile *profile) {
    bool left = button_is_pressed_physical(&(profile->dhat.left));
    bool right = button_is_pressed_physical(&(profile->dhat.right));
    bool up = button_is_pressed_physical(&(profile->dhat.up));
    bool down = button_is_pressed_physical(&(profile->dhat.down));
    float x = 0;
    float y = 0;
    if (left) x -= 1;
    if (right) x += 1;
    if (up) y -= 1;
    if (down) y += 1;
    if (x && y) {
        x *= 0.70710678;
        y *= 0.70710678;
    }
    float radius = (left || right || up || down) ? 1 : 0;
    float angle = radius ? atan2(x, -y) * (180 / M_PI) : 0;
    return (ThumbstickPosition){x, y, angle, radius};
}

static ThumbstickPosition webusb_input_stream_get_right_position(Profile *profile) {
    #ifdef DEVICE_ALPAKKA_V1
        return thumbstick_get_last_position(&(profile->right_thumbstick));
    #else
        (void)profile;
        return (ThumbstickPosition){0,};
    #endif
}

static void webusb_input_stream_button_set(CtrlInputStream *input_stream, uint8_t bit, bool value) {
    uint8_t index = bit / 8;
    uint8_t mask = 1 << (bit % 8);
    input_stream->buttons[index] = bitmask_set(input_stream->buttons[index], mask, value);
}

static CtrlInputStream webusb_input_stream_snapshot() {
    CtrlInputStream input_stream = {0,};
    Profile *profile = profile_get_active(false);
    if (!profile) return input_stream;
    ThumbstickPosition left = thumbstick_get_last_position(&(profile->left_thumbstick));
    ThumbstickPosition right = webusb_input_stream_get_right_position(profile);
    ThumbstickPosition dhat = webusb_input_stream_get_dhat_position(profile);
    Vector gyro = imu_get_last_gyro_iteration() == loop_get_iteration()
        ? imu_get_last_gyro()
        : imu_read_gyro();
    Vector accel = imu_read_accel();
    int8_t rotary = rotary_get_recent_increment(&(profile->rotary), WEBUSB_INPUT_STREAM_ROTARY_WINDOW_US);
    bool touch = touch_status();

    input_stream.sequence = webusb_pending_input_stream.sequence + 1;
    input_stream.lx = webusb_pack_axis(left.x);
    input_stream.ly = webusb_pack_axis(left.y);
    input_stream.rx = webusb_pack_axis(right.x);
    input_stream.ry = webusb_pack_axis(right.y);
    input_stream.dhx = webusb_pack_axis(dhat.x);
    input_stream.dhy = webusb_pack_axis(dhat.y);
    input_stream.l_radius = webusb_pack_radius(left.radius);
    input_stream.r_radius = webusb_pack_radius(right.radius);
    input_stream.gyro_x = webusb_pack_gyro(gyro.x);
    input_stream.gyro_y = webusb_pack_gyro(gyro.y);
    input_stream.gyro_z = webusb_pack_gyro(gyro.z);
    input_stream.accel_x = webusb_pack_accel(accel.x);
    input_stream.accel_y = webusb_pack_accel(accel.y);
    input_stream.accel_z = webusb_pack_accel(accel.z);
    input_stream.rotary = rotary;
    input_stream.profile_index = profile_get_active_index(false);
    input_stream.led_mask = led_get_visible_mask();

    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_A, button_is_pressed_physical(&(profile->a)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_B, button_is_pressed_physical(&(profile->b)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_X, button_is_pressed_physical(&(profile->x)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_Y, button_is_pressed_physical(&(profile->y)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_DPAD_LEFT, button_is_pressed_physical(&(profile->dpad_left)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_DPAD_RIGHT, button_is_pressed_physical(&(profile->dpad_right)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_DPAD_UP, button_is_pressed_physical(&(profile->dpad_up)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_DPAD_DOWN, button_is_pressed_physical(&(profile->dpad_down)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_SELECT_1, button_is_pressed_physical(&(profile->select_1)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_START_1, button_is_pressed_physical(&(profile->start_1)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_SELECT_2, button_is_pressed_physical(&(profile->select_2)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_START_2, button_is_pressed_physical(&(profile->start_2)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_L1, button_is_pressed_physical(&(profile->l1)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_R1, button_is_pressed_physical(&(profile->r1)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_L2, button_is_pressed_physical(&(profile->l2)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_R2, button_is_pressed_physical(&(profile->r2)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_L4, button_is_pressed_physical(&(profile->l4)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_R4, button_is_pressed_physical(&(profile->r4)));
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_HOME, profile_home_button_pressed());
    webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_L3, button_is_pressed_physical(&(profile->left_thumbstick.push)));
    #ifdef DEVICE_ALPAKKA_V0
        webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_R3, button_is_pressed_physical(&(profile->dhat.push)));
    #else
        webusb_input_stream_button_set(&input_stream, CTRL_INPUT_BUTTON_R3, button_is_pressed_physical(&(profile->right_thumbstick.push)));
    #endif

    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_TOUCH, touch);
    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_ROTARY, rotary != 0);
    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_LEFT_MOVED, left.radius > 0.05);
    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_RIGHT_MOVED, right.radius > 0.05);
    input_stream.flags = bitmask_set(
        input_stream.flags,
        CTRL_INPUT_FLAG_GYRO_ACTIVE,
        fabs(gyro.x) > 64 || fabs(gyro.y) > 64 || fabs(gyro.z) > 64
    );
    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_WIRED, loop_get_device_mode() == WIRED);
    input_stream.flags = bitmask_set(input_stream.flags, CTRL_INPUT_FLAG_WIRELESS, loop_get_device_mode() == WIRELESS);

    return input_stream;
}

bool webusb_get_input_stream_enabled() {
    return webusb_input_stream_enabled;
}

void webusb_input_stream_tick() {
    if (!webusb_input_stream_enabled) return;
    if (loop_get_device_mode() != WIRED) return;
    if (!tud_ready()) return;
    uint64_t now = time_us_64();
    uint64_t interval = (uint64_t)webusb_input_stream_interval_ms * 1000;
    if (webusb_input_stream_last_ts && now < (webusb_input_stream_last_ts + interval)) return;
    webusb_input_stream_last_ts = now;
    webusb_pending_input_stream = webusb_input_stream_snapshot();
    webusb_pending_input_stream_share = true;
}

void webusb_flush_force() {
    uint16_t i = 0;
    while(true) {
        tud_task();
        if (webusb_flush()) break;
        else {
            sleep_ms(1);
            i++;
            if (i>500) {
                // printf("USB: WebUSB timed out\n");
                webusb_timedout = true;
                return;
            }
        }
    }
}

bool webusb_transfer_wired(Ctrl ctrl) {
    // Check if TinyUSB device is ready (connected).
    if (!tud_ready()) return false;
    // Check if USB endpoint is free.
    if (usbd_edpt_busy(0, ADDR_WEBUSB_IN)) return false;
    // Claim USB endpoint.
    if (!usbd_edpt_claim(0, ADDR_WEBUSB_IN)) return false;
    // Transfer data.
    bool success = usbd_edpt_xfer(0, ADDR_WEBUSB_IN, (uint8_t*)&ctrl, ctrl.len+4);
    // Release USB endpoint.
    usbd_edpt_release(0, ADDR_WEBUSB_IN);
    return success;
}

bool webusb_transfer(Ctrl ctrl) {
    #if defined DEVICE_IS_ALPAKKA
        if (loop_get_device_mode() == WIRED) {
            return webusb_transfer_wired(ctrl);
        }
        if (loop_get_device_mode() == WIRELESS) {
            wireless_send_webusb(ctrl);
            return true;
        }
    #elif defined DEVICE_DONGLE
        return webusb_transfer_wired(ctrl);
    #endif
    return false;  // Prevent undefined behavior.
}

bool webusb_flush() {
    // Check if there is anything to flush.
    if (
        webusb_ptr_in == 0 &&
        !webusb_pending_status_share &&
        !webusb_pending_config_share &&
        !webusb_pending_profile_share &&
        !webusb_pending_section_share &&
        !webusb_pending_input_stream_share
    ) {
        return true;
    }
    // Using static to ensure the variable lives long enough in memory to be
    // referenced by the transfer underlying mechanisms.
    static Ctrl ctrl;
    // Generate message.
    if (webusb_pending_empty) {
        ctrl = ctrl_empty();
        bool sent = webusb_transfer(ctrl);
        if (sent) webusb_pending_empty = false;
    } else if (webusb_pending_status_share) {
        ctrl = ctrl_status_share();
        bool sent = webusb_transfer(ctrl);
        if (sent) webusb_pending_status_share = false;
    } else if (webusb_pending_config_share) {
        ctrl = ctrl_config_share(webusb_pending_config_share);
        bool sent = webusb_transfer(ctrl);
        if (sent) webusb_pending_config_share = 0;
    } else if (webusb_pending_profile_share || webusb_pending_section_share) {
        ctrl = ctrl_section_share(webusb_pending_profile_share, webusb_pending_section_share);
        bool sent = webusb_transfer(ctrl);
        if (sent) {
            webusb_pending_profile_share = 0;
            webusb_pending_section_share = 0;
        }
    } else if (webusb_pending_input_stream_share) {
        ctrl = ctrl_input_stream_share(webusb_pending_input_stream);
        bool sent = webusb_transfer(ctrl);
        if (sent) webusb_pending_input_stream_share = false;
    } else {
        uint8_t len = constrain(webusb_ptr_in-webusb_ptr_out, 0, CTRL_MAX_PAYLOAD_SIZE);
        uint8_t *offset_ptr = webusb_buffer + webusb_ptr_out;
        ctrl = ctrl_log(offset_ptr, len);
        bool sent = webusb_transfer(ctrl);
        if (sent) {
            webusb_ptr_out += len;
            if (webusb_ptr_out >= webusb_ptr_in) {
                webusb_ptr_in = 0;
                webusb_ptr_out = 0;
            }
        }
    }
    return true;
}

// Queue data to be sent (flushed) to the app later.
void webusb_write(char *msg) {
    uint16_t len = strlen(msg);
    // If the buffer is full, ignore the latest messages.
    if (webusb_ptr_in + len >= WEBUSB_BUFFER_SIZE-64-1) {
        return;
    }
    // Add message to the buffer.
    memcpy(webusb_buffer + webusb_ptr_in, msg, len);
    webusb_ptr_in += len;
    // If the configuration is still running (still not in the main loop), and
    // the webusb connection has not been flagged as timed out, then force
    // flush directly.
    if (!logging_get_onloop()) {
        if (!webusb_timedout) {
            webusb_flush_force();
        }
    } else {
        webusb_timedout = false;
    }
}

void webusb_handle_status_get() {
    debug("WebUSB: Received status GET from app\n");
    webusb_pending_empty = true;
    webusb_pending_status_share = true;
}

void webusb_handle_status_set(uint8_t time[8]) {
    debug("WebUSB: Received status SET from app\n");
    // set_system_clock(*(uint64_t*)time);  // TODO: Backport from other branch.
}

void webusb_handle_proc(uint8_t proc) {
    if (proc == PROC_RESTART) power_restart();
    else if (proc == PROC_BOOTSEL) power_bootsel();
    else if (proc == PROC_CALIBRATE) config_calibrate();
    else if (proc == PROC_RESET_FACTORY) config_reset_factory();
    else if (proc == PROC_RESET_CONFIG) config_reset_config();
    else if (proc == PROC_RESET_PROFILES) config_reset_profiles();
}

void webusb_handle_config_get(Ctrl_cfg_type key) {
    webusb_pending_config_share = key;
}

void webusb_handle_section_get(uint8_t profile, uint8_t section) {
    webusb_pending_profile_share = profile;
    webusb_pending_section_share = section;
}

void webusb_handle_section_set(uint8_t profileIndex, uint8_t sectionIndex, uint8_t section[58]) {
    debug("WebUSB: Handle profile SET %i %i\n", profileIndex, sectionIndex);
    // Update profile in config.
    CtrlProfile *profile_cfg = config_profile_read(profileIndex);
    memcpy(&profile_cfg->sections[sectionIndex], section, sizeof(CtrlSection));
    // Update profile runtime.
    Profile *profile = profile_get(profileIndex);
    profile->load_from_config(profile, profile_cfg);
    config_profile_set_sync(profileIndex, false);
    // Send back data as confirmation.
    webusb_pending_profile_share = profileIndex;
    webusb_pending_section_share = sectionIndex;
}

void webusb_handle_input_stream_set(uint8_t enabled, uint8_t interval_ms) {
    webusb_input_stream_enabled = enabled;
    webusb_pending_input_stream_share = false;
    webusb_input_stream_last_ts = 0;
    if (interval_ms) {
        webusb_input_stream_interval_ms = constrain(
            interval_ms,
            WEBUSB_INPUT_STREAM_INTERVAL_MS_MIN,
            WEBUSB_INPUT_STREAM_INTERVAL_MS_MAX
        );
    }
}

// Handle incomming message.
void webusb_handle(Ctrl ctrl) {
    if (ctrl.message_type == PROC) webusb_handle_proc(ctrl.payload[0]);
    if (ctrl.message_type == STATUS_GET) webusb_handle_status_get();
    if (ctrl.message_type == STATUS_SET) webusb_handle_status_set(ctrl.payload);
    if (ctrl.message_type == CONFIG_GET) webusb_handle_config_get(ctrl.payload[0]);
    if (ctrl.message_type == CONFIG_SET) {
        webusb_pending_config_share = ctrl.payload[0];  // Echo / confirmation.
        ctrl_config_set(
            ctrl.payload[0],  // Config index.
            ctrl.payload[1],  // Preset index.
            &ctrl.payload[2]  // Preset values. (Reference to sub-array).
        );
    }
    if (ctrl.message_type == SECTION_GET) {
        webusb_handle_section_get(ctrl.payload[0], ctrl.payload[1]);
    }
    if (ctrl.message_type == SECTION_SET) {
        webusb_handle_section_set(
            ctrl.payload[0],
            ctrl.payload[1],
            &ctrl.payload[2]
        );
    }
    if (ctrl.message_type == PROFILE_OVERWRITE) {
        config_profile_overwrite(ctrl.payload[0], ctrl.payload[1]);
    }
    if (ctrl.message_type == INPUT_STREAM_SET && ctrl.len > 0) {
        uint8_t interval_ms = ctrl.len > 1 ? ctrl.payload[1] : webusb_input_stream_interval_ms;
        webusb_handle_input_stream_set(ctrl.payload[0], interval_ms);
    }
}

void webusb_read() {
    // Parse data coming from the app.
    if (!tud_ready() || usbd_edpt_busy(0, ADDR_WEBUSB_OUT)) return;
    // Using static to ensure the variable lives long enough in memory to be
    // referenced by the transfer underlying mechanisms.
    static Ctrl ctrl;
    usbd_edpt_claim(0, ADDR_WEBUSB_OUT);
    usbd_edpt_xfer(0, ADDR_WEBUSB_OUT, (uint8_t*)&ctrl, 64);
    usbd_edpt_release(0, ADDR_WEBUSB_OUT);
    // Wireless switch.
    if (ctrl.protocol_flags == CTRL_FLAG_WIRELESS) {
        // Redirect to wireless controller.
        wireless_send_webusb(ctrl);
    } else {
        // Handle locally.
        webusb_handle(ctrl);
    }
}

void webusb_set_pending_config_share(bool value) {
    webusb_pending_config_share = value;
}
