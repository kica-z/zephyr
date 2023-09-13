/*
 * Copyright (c) 2023 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_disk_flash)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_disk_flash)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int res = 0;
	const struct device *const dev = DEVICE_DT_GET(FLASH_NODE);

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: device not ready\n", dev->name);
		return 0;
	}

	char testWrite[] = "This was written to flash_disk.";
	char testRead[sizeof(testWrite)];

	res = flash_write(dev, (off_t)20, testWrite, strlen(testWrite));
	if (res != 0) {
		LOG_ERR("Flash write failed");
	}

	res = flash_read(dev, (off_t)20, testRead, sizeof(testRead));
	if (res != 0) {
		LOG_ERR("Flash read failed");
	}

	if (strcmp(testWrite, testRead) != 0) {
		LOG_ERR("Flash read return wrong data: \"%s\" != \"%s\"", testWrite, testRead);
	} else {
		LOG_INF("Read/Write test successful");
	}

	return 0;
}
