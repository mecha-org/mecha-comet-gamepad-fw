/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_STATUS_LED_H_
#define APP_STATUS_LED_H_

#ifdef CONFIG_COMET_GAMEPAD_STATUS_LED
/**
 * Start blinking the status LED.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
int status_led_init(void);
#else
static inline int status_led_init(void)
{
	return 0;
}
#endif /* CONFIG_COMET_GAMEPAD_STATUS_LED */

#endif /* APP_STATUS_LED_H_ */
