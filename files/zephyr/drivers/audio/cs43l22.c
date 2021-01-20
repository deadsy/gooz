//-----------------------------------------------------------------------------
/*

Copyright (c) 2021 Jason T. Harris
SPDX-License-Identifier: Apache-2.0

*/
//-----------------------------------------------------------------------------

#define DT_DRV_COMPAT cirrus_cs43l22

#include <errno.h>
#include <sys/util.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <audio/codec.h>

#include "cs43l22.h"

//-----------------------------------------------------------------------------

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(cs43l22);

//-----------------------------------------------------------------------------

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
	int out;		// output device
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

//-----------------------------------------------------------------------------

#define DEV_CFG(dev) \
	((struct cs4x_driver_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct cs4x_driver_data *const)(dev)->data)

//-----------------------------------------------------------------------------

#define CS43L22_REG_ID                              0x01
#define CS43L22_REG_Power_Ctl_1                     0x02
#define CS43L22_REG_Power_Ctl_2                     0x04
#define CS43L22_REG_Clocking_Ctl                    0x05
#define CS43L22_REG_Interface_Ctl_1                 0x06
#define CS43L22_REG_Interface_Ctl_2                 0x07
#define CS43L22_REG_Passthrough_A_Select            0x08
#define CS43L22_REG_Passthrough_B_Select            0x09
#define CS43L22_REG_Analog_ZC_and_SR_Settings       0x0A
#define CS43L22_REG_Passthrough_Gang_Control        0x0C
#define CS43L22_REG_Playback_Ctl_1                  0x0D
#define CS43L22_REG_Misc_Ctl                        0x0E
#define CS43L22_REG_Playback_Ctl_2                  0x0F
#define CS43L22_REG_Passthrough_A_Vol               0x14
#define CS43L22_REG_Passthrough_B_Vol               0x15
#define CS43L22_REG_PCMA_Vol                        0x1A
#define CS43L22_REG_PCMB_Vol                        0x1B
#define CS43L22_REG_BEEP_Freq_On_Time               0x1C
#define CS43L22_REG_BEEP_Vol_Off_Time               0x1D
#define CS43L22_REG_BEEP_Tone_Cfg                   0x1E
#define CS43L22_REG_Tone_Ctl                        0x1F
#define CS43L22_REG_Master_A_Vol                    0x20
#define CS43L22_REG_Master_B_Vol                    0x21
#define CS43L22_REG_Headphone_A_Volume              0x22
#define CS43L22_REG_Headphone_B_Volume              0x23
#define CS43L22_REG_Speaker_A_Volume                0x24
#define CS43L22_REG_Speaker_B_Volume                0x25
#define CS43L22_REG_Channel_Mixer_Swap              0x26
#define CS43L22_REG_Limit_Ctl_1_Thresholds          0x27
#define CS43L22_REG_Limit_Ctl_2_Release Rate        0x28
#define CS43L22_REG_Limiter_Attack_Rate             0x29
#define CS43L22_REG_Overflow_Clock_Status           0x2E
#define CS43L22_REG_Battery_Compensation            0x2F
#define CS43L22_REG_VP_Battery_Level                0x30
#define CS43L22_REG_Speaker_Status                  0x31
#define CS43L22_REG_Charge_Pump_Frequency           0x34

enum {
	DAC_OUTPUT_OFF,		// must be 0
	DAC_OUTPUT_SPEAKER,
	DAC_OUTPUT_HEADPHONE,
	DAC_OUTPUT_BOTH,
	DAC_OUTPUT_AUTO,
	DAC_OUTPUT_MAX,		// must be last
};

//-----------------------------------------------------------------------------

// read a dac register
static int cs4x_rd(const struct device *dev, uint8_t reg, uint8_t * val) {
	struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	return i2c_reg_read_byte(dev_cfg->i2c_device, dev_cfg->i2c_address, reg, val);
}

// write a dac register
static int cs4x_wr(const struct device *dev, uint8_t reg, uint8_t val) {
	struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	return i2c_reg_write_byte(dev_cfg->i2c_device, dev_cfg->i2c_address, reg, val);
}

