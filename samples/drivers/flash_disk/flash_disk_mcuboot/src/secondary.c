/*
 * Copyright (c) 2023 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/dfu/mcuboot.h>
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
	int upgrade = -1;

	if(!boot_is_img_confirmed()) {
		LOG_INF("Confirming image");
	} else {
		LOG_INF("Image already confirmed");
	}

	LOG_INF("Upgrading to primary");
	upgrade =  boot_request_upgrade(BOOT_UPGRADE_TEST);
	if(upgrade != 0) {
		LOG_ERR("Upgrade not possible");
	}

	return 0;
}
