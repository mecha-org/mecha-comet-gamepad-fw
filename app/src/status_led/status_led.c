/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "status_led/status_led.h"

LOG_MODULE_REGISTER(status_led, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

#define BLINK_PERIOD_MS 500

/* Which pin "status_led" is comes from the board devicetree, never from the app. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(status_led), gpios);

static void blink_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(blink_work, blink_handler);

static void blink_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	gpio_pin_toggle_dt(&led);
	k_work_schedule(&blink_work, K_MSEC(BLINK_PERIOD_MS));
}

int status_led_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("Status LED GPIO controller is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure the status LED (%d)", ret);
		return ret;
	}

	k_work_schedule(&blink_work, K_MSEC(BLINK_PERIOD_MS));

	return 0;
}