// read/modify/write a register
static int cs4x_rmw(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val) {
	struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	return i2c_reg_update_byte(dev_cfg->i2c_device, dev_cfg->i2c_address, reg, mask, val);
}

// set bits in a register
static int cs4x_set(const struct device *dev, uint8_t reg, uint8_t bits) {
	return cs4x_rmw(dev, reg, bits, 0xff);
}

// clear bits in a register
static int cs4x_clr(const struct device *dev, uint8_t reg, uint8_t bits) {
	return cs4x_rmw(dev, reg, bits, 0);
}

//-----------------------------------------------------------------------------

// read and verify the device id
static int cs4x_id(const struct device *dev) {
	uint8_t id;
	int rc;
	rc = cs4x_rd(dev, CS43L22_REG_ID, &id);
	if (rc != 0) {
		return rc;
	}
	if ((id & 0xf8) != 0xe0) {
		return -1;
	}
	return 0;
}

//-----------------------------------------------------------------------------

// set the output device
static int cs4x_output(const struct device *dev, unsigned int out) {
	struct cs4x_driver_data *dev_data = DEV_DATA(dev);
	const uint8_t ctrl[DAC_OUTPUT_MAX] = { 0xff, 0xfa, 0xaf, 0xaa, 0x05 };
	int rc;
	if (out >= DAC_OUTPUT_MAX) {
		out = DAC_OUTPUT_OFF;
	}
	rc = cs4x_wr(dev, CS43L22_REG_Power_Ctl_2, ctrl[out]);
	if (rc != 0) {
		return rc;
	}
	dev_data->out = out;
	return 0;
}

//-----------------------------------------------------------------------------
// volume controls
// Map 0..255 to the control value for a volume register.
// 0 is minium volume (or mute), 255 is maximum volume.

// set the master volume
static int cs4x_master_volume(const struct device *dev, uint8_t ch, int vol) {
	uint32_t x;
	x = (((281 - 52) << 16) / 255) * vol + (52 << 16);
	x >>= 16;
	x &= 255;
	ch = ch ? CS43L22_REG_Master_B_Vol : CS43L22_REG_Master_A_Vol;
	return cs4x_wr(dev, ch, x);
}

// set the headphone volume
static int cs4x_headphone_volume(const struct device *dev, uint8_t ch, int vol) {
	uint32_t x;
	if (vol == 0) {
		x = 1;		// muted
	} else {
		x = (((257 - 52) << 16) / 255) * (vol - 1) + (52 << 16);
		x >>= 16;
		x &= 255;
	}
	ch = ch ? CS43L22_REG_Headphone_B_Volume : CS43L22_REG_Headphone_A_Volume;
	return cs4x_wr(dev, ch, x);
}

// set the speaker volume
static int cs4x_speaker_volume(const struct device *dev, uint8_t ch, int vol) {
	uint32_t x;
	if (vol == 0) {
		x = 1;		// muted
	} else {
		x = (((257 - 64) << 16) / 255) * (vol - 1) + (64 << 16);
		x >>= 16;
		x &= 255;
	}
	ch = ch ? CS43L22_REG_Speaker_B_Volume : CS43L22_REG_Speaker_A_Volume;
	return cs4x_wr(dev, ch, x);
}

// set the pcm volume
static int cs4x_pcm_volume(const struct device *dev, uint8_t ch, int vol) {
	uint32_t x;
	if (vol == 0) {
		x = 0x80;	// muted
	} else {
		x = (((281 - 25) << 16) / (255 - 1)) * (vol - 1) + (25 << 16);
		x >>= 16;
		x &= 255;
	}
	ch = ch ? CS43L22_REG_PCMB_Vol : CS43L22_REG_PCMA_Vol;
	return cs4x_wr(dev, ch, x);
}

//-----------------------------------------------------------------------------
// mute on/off

static int cs4x_mute_on(const struct device *dev) {
	int rc = cs4x_wr(dev, CS43L22_REG_Power_Ctl_2, 0xff);
	rc |= cs4x_headphone_volume(dev, 0, 0);
	rc |= cs4x_headphone_volume(dev, 1, 0);
	return rc;
}

