//-----------------------------------------------------------------------------
/*

 Copyright (c) 2021 Jason T. Harris. (sirmanlypowers@gmail.com)
 SPDX-License-Identifier: Apache-2.0

*/
//-----------------------------------------------------------------------------

#ifndef AUDIO_H
#define AUDIO_H

//-----------------------------------------------------------------------------

#define AudioOutputChannels 2
#define AudioBufferBytes (AudioBufferSize * sizeof(int16_t))

struct audio_drv {
	const struct device *dac;
	const struct device *i2s;
	struct k_mem_slab buffer_mem_slab;
	int16_t buffer[AudioBufferSize * AudioOutputChannels * 2];
};

//-----------------------------------------------------------------------------

int audio_init(struct audio_drv *audio);
int audio_start(struct audio_drv *audio);

//-----------------------------------------------------------------------------

#endif				// AUDIO_H

//-----------------------------------------------------------------------------
