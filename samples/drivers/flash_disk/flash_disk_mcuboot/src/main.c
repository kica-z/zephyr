/*
 * Copyright (c) 2023 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/logging/log.h>

// #if DT_HAS_COMPAT_STATUS_OKAY(zephyr_disk_flash)
// #define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_disk_flash)
// #else
// #error Unsupported flash driver
// #define FLASH_NODE DT_INVALID_NODE
// #endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

// static char const secondary[] = {
// #include "secondary.inc"
// };

int main(void)
{
	int res = -1;
	//const struct device *const slot1_dev = DEVICE_DT_GET(FLASH_NODE);
	//uint16_t const image_size = (uint16_t)sizeof(secondary);
	uint16_t bytes_written = 0;
	// k_msleep(3000);

	// if(!boot_is_img_confirmed()) {
	// 	LOG_INF("Confirming image");
	// } else {
	// 	LOG_INF("Image already confirmed");
	// }

	//LOG_INF("Secondary image starts with: %x", secondary[5]);
	// while(bytes_written < image_size) {
	// 	LOG_INF("Progress %d/%d", bytes_written, image_size);
	// 	res = flash_write(slot1_dev, bytes_written, &secondary[bytes_written], 50);
	// 	if(res != 0) {
	// 		LOG_ERR("Write not possible");
	// 		return -1;
	// 	}
	// 	bytes_written += 50;
	// }
	LOG_INF("Secondary image written successfully");

	// res = flash_img_init_id(&flash_context, DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)));
	// if(res != 0) {
	// 	LOG_ERR("Flash image init not possible for id DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition))");
	// }

	// res = flash_img_buffered_write(&flash_context, secondary, sizeof(secondary), true);
	// if(res != 0) {
	// 	LOG_ERR("Flash image write not possible");
	// }

	// res =  boot_request_upgrade(BOOT_UPGRADE_TEST);
	// if(res != 0) {
	// 	LOG_ERR("Upgrade not possible");
	// }

	return 0;
}
