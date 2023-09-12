/*
 * Copyright (c) 2023 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_disk_flash

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/disk.h>

/* TODO Malloc? Is unecessarily big depending on use case */
BUILD_ASSERT(CONFIG_FLASH_DISK_BUFFER_SIZE > 0, "CONFIG_FLASH_DISK_BUFFER_SIZE needs to be > 0");
LOG_MODULE_REGISTER(disk_flash, CONFIG_FLASH_LOG_LEVEL);

struct flash_disk_dev_data {
	struct k_sem sem;
	char buf[CONFIG_FLASH_DISK_BUFFER_SIZE];
	uint32_t disk_total_sector_cnt;
	uint32_t disk_sector_size;
};

struct flash_disk_dev_config {
	int flash_size;
	struct flash_parameters flash_parameters;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout flash_pages_layout;
#endif
	char disk_name[Z_DEVICE_MAX_NAME_LEN];
	int disk_offset;
};

static void acquire_device(const struct device *dev);
static void release_device(const struct device *dev);

static int flash_disk_init(const struct device *dev)
{
	struct flash_disk_dev_data *data = dev->data;
	const struct flash_disk_dev_config *cfg = dev->config;
	LOG_DBG("flash_disk_init called");

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_sem_init(&data->sem, 1, K_SEM_MAX_LIMIT);
	}

	int res = disk_access_init(cfg->disk_name);
	if (res != 0) {
		LOG_ERR("init disk failed: %d", res);
		return -EINVAL;
	}

	res = disk_access_ioctl(cfg->disk_name, DISK_IOCTL_GET_SECTOR_COUNT,
				&data->disk_total_sector_cnt);
	if (res != 0) {
		LOG_ERR("read total sector cnt failed: %d", res);
		return -EINVAL;
	}

	res = disk_access_ioctl(cfg->disk_name, DISK_IOCTL_GET_SECTOR_SIZE,
				&data->disk_sector_size);
	if (res != 0) {
		LOG_ERR("read sector size failed: %d", res);
		return -EINVAL;
	}

	if (data->disk_sector_size > sizeof(data->buf)) {
		LOG_ERR("sector size %u of disk to big for buffer %u", data->disk_sector_size,
			(uint32_t)sizeof(data->buf));
		return -EINVAL;
	}

	if (cfg->disk_offset % data->disk_sector_size != 0) {
		LOG_WRN("offset is not aligned with disk sectors (disk sector size: %u)",
			data->disk_sector_size);
	}

	return 0;
}

