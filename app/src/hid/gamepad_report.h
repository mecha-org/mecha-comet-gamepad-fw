/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_REPORT_H_
#define APP_GAMEPAD_REPORT_H_

#include <stdint.h>

#include "gamepad/gamepad_state.h"

/** Size in bytes of the HID input report. */
#define GAMEPAD_HID_REPORT_SIZE 11

/**
 * Pack a gamepad state into the HID input report.
 *
 * Opposite D-pad directions cancel each other and out-of-range axis values are clamped, so the
 * report is always valid. The layout is described by the report descriptor in gamepad_hid.c.
 *
 * @param state  State to pack.
 * @param report Buffer that receives the report.
 */
void gamepad_report_build(const struct gamepad_state *state,
			  uint8_t report[GAMEPAD_HID_REPORT_SIZE]);

#endif /* APP_GAMEPAD_REPORT_H_ */
