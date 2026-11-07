/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "hid/gamepad_report.h"

/* Offsets in the report. */
#define REPORT_BUTTONS_OFFSET 0
#define REPORT_HAT_OFFSET     2
#define REPORT_AXES_OFFSET    3

/* A hat switch value outside 0 to 7 means "not pressed". */
#define HAT_NEUTRAL 8

BUILD_ASSERT(REPORT_AXES_OFFSET + GAMEPAD_AXIS_COUNT * sizeof(int16_t) == GAMEPAD_HID_REPORT_SIZE);

/* Hat values count clockwise from up: 0 = up, 1 = up and right, 2 = right, and so on. */
static uint8_t dpad_to_hat(uint8_t dpad)
{
	/* Indexed by [vertical + 1][horizontal + 1], where up and left are negative. */
	static const uint8_t hat[3][3] = {
		{7, 0, 1},
		{6, HAT_NEUTRAL, 2},
		{5, 4, 3},
	};
	int horizontal =
		(int)((dpad >> GAMEPAD_DPAD_RIGHT) & 1U) - (int)((dpad >> GAMEPAD_DPAD_LEFT) & 1U);
	int vertical =
		(int)((dpad >> GAMEPAD_DPAD_DOWN) & 1U) - (int)((dpad >> GAMEPAD_DPAD_UP) & 1U);

	return hat[vertical + 1][horizontal + 1];
}

void gamepad_report_build(const struct gamepad_state *state,
			  uint8_t report[GAMEPAD_HID_REPORT_SIZE])
{
	sys_put_le16(state->buttons, &report[REPORT_BUTTONS_OFFSET]);
	report[REPORT_HAT_OFFSET] = dpad_to_hat(state->dpad);

	for (size_t i = 0; i < GAMEPAD_AXIS_COUNT; i++) {
		int16_t value = CLAMP(state->axis[i], -GAMEPAD_AXIS_MAX, GAMEPAD_AXIS_MAX);

		sys_put_le16((uint16_t)value, &report[REPORT_AXES_OFFSET + i * sizeof(int16_t)]);
	}
}
