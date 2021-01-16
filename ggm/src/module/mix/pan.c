/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Left/Right Pan and Volume Module
 * Takes a single audio buffer stream as input and outputs left and right channels.
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct pan {
	float vol;		/* overall volume */
	float pan;		/* pan value 0 == left, 1 == right */
	float new_vol_l;	/* target left channel volume */
	float new_vol_r;	/* target right channel volume */
	float vol_l;		/* left channel volume */
	float vol_r;		/* right channel volume */
};

/******************************************************************************
 * MIDI to port event conversion functions
 */

/* pan_midi_cc converts a cc message to a 0..1 float event */
static void pan_midi_cc(struct event *dst, const struct event *src) {
	event_set_float(dst, event_get_midi_cc_float(src));
}

/******************************************************************************
 * module port functions
 */

static void pan_set(struct module *m) {
	struct pan *this = (struct pan *)m->priv;

	/* Use sin/cos so that l*l + r*r = K (constant power) */
	this->new_vol_l = this->vol * cosf(this->pan);
	this->new_vol_r = this->vol * sinf(this->pan);
}

static void pan_port_vol(struct module *m, const struct event *e) {
	struct pan *this = (struct pan *)m->priv;
	float vol = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("%s:vol %f", m->name, vol);
	/* convert to a linear volume */
	this->vol = map_exp(vol, 0.f, 1.f, -2.f);
	pan_set(m);
}

static void pan_port_pan(struct module *m, const struct event *e) {
	struct pan *this = (struct pan *)m->priv;
	float pan = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("%s:pan %f", m->name, pan);
	this->pan = pan * (0.5f * Pi);
	pan_set(m);
}

/******************************************************************************
 * module functions
 */

static int pan_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct pan *this = ggm_calloc(1, sizeof(struct pan));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set some default values */
	event_in_float(m, "vol", 1.f, NULL);
	event_in_float(m, "pan", 0.5f, NULL);

	return 0;
}

static void pan_free(struct module *m) {
	struct pan *this = (struct pan *)m->priv;

	ggm_free(this);
}

static bool pan_process(struct module *m, float *bufs[]) {
	struct pan *this = (struct pan *)m->priv;
	float *in = bufs[0];
	float *out0 = bufs[1];
	float *out1 = bufs[2];

	/* use a proportional update to control the actual channel volume */
	float err_l = this->new_vol_l - this->vol_l;
	float err_r = this->new_vol_r - this->vol_r;

	this->vol_l += 0.01f * err_l;
	this->vol_r += 0.01f * err_r;

	block_copy_mul_k(out0, in, this->vol_l);
	block_copy_mul_k(out1, in, this->vol_r);
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "in",.type = PORT_TYPE_AUDIO,},
	{.name = "vol",.type = PORT_TYPE_FLOAT,.pf = pan_port_vol,.mf = pan_midi_cc,},
	{.name = "pan",.type = PORT_TYPE_FLOAT,.pf = pan_port_pan,.mf = pan_midi_cc,},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out0",.type = PORT_TYPE_AUDIO,},
	{.name = "out1",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info mix_pan_module = {
	.mname = "mix/pan",
	.iname = "pan",
	.in = in_ports,
	.out = out_ports,
	.alloc = pan_alloc,
	.free = pan_free,
	.process = pan_process,
};

MODULE_REGISTER(mix_pan_module);

/*****************************************************************************/
