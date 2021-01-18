/*
 * Copyright (c) 2021 Jason T. Harris
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs43l22

#include <errno.h>

#include <sys/util.h>

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

#include <audio/codec.h>
#include "cs43l22.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(cs43l22);

struct cs4x_driver_config {
	const struct device *i2c_device;
	const char *i2c_dev_name;
	uint8_t i2c_address;
	const struct device *gpio_device;
	const char *gpio_dev_name;
	uint32_t gpio_pin;
	int gpio_flags;
};

struct cs4x_driver_data {
	//struct reg_addr       reg_addr_cache;
};

static struct cs4x_driver_config cs4x_device_config = {
	.i2c_device = NULL,
	.i2c_dev_name = DT_INST_BUS_LABEL(0),
	.i2c_address = DT_INST_REG_ADDR(0),
	.gpio_device = NULL,
	.gpio_dev_name = DT_INST_GPIO_LABEL(0, reset_gpios),
	.gpio_pin = DT_INST_GPIO_PIN(0, reset_gpios),
	.gpio_flags = DT_INST_GPIO_FLAGS(0, reset_gpios),
};

static struct cs4x_driver_data cs4x_device_data;

#define DEV_CFG(dev) \
	((struct codec_driver_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct codec_driver_data *const)(dev)->data)

static int cs4x_initialize(const struct device *dev) {
	//struct codec_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);
	return 0;
}

static int cs4x_configure(const struct device *dev, struct audio_codec_cfg *cfg) {
	//struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);
	return 0;
}
static void cs4x_start_output(const struct device *dev) {
	//struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);

}
static void cs4x_stop_output(const struct device *dev) {
	//struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);
}
static int cs4x_set_property(const struct device *dev, audio_property_t property, audio_channel_t channel, audio_property_value_t val) {
	//struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);
	return 0;
}
static int cs4x_apply_properties(const struct device *dev) {
	//struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	LOG_DBG("%s %d", __FILE__, __LINE__);
	return 0;
}

static const struct audio_codec_api cs4x_driver_api = {
	.configure = cs4x_configure,
	.start_output = cs4x_start_output,
	.stop_output = cs4x_stop_output,
	.set_property = cs4x_set_property,
	.apply_properties = cs4x_apply_properties,
};

DEVICE_AND_API_INIT(cs43l22, DT_INST_LABEL(0), cs4x_initialize, &cs4x_device_data, &cs4x_device_config, POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &cs4x_driver_api);
