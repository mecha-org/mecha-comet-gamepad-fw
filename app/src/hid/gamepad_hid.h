/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_HID_H_
#define APP_GAMEPAD_HID_H_

#include <zephyr/kernel.h>

#include "gamepad/gamepad_state.h"

/**
 * Register the gamepad with the USB HID class.
 *
 * Call this before gamepad_usb_init().
 *
 * @param ready Semaphore given when the host is ready to receive reports.
 *
 * @retval 0 on success.
 * @retval -ENODEV if the HID device is not ready.
 */
int gamepad_hid_init(struct k_sem *ready);

/**
 * Send the gamepad state to the host as an HID input report.
 *
 * @param state State to send.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if the host is not ready for reports yet.
 */
int gamepad_hid_submit(const struct gamepad_state *state);

#endif /* APP_GAMEPAD_HID_H_ */