static int cs4x_mute_off(const struct device *dev) {
	struct cs4x_driver_data *dev_data = DEV_DATA(dev);
	int rc = cs4x_headphone_volume(dev, 0, 0xff);
	rc |= cs4x_headphone_volume(dev, 1, 0xff);
	rc |= cs4x_output(dev, dev_data->out);
	return rc;
}

//-----------------------------------------------------------------------------

static int cs4x_initialize(const struct device *dev) {
	struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);

	// bind I2C
	dev_cfg->i2c_device = device_get_binding(dev_cfg->i2c_dev_name);
	if (dev_cfg->i2c_device == NULL) {
		LOG_ERR("I2C device binding error");
		return -ENXIO;
	}
	// bind gpio
	dev_cfg->gpio_device = device_get_binding(dev_cfg->gpio_dev_name);
	if (dev_cfg->gpio_device == NULL) {
		LOG_ERR("GPIO device binding error");
		return -ENXIO;
	}

	return 0;
}

//-----------------------------------------------------------------------------

static int cs4x_configure(const struct device *dev, struct audio_codec_cfg *cfg) {
	struct cs4x_driver_config *const dev_cfg = DEV_CFG(dev);
	struct cs4x_driver_data *dev_data = DEV_DATA(dev);
	int err;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type != AUDIO_DAI_TYPE_I2S");
		return -EINVAL;
	}
	// configure the reset line
	gpio_pin_configure(dev_cfg->gpio_device, dev_cfg->gpio_pin, dev_cfg->gpio_flags | GPIO_OUTPUT_HIGH);

	// 4.9 Recommended Power-Up Sequence (1,2)

	// reset the dac
	gpio_pin_set_raw(dev_cfg->gpio_device, dev_cfg->gpio_pin, 0);
	gpio_pin_set_raw(dev_cfg->gpio_device, dev_cfg->gpio_pin, 1);

	// check the device identity
	if (cs4x_id(dev)) {
		LOG_ERR("bad device id");
		return -ENODEV;
	}
	// 4.9 Recommended Power-Up Sequence (4)
	// 4.11 Required Initialization Settings
	err = cs4x_wr(dev, 0, 0x99);
	err |= cs4x_wr(dev, 0x47, 0x80);
	err |= cs4x_set(dev, 0x32, 1 << 7);
	err |= cs4x_clr(dev, 0x32, 1 << 7);
	err |= cs4x_wr(dev, 0, 0);

	// set the output to AUTO
	err |= cs4x_output(dev, DAC_OUTPUT_AUTO);
	// Clock configuration: Auto detection
	err |= cs4x_wr(dev, CS43L22_REG_Clocking_Ctl, 0x81);
	// Set the Slave Mode and the audio Standard
	err |= cs4x_wr(dev, CS43L22_REG_Interface_Ctl_1, 0x04);

	// Set the Master volume
	err |= cs4x_master_volume(dev, 0, 169);
	err |= cs4x_master_volume(dev, 1, 169);

	// If the Speaker is enabled, set the Mono mode and volume attenuation level
	if (dev_data->out != DAC_OUTPUT_OFF && dev_data->out != DAC_OUTPUT_HEADPHONE) {
		// Set the Speaker Mono mode
		err |= cs4x_wr(dev, CS43L22_REG_Playback_Ctl_2, 0x06);
		err |= cs4x_speaker_volume(dev, 0, 0xff);
		err |= cs4x_speaker_volume(dev, 1, 0xff);
	}
	// Additional configuration for the CODEC. These configurations are done to reduce
	// the time needed for the Codec to power off. If these configurations are removed,
	// then a long delay should be added between powering off the Codec and switching
	// off the I2S peripheral MCLK clock (which is the operating clock for Codec).
	// If this delay is not inserted, then the codec will not shut down properly and
	// it results in high noise after shut down.

	// Disable the analog soft ramp
	err |= cs4x_rmw(dev, CS43L22_REG_Analog_ZC_and_SR_Settings, 0x0f, 0x00);
	// Disable the digital soft ramp
	err |= cs4x_wr(dev, CS43L22_REG_Misc_Ctl, 0x04);
	// Disable the limiter attack level
	err |= cs4x_wr(dev, CS43L22_REG_Limit_Ctl_1_Thresholds, 0x00);
	// Adjust Bass and Treble levels
	err |= cs4x_wr(dev, CS43L22_REG_Tone_Ctl, 0x0f);
	// Adjust PCM volume level
	err |= cs4x_pcm_volume(dev, 0, 241);
	err |= cs4x_pcm_volume(dev, 1, 241);

	if (err) {
		LOG_ERR("configure failed");
		return -EIO;
	}

	return 0;
}