static int flash_disk_read(const struct device *dev, off_t offset, void *dst, size_t len)
{
	int ret = 0;
	acquire_device(dev);
	LOG_DBG("flash_disk_read called");
	char *dstBuf = (char *)dst;
	struct flash_disk_dev_data *data = dev->data;
	const struct flash_disk_dev_config *cfg = dev->config;

	if (data->disk_sector_size == 0) {
		ret = -EINVAL;
		goto release_device;
	}

	uint32_t write_cursor = 0;
	while (write_cursor < len) {
		uint32_t read_cursor = cfg->disk_offset + offset + write_cursor;

		uint32_t read_sector = read_cursor / data->disk_sector_size;
		uint32_t read_padding = read_cursor % data->disk_sector_size;

		if (read_sector < 0 || read_sector >= data->disk_total_sector_cnt) {
			ret = -EINVAL;
			goto release_device;
		}

		if (disk_access_read(cfg->disk_name, data->buf, read_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		uint32_t len_write = (len - write_cursor);
		if (len_write > sizeof(data->buf) - read_padding) {
			len_write = sizeof(data->buf) - read_padding;
		}
		memcpy(&dstBuf[write_cursor], &data->buf[read_padding], len_write);
		write_cursor += len_write;
	}
release_device:
	release_device(dev);
	return ret;
}

static int flash_disk_write(const struct device *dev, off_t offset, const void *src, size_t len)
{
	int ret = 0;
	acquire_device(dev);
	LOG_DBG("flash_disk_write called");
	const char *srcBuf = (const char *)src;
	struct flash_disk_dev_data *data = dev->data;
	const struct flash_disk_dev_config *cfg = dev->config;
	if (data->disk_sector_size == 0) {
		ret = -EINVAL;
		goto release_device;
	}

	uint32_t read_cursor = 0;
	while (read_cursor < len) {
		uint32_t write_cursor = cfg->disk_offset + offset + read_cursor;

		uint32_t write_sector = write_cursor / data->disk_sector_size;
		uint32_t write_padding = write_cursor % data->disk_sector_size;

		if (write_sector < 0 || write_sector >= data->disk_total_sector_cnt) {
			ret = -EINVAL;
			goto release_device;
		}

		if (disk_access_read(cfg->disk_name, data->buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		uint32_t len_write = (len - read_cursor);
		if (len_write > sizeof(data->buf) - write_padding) {
			len_write = sizeof(data->buf) - write_padding;
		}
		memcpy(&data->buf[write_padding], &srcBuf[read_cursor], len_write);

		if (disk_access_write(cfg->disk_name, data->buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}
		read_cursor += len_write;
	}
release_device:
	release_device(dev);
	return ret;
}

static int flash_disk_erase(const struct device *dev, off_t offset, size_t size)
{
	int ret = 0;
	acquire_device(dev);
	LOG_DBG("flash_disk_erase called");
	struct flash_disk_dev_data *data = dev->data;
	const struct flash_disk_dev_config *cfg = dev->config;

	if (data->disk_sector_size == 0) {
		return -EINVAL;
	}

	uint32_t read_cursor = 0;
	while (read_cursor < size) {
		uint32_t write_cursor = cfg->disk_offset + offset + read_cursor;

		uint32_t write_sector = write_cursor / data->disk_sector_size;
		uint32_t write_padding = write_cursor % data->disk_sector_size;

		if (write_sector < 0 || write_sector >= data->disk_total_sector_cnt) {
			ret = -EINVAL;
			goto release_device;
		}

		if (disk_access_read(cfg->disk_name, data->buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		uint32_t len_write = (size - read_cursor);
		if (len_write > sizeof(data->buf) - write_padding) {
			len_write = sizeof(data->buf) - write_padding;
		}
		// NULL works here, because is ignored, but is not beautiful
		memset(&data->buf[write_padding], cfg->flash_parameters.erase_value, len_write);

		if (disk_access_write(cfg->disk_name, data->buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}
		read_cursor += len_write;
	}
release_device:
	release_device(dev);
	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_disk_pages_layout(const struct device *dev,
				    const struct flash_pages_layout **layout, size_t *layout_size)
{
	LOG_DBG("flash_disk_pages_layout called");
	const struct flash_disk_dev_config *cfg = dev->config;

	*layout = &cfg->flash_pages_layout;
	*layout_size = 1;
	return;
}
#endif

static const struct flash_parameters *flash_disk_get_parameters(const struct device *dev)
{
	LOG_DBG("flash_disk_get_parameters called");
	const struct flash_disk_dev_config *cfg = dev->config;
	return &cfg->flash_parameters;
}

static const struct flash_driver_api disk_flash_api = {
	.read = flash_disk_read,
	.write = flash_disk_write,
	.erase = flash_disk_erase,
	.get_parameters = flash_disk_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_disk_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = NULL,
	.read_jedec_id = NULL,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = NULL,
#endif
};

static void acquire_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct flash_disk_dev_data *const data = dev->data;

		k_sem_take(&data->sem, K_FOREVER);
	}
}

static void release_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct flash_disk_dev_data *const data = dev->data;

		k_sem_give(&data->sem);
	}
}

/* clang-format off */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define DISK_FLASH_DEFINE(index)                                                            \
	static struct flash_disk_dev_data disk_flash_data_##index;                              \
                                                                                            \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, size) > 0, "flash_size needs to be > 0");                       \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, write_block_size) > 0, "write_block_size needs to be > 0");     \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, erase_value) > 0 && DT_INST_PROP(index, erase_value) <= 0xFF,   \
        "erase_value has to be between 0x00 and 0xFF");                                     \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, page_size) > 0, "pages_size needs to be > 0");                  \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, disk_offset) >= 0, "disk_offset needs to be >= 0");             \
    static const struct flash_disk_dev_config disk_flash_config_##index = {                 \
        .flash_size = DT_INST_PROP(index, size),                                            \
        .flash_parameters = {                                                               \
            .write_block_size = DT_INST_PROP(index, write_block_size),                      \
            .erase_value = DT_INST_PROP(index, erase_value),                                \
        },                                                                                  \
        .flash_pages_layout = {                                                             \
            .pages_size = DT_INST_PROP(index, page_size),                                   \
            .pages_count = (DT_INST_PROP(index, size) / 8) / DT_INST_PROP(index, page_size),\
        },                                                                                  \
        .disk_name = DT_INST_PROP(index, disk_name),                                        \
        .disk_offset = DT_INST_PROP(index, disk_offset),                                    \
    };                                                                                      \
                                                                                            \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, init_priority) >= 0, "init_priority needs to be >= 0");         \
    DEVICE_DT_INST_DEFINE(index, &flash_disk_init, NULL,                                    \
            &disk_flash_data_##index, &disk_flash_config_##index,                           \
            APPLICATION, DT_INST_PROP(index, init_priority),                                \
            &disk_flash_api);
#else
#define DISK_FLASH_DEFINE(index)                                                            \
	static struct flash_disk_dev_data disk_flash_data_##index;                              \
                                                                                            \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, size) > 0, "flash_size needs to be > 0");                       \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, write_block_size) > 0, "write_block_size needs to be > 0");     \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, erase_value) > 0 && DT_INST_PROP(index, erase_value) <= 0xFF,   \
        "erase_value has to be between 0x00 and 0xFF");                                     \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, disk_offset) >= 0, "disk_offset needs to be >= 0");             \
    static const struct flash_disk_dev_config disk_flash_config_##index = {                 \
        .flash_size = DT_INST_PROP(index, size),                                            \
        .flash_parameters = {                                                               \
            .write_block_size = DT_INST_PROP(index, write_block_size),                      \
            .erase_value = DT_INST_PROP(index, erase_value),                                \
        },                                                                                  \
        .disk_name = DT_INST_PROP(index, disk_name),                                        \
        .disk_offset = DT_INST_PROP(index, disk_offset),                                    \
    };                                                                                      \
                                                                                            \
    BUILD_ASSERT(                                                                           \
        DT_INST_PROP(index, init_priority) >= 0, "init_priority needs to be >= 0");         \
    DEVICE_DT_INST_DEFINE(index, &flash_disk_init, NULL,                                    \
            &disk_flash_data_##index, &disk_flash_config_##index,                           \
            APPLICATION, DT_INST_PROP(index, init_priority),                                \
            &disk_flash_api);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DISK_FLASH_DEFINE)
