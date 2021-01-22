//-----------------------------------------------------------------------------
/*

  Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
  SPDX-License-Identifier: Apache-2.0

*/
//-----------------------------------------------------------------------------

#define GGM_MAIN

#include "ggm.h"
#include "module.h"
#include "audio.h"

//-----------------------------------------------------------------------------

static struct audio_drv ggm_audio;

//-----------------------------------------------------------------------------

static void midi_out(void *arg, const struct event *e, int idx) {
	uint8_t status = e->u.midi.status;
	uint8_t arg0 = e->u.midi.arg0;
	uint8_t arg1 = e->u.midi.arg1;
	LOG_DBG("midi_out[%d] %d:%d:%d", idx, status, arg0, arg1);
}

//-----------------------------------------------------------------------------

int main(void) {
	struct synth *s = NULL;
	struct module *m = NULL;
	int rc;

	LOG_INF("GooGooMuck %s (%s)", GGM_VERSION, CONFIG_BOARD);

	rc = audio_init(&ggm_audio);
	if (rc != 0) {
		LOG_DBG("audio_init failed %d", rc);
		goto exit;
	}

	s = synth_new();
	if (s == NULL) {
		goto exit;
	}

	m = module_root(s, "root/metro", -1);
	if (m == NULL) {
		goto exit;
	}

	rc = synth_set_root(s, m);
	if (rc != 0) {
		goto exit;
	}

	s->midi_out = midi_out;

	for (int i = 0; i < 10; i++) {
		synth_loop(s);
		ggm_mdelay(3);
	}

 exit:
	synth_del(s);

	while (1) {
		ggm_mdelay(1000);
	}

	return 0;
}

//-----------------------------------------------------------------------------
