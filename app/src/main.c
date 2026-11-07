/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <zephyr/app_version.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "gamepad/gamepad_input.h"
#include "gamepad/gamepad_state.h"
#include "hid/gamepad_hid.h"
#include "status_led/status_led.h"
#include "usb/gamepad_usb.h"

/* Time to wait after a report could not be sent, so errors do not flood the log. */
#define ERROR_RETRY_DELAY_MS 10

LOG_MODULE_REGISTER(comet_gamepad, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* Given when the gamepad state changed or when the host became ready for reports. */
static K_SEM_DEFINE(report_sem, 0, 1);

int main(void)
{
	struct gamepad_state state;
	int ret;

	LOG_INF("Comet Gamepad firmware %s", APP_VERSION_STRING);

	ret = status_led_init();
	if (ret < 0) {
		LOG_WRN("Running without the status LED (%d)", ret);
	}

	ret = gamepad_input_init(&report_sem);
	if (ret < 0) {
		return ret;
	}

	ret = gamepad_hid_init(&report_sem);
	if (ret < 0) {
		return ret;
	}

	ret = gamepad_usb_init();
	if (ret < 0) {
		return ret;
	}

	while (true) {
		k_sem_take(&report_sem, K_FOREVER);

		gamepad_input_get_state(&state);

		/*
		 * This waits until the host has read the report, so the USB polling interval
		 * already limits the report rate. Changes that happen while it waits are merged
		 * into the next report.
		 */
		ret = gamepad_hid_submit(&state);
		if (ret < 0 && ret != -EAGAIN) {
			LOG_WRN("Failed to send the report (%d)", ret);

			/* Do not retry in a tight loop. */
			k_msleep(ERROR_RETRY_DELAY_MS);
		}
	}

	return 0;
}
