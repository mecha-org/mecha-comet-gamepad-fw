/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_STATE_H_
#define APP_GAMEPAD_STATE_H_

#include <stdint.h>

/**
 * Gamepad buttons. The value is the bit position in the HID report, which is the HID button
 * number minus one. The order follows the Linux gamepad layout, so Linux, SDL and Steam know
 * every button without a controller mapping. The gaps are positions that layout gives to
 * controls this gamepad does not have.
 */
enum gamepad_button {
	GAMEPAD_BUTTON_A = 0,
	GAMEPAD_BUTTON_B = 1,
	GAMEPAD_BUTTON_X = 3,
	GAMEPAD_BUTTON_Y = 4,
	GAMEPAD_BUTTON_LB = 6,
	GAMEPAD_BUTTON_RB = 7,
	GAMEPAD_BUTTON_SELECT = 10,
	GAMEPAD_BUTTON_START = 11,
	GAMEPAD_BUTTON_HOME = 12,
	GAMEPAD_BUTTON_L3 = 13,
};

/** D-pad directions, as bit positions in struct gamepad_state.dpad. */
enum gamepad_dpad {
	GAMEPAD_DPAD_UP,
	GAMEPAD_DPAD_DOWN,
	GAMEPAD_DPAD_LEFT,
	GAMEPAD_DPAD_RIGHT,
};

/** Analog axes, in the order they appear in the HID report. */
enum gamepad_axis {
	GAMEPAD_AXIS_LEFT_X,
	GAMEPAD_AXIS_LEFT_Y,
	GAMEPAD_AXIS_RIGHT_X,
	GAMEPAD_AXIS_RIGHT_Y,
	GAMEPAD_AXIS_COUNT,
};

/** Largest absolute value of an axis. The range is symmetric so that the centre is exactly 0. */
#define GAMEPAD_AXIS_MAX 32767

/** Everything the host can see of the gamepad at one moment. */
struct gamepad_state {
	/** Bit n is set while the button with enum gamepad_button value n is pressed. */
	uint16_t buttons;
	/** Bit n is set while the direction with enum gamepad_dpad value n is pressed. */
	uint8_t dpad;
	/** Axis values, from -GAMEPAD_AXIS_MAX to GAMEPAD_AXIS_MAX. */
	int16_t axis[GAMEPAD_AXIS_COUNT];
};

#endif /* APP_GAMEPAD_STATE_H_ */
