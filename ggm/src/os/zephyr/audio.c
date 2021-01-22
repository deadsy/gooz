//-----------------------------------------------------------------------------
/*

Copyright (c) 2021 Jason T. Harris. (sirmanlypowers@gmail.com)
SPDX-License-Identifier: Apache-2.0

*/
//-----------------------------------------------------------------------------

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/i2s.h>
#include <audio/codec.h>

#include "ggm.h"
#include "audio.h"

//-----------------------------------------------------------------------------

#define I2S_DEVNAME "I2S_1"
#define DAC_DEVNAME "CS43L22"

//-----------------------------------------------------------------------------

int audio_init(struct audio_drv *audio) {
	int err;

	// bind i2s
	audio->i2s = device_get_binding(I2S_DEVNAME);
	if (!audio->i2s) {
		LOG_ERR("can't bind %s", I2S_DEVNAME);
		return -ENXIO;
	}
	// configure the i2s
	struct i2s_config i2s_cfg;
	memset(&i2s_cfg, 0, sizeof(struct i2s_config));
	i2s_cfg.word_size = sizeof(int16_t) * 8;
	i2s_cfg.channels = AudioOutputChannels;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = AudioSampleFrequency;
	i2s_cfg.mem_slab = &audio->buffer_mem_slab;
	i2s_cfg.block_size = AudioBufferBytes;
	i2s_cfg.timeout = 0;

	err = k_mem_slab_init(&audio->buffer_mem_slab, audio->buffer, AudioBufferBytes, AudioOutputChannels * 2);
	if (err != 0) {
		LOG_ERR("k_mem_slab_init failed %d", err);
		return -1;
	}

	err = i2s_configure(audio->i2s, I2S_DIR_TX, &i2s_cfg);
	if (err != 0) {
		LOG_ERR("i2s_configure failed %d", err);
		return -1;
	}
	// bind dac
	audio->dac = device_get_binding(DAC_DEVNAME);
	if (!audio->dac) {
		LOG_ERR("can't bind %s", DAC_DEVNAME);
		return -ENXIO;
	}
	// configure the dac
	struct audio_codec_cfg dac_cfg;
	memset(&dac_cfg, 0, sizeof(struct audio_codec_cfg));

	err = audio_codec_configure(audio->dac, &dac_cfg);
	if (err != 0) {
		LOG_ERR("audio_codec_configure failed %d", err);
		return -1;
	}

	return 0;
}

//-----------------------------------------------------------------------------

int audio_start(struct audio_drv *audio) {
	return 0;
}

//-----------------------------------------------------------------------------
