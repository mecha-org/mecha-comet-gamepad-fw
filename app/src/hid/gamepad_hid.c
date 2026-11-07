/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/usb/usb_buf.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbd_hid.h>

#include "gamepad/gamepad_state.h"
#include "hid/gamepad_hid.h"
#include "hid/gamepad_report.h"

LOG_MODULE_REGISTER(gamepad_hid, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* Generic Desktop usages that hid.h does not define. */
#define GAMEPAD_HID_USAGE_RX         0x33
#define GAMEPAD_HID_USAGE_RY         0x34
#define GAMEPAD_HID_USAGE_HAT_SWITCH 0x39

/* Flags of the HID Input item. */
#define GAMEPAD_HID_DATA_VAR_ABS      0x02
#define GAMEPAD_HID_CONST_VAR_ABS     0x03
#define GAMEPAD_HID_DATA_VAR_ABS_NULL 0x42

/*
 * HID report descriptor, 11 bytes per report (GAMEPAD_HID_REPORT_SIZE):
 *
 *   bits  0 to 15  buttons
 *   bits 16 to 19  hat switch (the D-pad), 8 means not pressed
 *   bits 20 to 23  padding
 *   bits 24 to 87  axes X, Y, Rx and Ry, 16 bits each, from -32767 to 32767
 *
 * The button numbers follow the Linux gamepad layout (see enum gamepad_button). Numbers that
 * layout gives to controls this gamepad does not have are constant padding bits.
 */
static const uint8_t report_desc[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_GAMEPAD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),

	HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
	HID_LOGICAL_MIN8(0),
	HID_LOGICAL_MAX8(1),
	HID_REPORT_SIZE(1),
	/* Buttons 1 and 2: A and B. */
	HID_USAGE_MIN8(1),
	HID_USAGE_MAX8(2),
	HID_REPORT_COUNT(2),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS),
	/* Button 3 is unused. */
	HID_REPORT_COUNT(1),
	HID_INPUT(GAMEPAD_HID_CONST_VAR_ABS),
	/* Buttons 4 and 5: X and Y. */
	HID_USAGE_MIN8(4),
	HID_USAGE_MAX8(5),
	HID_REPORT_COUNT(2),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS),
	/* Button 6 is unused. */
	HID_REPORT_COUNT(1),
	HID_INPUT(GAMEPAD_HID_CONST_VAR_ABS),
	/* Buttons 7 and 8: LB and RB. */
	HID_USAGE_MIN8(7),
	HID_USAGE_MAX8(8),
	HID_REPORT_COUNT(2),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS),
	/* Buttons 9 and 10 are unused. */
	HID_REPORT_COUNT(2),
	HID_INPUT(GAMEPAD_HID_CONST_VAR_ABS),
	/* Buttons 11 to 14: Select, Start, Home and L3. */
	HID_USAGE_MIN8(11),
	HID_USAGE_MAX8(14),
	HID_REPORT_COUNT(4),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS),
	/* Buttons 15 and 16 are unused. */
	HID_REPORT_COUNT(2),
	HID_INPUT(GAMEPAD_HID_CONST_VAR_ABS),

	/* D-pad as a hat switch. A value outside 0 to 7 means not pressed. */
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(GAMEPAD_HID_USAGE_HAT_SWITCH),
	HID_LOGICAL_MIN8(0),
	HID_LOGICAL_MAX8(7),
	HID_REPORT_SIZE(4),
	HID_REPORT_COUNT(1),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS_NULL),
	HID_REPORT_COUNT(1),
	HID_INPUT(GAMEPAD_HID_CONST_VAR_ABS),

	/* Left stick (X, Y) and right stick (Rx, Ry). */
	HID_USAGE(HID_USAGE_GEN_DESKTOP_X),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),
	HID_USAGE(GAMEPAD_HID_USAGE_RX),
	HID_USAGE(GAMEPAD_HID_USAGE_RY),
	HID_LOGICAL_MIN16(0x01, 0x80),
	HID_LOGICAL_MAX16(0xFF, 0x7F),
	HID_REPORT_SIZE(16),
	HID_REPORT_COUNT(GAMEPAD_AXIS_COUNT),
	HID_INPUT(GAMEPAD_HID_DATA_VAR_ABS),

	HID_END_COLLECTION,
};

#define HID_GAMEPAD_NODE DT_NODELABEL(hid_gamepad)

BUILD_ASSERT(DT_PROP(HID_GAMEPAD_NODE, in_report_size) >= GAMEPAD_HID_REPORT_SIZE,
	     "in-report-size of hid_gamepad in app.overlay is smaller than the report");

static const struct device *const hid_dev = DEVICE_DT_GET(HID_GAMEPAD_NODE);

/* The report buffer must be aligned for the USB controller. It also holds the last report. */
UDC_STATIC_BUF_DEFINE(report_buf, GAMEPAD_HID_REPORT_SIZE)
static struct k_spinlock report_lock;

static atomic_t host_ready;
static struct k_sem *ready_sem;

static void gamepad_hid_iface_ready(const struct device *dev, const bool ready)
{
	ARG_UNUSED(dev);

	LOG_INF("HID interface %s", ready ? "ready" : "not ready");
	atomic_set(&host_ready, ready);

	if (ready && ready_sem != NULL) {
		k_sem_give(ready_sem);
	}
}

static int gamepad_hid_get_report(const struct device *dev, const uint8_t type, const uint8_t id,
				  const uint16_t len, uint8_t *const buf)
{
	k_spinlock_key_t key;
	size_t size = MIN(len, GAMEPAD_HID_REPORT_SIZE);

	ARG_UNUSED(dev);

	if (type != HID_REPORT_TYPE_INPUT || id != 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&report_lock);
	memcpy(buf, report_buf, size);
	k_spin_unlock(&report_lock, key);

	return size;
}

static const struct hid_device_ops hid_ops = {
	.iface_ready = gamepad_hid_iface_ready,
	.get_report = gamepad_hid_get_report,
};

int gamepad_hid_init(struct k_sem *ready)
{
	int ret;

	ready_sem = ready;

	if (!device_is_ready(hid_dev)) {
		LOG_ERR("The HID device is not ready");
		return -ENODEV;
	}

	ret = hid_device_register(hid_dev, report_desc, sizeof(report_desc), &hid_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register the HID device (%d)", ret);
		return ret;
	}

	return 0;
}

int gamepad_hid_submit(const struct gamepad_state *state)
{
	k_spinlock_key_t key;

	if (!atomic_get(&host_ready)) {
		return -EAGAIN;
	}

	key = k_spin_lock(&report_lock);
	gamepad_report_build(state, report_buf);
	k_spin_unlock(&report_lock, key);

	return hid_device_submit_report(hid_dev, GAMEPAD_HID_REPORT_SIZE, report_buf);
}
