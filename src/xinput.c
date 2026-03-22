// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <tusb.h>
#include "xinput.h"

bool xinput_send_report(XInputReport *report) {
    if (!tud_ready()) return false;
    if (!tud_hid_n_ready(1)) return false;  // HID instance 1 = PlayStation-compatible gamepad.
    return tud_hid_n_report(1, report->report_id, report, XINPUT_REPORT_SIZE);
}

void xinput_receive_report() {}