//-----------------------------------------------------------------------------

static void cs4x_start_output(const struct device *dev) {
	// Enable the digital soft ramp
	int rc = cs4x_wr(dev, CS43L22_REG_Misc_Ctl, 0x06);
	// Enable Output device
	rc |= cs4x_mute_off(dev);
	// Power on the Codec
	rc |= cs4x_wr(dev, CS43L22_REG_Power_Ctl_1, 0x9e);
	if (rc != 0) {
		LOG_DBG("cs4x_start_output failed");
	}
}

//-----------------------------------------------------------------------------

static void cs4x_stop_output(const struct device *dev) {
	// Mute the output
	int rc = cs4x_mute_on(dev);
	// Disable the digital soft ramp
	rc |= cs4x_wr(dev, CS43L22_REG_Misc_Ctl, 0x04);
	// Power down the DAC and the speaker (PMDAC and PMSPK bits)
	rc |= cs4x_wr(dev, CS43L22_REG_Power_Ctl_1, 0x9f);
	if (rc != 0) {
		LOG_DBG("cs4x_stop_output failed");
	}
}

//-----------------------------------------------------------------------------
// properties

static int cs4x_set_property(const struct device *dev, audio_property_t property, audio_channel_t channel, audio_property_value_t val) {
	LOG_DBG("%s %d", __FILE__, __LINE__);
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		switch (channel) {
		case AUDIO_CHANNEL_MASTER_LEFT:
			return cs4x_master_volume(dev, 0, val.vol);
		case AUDIO_CHANNEL_MASTER_RIGHT:
			return cs4x_master_volume(dev, 1, val.vol);
		case AUDIO_CHANNEL_HEADPHONE_LEFT:
			return cs4x_headphone_volume(dev, 0, val.vol);
		case AUDIO_CHANNEL_HEADPHONE_RIGHT:
			return cs4x_headphone_volume(dev, 1, val.vol);
		case AUDIO_CHANNEL_SPEAKER_LEFT:
			return cs4x_speaker_volume(dev, 0, val.vol);
		case AUDIO_CHANNEL_SPEAKER_RIGHT:
			return cs4x_speaker_volume(dev, 1, val.vol);
		case AUDIO_CHANNEL_PCM_LEFT:
			return cs4x_pcm_volume(dev, 0, val.vol);
		case AUDIO_CHANNEL_PCM_RIGHT:
			return cs4x_pcm_volume(dev, 1, val.vol);
		default:
			LOG_ERR("bad audio channel %d", channel);
		}
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (val.mute) {
			return cs4x_mute_on(dev);
		}
		return cs4x_mute_off(dev);
	default:
		LOG_ERR("bad audio property %d", property);
	}
	return -1;
}

static int cs4x_apply_properties(const struct device *dev) {
	// do nothing
	return 0;
}

//-----------------------------------------------------------------------------

static const struct audio_codec_api cs4x_driver_api = {
	.configure = cs4x_configure,
	.start_output = cs4x_start_output,
	.stop_output = cs4x_stop_output,
	.set_property = cs4x_set_property,
	.apply_properties = cs4x_apply_properties,
};

//-----------------------------------------------------------------------------

DEVICE_AND_API_INIT(cs43l22, DT_INST_LABEL(0), cs4x_initialize, &cs4x_device_data, &cs4x_device_config, POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &cs4x_driver_api);

//-----------------------------------------------------------------------------
