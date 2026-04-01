
#include "thumbstick.h"
#include "config.h"
#include "hid.h"

static void report_mouse(Thumbstick *self, float delta_angle, uint8_t action, bool is_in_deadzone, bool can_be_delayed) {
    if (is_in_deadzone && !self->rot.did_flick) {
        self->rot.last_angle = self->rot.angle_smooth;
        self->rot.did_flick = true;
        return;
    }
    if (can_be_delayed) {
        self->rot.delayed_angle += delta_angle;
        self->rot.delayed_action = action;
        self->rot.did_flick = true;
        return;
    }
    float pixels_per_degree;
    if (self->rot_rws_enabled) {
        float gyro_sens = config_get_mouse_sens_value(config_get_mouse_sens_preset());
        pixels_per_degree = ((gyro_sens * 1920) / 45) / self->rot_rws;
    } else {
        pixels_per_degree = self->sens_mouse / 360.0f;
    }
    float value = delta_angle * pixels_per_degree;
    if      (action == MOUSE_X)     hid_mouse_move( value, 0);
    else if (action == MOUSE_X_NEG) hid_mouse_move(-value, 0);
    else if (action == MOUSE_Y)     hid_mouse_move(0,  value);
    else if (action == MOUSE_Y_NEG) hid_mouse_move(0, -value);
}

void Thumbstick__report_rotation(Thumbstick *self, ThumbstickPosition pos, float raw_radius) {
    if (fabsf(self->rot.delayed_angle) > 0.1f) {
        float factor = 0.1f;
        float delta_angle = self->rot.delayed_angle * factor;
        self->rot.delayed_angle *= 1-factor;
        report_mouse(self, delta_angle, self->rot.delayed_action, false, false);
    }
    if (pos.radius == 0) {
        self->rot.angle_smooth = 0;
        self->rot.delta_smooth = 0;
        self->rot.tracked_angle = 0;
        self->rot.action = KEY_NONE;
        self->rot.has_action = false;
        self->rot.action_is_secondary = false;
        self->rot.did_flick = false;
    } else {
        if (!self->rot.has_action) {
            // Find the action and entry angle, by walking (clockwise or
            // anticlockwise) from the initial angle to the first cardinal with
            // a defined action.
            uint8_t actions0[4] = {self->up.actions[0], self->right.actions[0], self->down.actions[0], self->left.actions[0]};
            uint8_t actions1[4] = {self->up.actions[1], self->right.actions[1], self->down.actions[1], self->left.actions[1]};
            // Determine entry quarter.
            float angle_360 = pos.angle > 0 ? pos.angle : pos.angle+360;  // Reframe -180:180 to 0:360.
            uint8_t quarter = floorf(angle_360 / 90);  // Get initial angle quarter 0|1|2|3.
            // Walk in circle.
            for(uint8_t i=0; i<4; i++) {
                int8_t ii = self->rot_anticlockwise ? (int8_t)i : (int8_t)-i;
                uint8_t offset0 = (quarter+4 + ii + (self->rot_anticlockwise ? 1 : 0)) % 4;
                uint8_t offset1 = (quarter+4 - ii + (self->rot_anticlockwise ? 0 : 1)) % 4;
                uint8_t offset = 0;
                if (actions0[offset0]) {
                    self->rot.action = actions0[offset0];
                    offset = offset0;
                    self->rot.action_is_secondary = false;
                }
                else if (actions1[offset1]) {
                    self->rot.action = actions1[offset1];
                    offset = offset1;
                    self->rot.action_is_secondary = true;
                }
                else continue;
                self->rot.has_action = true;
                self->rot.entry_angle = offset * 90;  // Cardinal entry angle of defined action.
                if (self->rot.entry_angle >  180) self->rot.entry_angle -= 360;  // Reframe 0:360 to -180:180.
                if (self->rot.entry_angle < -180) self->rot.entry_angle += 360;  // Reframe 0:360 to -180:180.
                self->rot.last_angle = self->rot_any_angle ? pos.angle : self->rot.entry_angle;
                break;
            }
        }
        // Determine angle.
        float angle = pos.angle;
        if (self->rot.angle_smooth == 0) self->rot.angle_smooth = angle;
        if (self->rot.angle_smooth >  90 && angle < -90) angle += 360;
        if (self->rot.angle_smooth < -90 && angle >  90) angle -= 360;
        // Smoothing.
        float smooth_cutoff = fabsf(self->rot.delta_smooth) / TS_ROTATION_SMOOTH_ANGLE;
        float smooth_factor = interpolate(self->rot_smoothing, 1, smooth_cutoff);
        self->rot.angle_smooth = wrap_angle(smooth(self->rot.angle_smooth, angle, smooth_factor));
        // Deadzone.
        bool is_in_deadzone = is_between(
            (self->rot.entry_angle != 180) ? self->rot.angle_smooth : fabs(self->rot.angle_smooth),
            self->rot.entry_angle - self->rot_entry_deadzone / 2,
            self->rot.entry_angle + self->rot_entry_deadzone / 2
        );
        // Delta.
        float delta_angle = wrap_angle(self->rot.angle_smooth - self->rot.last_angle);
        if (self->rot_anticlockwise) delta_angle = -delta_angle;
        self->rot.delta_smooth = smooth(self->rot.delta_smooth, delta_angle, TS_ROTATION_SMOOTH_SPEED);
        self->rot.tracked_angle += (self->rot.action_is_secondary) ? -delta_angle : delta_angle;
        // Mouse.
        if (hid_is_mouse_axis(self->rot.action)) {
            report_mouse(self, delta_angle, self->rot.action, is_in_deadzone, !self->rot.did_flick);
        }
        // Gamepad.
        else if (hid_is_gamepad_axis(self->rot.action)) {
            float value = self->rot.tracked_angle * self->rot_sens_axis / 360.0;
            if (value < 0) {
                // Force disengage.
                self->rot.has_action = false;
                self->rot.tracked_angle = 0;
            }
            value = constrain(value, 0.0, 1.0);
            // if (is_in_deadzone) value = 0;
            hid_gamepad_axis(self->rot.action-GAMEPAD_AXIS_INDEX, value);
            // info("G %.2f\n", value);
        }
        self->rot.last_angle = self->rot.angle_smooth;
    }
}
