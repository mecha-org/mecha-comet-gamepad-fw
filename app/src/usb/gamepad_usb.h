/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_USB_H_
#define APP_GAMEPAD_USB_H_

/**
 * Set up the USB device (identity, strings, configuration) and enable it.
 *
 * The USB classes, such as the HID gamepad, must be registered before this is called.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
int gamepad_usb_init(void);

#endif /* APP_GAMEPAD_USB_H_ */
