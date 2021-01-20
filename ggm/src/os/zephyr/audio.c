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

struct audio_drv ggm_audio;

//-----------------------------------------------------------------------------

// cs43l22 DAC setup
static struct audio_codec_cfg dac_cfg = { };

//-----------------------------------------------------------------------------

int audio_init(struct audio_drv *audio) {
	int rc = -1;

	audio->dac = device_get_binding(DT_LABEL(DT_INST(0, cirrus_cs43l22)));
	if (!audio->dac) {
		LOG_ERR("unable to find device %s", DT_LABEL(DT_INST(0, cirrus_cs43l22)));
		goto exit;
	}
	// setup the dac
	int err = audio_codec_configure(audio->dac, &dac_cfg);
	if (err != 0) {
		LOG_ERR("audio_codec_configure failed %d", rc);
		goto exit;
	}

	rc = 0;

 exit:
	return rc;
}

//-----------------------------------------------------------------------------
