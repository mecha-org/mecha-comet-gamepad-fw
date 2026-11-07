/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

#include "usb/gamepad_usb.h"

LOG_MODULE_REGISTER(gamepad_usb, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* The gamepad is powered from the USB bus and does not support remote wakeup yet. */
#define GAMEPAD_USB_ATTRIBUTES 0

USBD_DEVICE_DEFINE(gamepad_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_COMET_GAMEPAD_USB_VID, CONFIG_COMET_GAMEPAD_USB_PID);

USBD_DESC_LANG_DEFINE(gamepad_lang);
USBD_DESC_MANUFACTURER_DEFINE(gamepad_manufacturer, CONFIG_COMET_GAMEPAD_USB_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(gamepad_product, CONFIG_COMET_GAMEPAD_USB_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(gamepad_serial);

USBD_DESC_CONFIG_DEFINE(gamepad_fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(gamepad_fs_config, GAMEPAD_USB_ATTRIBUTES,
			  CONFIG_COMET_GAMEPAD_USB_MAX_POWER, &gamepad_fs_cfg_desc);

static void gamepad_usb_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *const msg)
{
	ARG_UNUSED(ctx);

	LOG_INF("USB device message: %s", usbd_msg_type_string(msg->type));
}

int gamepad_usb_init(void)
{
	int ret;

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_lang);
	if (ret != 0) {
		LOG_ERR("Failed to add the language descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_manufacturer);
	if (ret != 0) {
		LOG_ERR("Failed to add the manufacturer descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_product);
	if (ret != 0) {
		LOG_ERR("Failed to add the product descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_serial);
	if (ret != 0) {
		LOG_ERR("Failed to add the serial number descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_configuration(&gamepad_usbd, USBD_SPEED_FS, &gamepad_fs_config);
	if (ret != 0) {
		LOG_ERR("Failed to add the configuration (%d)", ret);
		return ret;
	}

	ret = usbd_register_all_classes(&gamepad_usbd, USBD_SPEED_FS, 1, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to register the USB classes (%d)", ret);
		return ret;
	}

	/* The class, subclass and protocol are given by the interface descriptors. */
	usbd_device_set_code_triple(&gamepad_usbd, USBD_SPEED_FS, 0, 0, 0);
	usbd_self_powered(&gamepad_usbd, false);

	ret = usbd_msg_register_cb(&gamepad_usbd, gamepad_usb_msg_cb);
	if (ret != 0) {
		LOG_ERR("Failed to register the message callback (%d)", ret);
		return ret;
	}

	ret = usbd_init(&gamepad_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to initialize the USB device (%d)", ret);
		return ret;
	}

	ret = usbd_enable(&gamepad_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to enable the USB device (%d)", ret);
		return ret;
	}

	return 0;
}
