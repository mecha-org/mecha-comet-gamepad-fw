/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_INPUT_H_
#define APP_GAMEPAD_INPUT_H_

#include <zephyr/kernel.h>

#include "gamepad/gamepad_state.h"

/**
 * Start collecting input events (buttons and sticks) into a gamepad state.
 *
 * @param changed Semaphore given every time the state changes, so a report can be sent.
 *
 * @retval 0 on success.
 */
int gamepad_input_init(struct k_sem *changed);

/**
 * Get a consistent copy of the current gamepad state.
 *
 * @param state Receives the state.
 */
void gamepad_input_get_state(struct gamepad_state *state);

#endif /* APP_GAMEPAD_INPUT_H_ */
