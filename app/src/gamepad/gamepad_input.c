/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "gamepad/gamepad_input.h"

/*
 * An axis must move at least this far from its last value to count as a change. Without this, a
 * stick resting between two ADC steps flips between them all the time and the gamepad sends a
 * stream of reports for nothing. The value is 0.2% of the range, too small to notice in use.
 */
#define AXIS_NOISE_THRESHOLD 64

LOG_MODULE_REGISTER(gamepad_input, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

struct button_map {
	uint16_t code;
	enum gamepad_button button;
};

struct dpad_map {
	uint16_t code;
	enum gamepad_dpad direction;
};

struct axis_map {
	uint16_t code;
	enum gamepad_axis axis;
	bool invert;
};

/*
 * The board devicetree names every button by where it sits on the gamepad, using the standard
 * input codes: the face buttons by position (south is the bottom one) and the rest by function.
 * This table turns those into the Xbox style button letters.
 */
static const struct button_map button_map[] = {
	{INPUT_BTN_SOUTH, GAMEPAD_BUTTON_A},       {INPUT_BTN_EAST, GAMEPAD_BUTTON_B},
	{INPUT_BTN_WEST, GAMEPAD_BUTTON_X},        {INPUT_BTN_NORTH, GAMEPAD_BUTTON_Y},
	{INPUT_BTN_TL, GAMEPAD_BUTTON_LB},         {INPUT_BTN_TR, GAMEPAD_BUTTON_RB},
	{INPUT_BTN_SELECT, GAMEPAD_BUTTON_SELECT}, {INPUT_BTN_START, GAMEPAD_BUTTON_START},
	{INPUT_BTN_MODE, GAMEPAD_BUTTON_HOME},     {INPUT_BTN_THUMBL, GAMEPAD_BUTTON_L3},
};

static const struct dpad_map dpad_map[] = {
	{INPUT_BTN_DPAD_UP, GAMEPAD_DPAD_UP},
	{INPUT_BTN_DPAD_DOWN, GAMEPAD_DPAD_DOWN},
	{INPUT_BTN_DPAD_LEFT, GAMEPAD_DPAD_LEFT},
	{INPUT_BTN_DPAD_RIGHT, GAMEPAD_DPAD_RIGHT},
};

/*
 * HID and Linux want an axis that is negative for up and for left. The vertical axis of the left
 * stick reads higher when pushed up, so it is flipped here.
 */
static const struct axis_map axis_map[] = {
	{INPUT_ABS_X, GAMEPAD_AXIS_LEFT_X, false},
	{INPUT_ABS_Y, GAMEPAD_AXIS_LEFT_Y, true},
	{INPUT_ABS_RX, GAMEPAD_AXIS_RIGHT_X, false},
	{INPUT_ABS_RY, GAMEPAD_AXIS_RIGHT_Y, false},
};

static struct gamepad_state state;
static struct k_spinlock state_lock;
static struct k_sem *changed_sem;

/* Returns true if the stored state changed. The caller holds state_lock. */
static bool update_key(uint16_t code, bool pressed)
{
	for (size_t i = 0; i < ARRAY_SIZE(button_map); i++) {
		if (button_map[i].code == code) {
			uint16_t old = state.buttons;

			WRITE_BIT(state.buttons, button_map[i].button, pressed);
			return state.buttons != old;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(dpad_map); i++) {
		if (dpad_map[i].code == code) {
			uint8_t old = state.dpad;

			WRITE_BIT(state.dpad, dpad_map[i].direction, pressed);
			return state.dpad != old;
		}
	}

	LOG_DBG("Ignoring key code 0x%x", code);

	return false;
}

/* The centre and the ends of the range are always accepted, so the stick can reach them exactly. */
static bool is_axis_noise(int16_t old_value, int16_t new_value)
{
	if (new_value == 0 || new_value == GAMEPAD_AXIS_MAX || new_value == -GAMEPAD_AXIS_MAX) {
		return false;
	}

	return abs(new_value - old_value) < AXIS_NOISE_THRESHOLD;
}

/* Returns true if the stored state changed. The caller holds state_lock. */
static bool update_axis(uint16_t code, int32_t value)
{
	for (size_t i = 0; i < ARRAY_SIZE(axis_map); i++) {
		if (axis_map[i].code == code) {
			int16_t *axis = &state.axis[axis_map[i].axis];
			int32_t clamped = CLAMP(value, -GAMEPAD_AXIS_MAX, GAMEPAD_AXIS_MAX);
			/* The range is symmetric, so flipping the sign always fits. */
			int16_t new_value = axis_map[i].invert ? -clamped : clamped;
			bool changed = new_value != *axis && !is_axis_noise(*axis, new_value);

			if (changed) {
				*axis = new_value;
			}

			return changed;
		}
	}

	return false;
}

static void gamepad_input_cb(struct input_event *evt, void *user_data)
{
	k_spinlock_key_t key;
	bool changed = false;

	ARG_UNUSED(user_data);

	key = k_spin_lock(&state_lock);

	switch (evt->type) {
	case INPUT_EV_KEY:
		changed = update_key(evt->code, evt->value != 0);
		break;
	case INPUT_EV_ABS:
		changed = update_axis(evt->code, evt->value);
		break;
	default:
		break;
	}

	k_spin_unlock(&state_lock, key);

	if (changed && changed_sem != NULL) {
		k_sem_give(changed_sem);
	}
}

/* NULL device: receive the events of every input device. */
INPUT_CALLBACK_DEFINE(NULL, gamepad_input_cb, NULL);

int gamepad_input_init(struct k_sem *changed)
{
	changed_sem = changed;

	return 0;
}

void gamepad_input_get_state(struct gamepad_state *out)
{
	k_spinlock_key_t key = k_spin_lock(&state_lock);

	*out = state;
	k_spin_unlock(&state_lock, key);
}
