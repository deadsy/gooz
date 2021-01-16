/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BiQuad Filter
 * See: http://www.earlevel.com/main/2003/02/28/biquads/
 */

#include "ggm.h"
#include "filter/filter.h"

/******************************************************************************
 * private state
 */

struct biquad {
	float a0, a1, a2;	/* zero coefficients */
	float b1, b2;		/* pole coefficients */
	float d1, d2;		/* delay variables */
};

/******************************************************************************
 * module port functions
 */

static void biquad_port_cutoff(struct module *m, const struct event *e) {
	// struct biquad *this = (struct biquad *)m->priv;
	float cutoff = clampf(event_get_float(e), 0.f, 0.5f * AudioSampleFrequency);

	LOG_INF("set cutoff frequency %f Hz", cutoff);
	/* TODO */
}

static void biquad_port_resonance(struct module *m, const struct event *e) {
	// struct biquad *this = (struct biquad *)m->priv;
	float resonance = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("set resonance %f", resonance);
	/* TODO */
}

/******************************************************************************
 * module functions
 */

static int biquad_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct biquad *this = ggm_calloc(1, sizeof(struct biquad));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	return 0;
}

static void biquad_free(struct module *m) {
	struct biquad *this = (struct biquad *)m->priv;

	ggm_free(this);
}

static bool biquad_process(struct module *m, float *bufs[]) {
	struct biquad *this = (struct biquad *)m->priv;
	float *in = bufs[0];
	float *out = bufs[1];
	float a0 = this->a0;
	float a1 = this->a1;
	float a2 = this->a2;
	float b1 = this->b1;
	float b2 = this->b2;
	float d1 = this->d1;
	float d2 = this->d2;

	for (int i = 0; i < AudioBufferSize; i++) {
		/* direct form 2 */
		float d0 = in[i] - (b1 * d1) - (b2 * d2);
		out[i] = (a0 * d0) + (a1 * d1) + (a2 * d2);
		d1 = d0;
		d2 = d1;
	}

	/* store the delay variables */
	this->d1 = d1;
	this->d2 = d2;

	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "in",.type = PORT_TYPE_AUDIO,},
	{.name = "cutoff",.type = PORT_TYPE_FLOAT,.pf = biquad_port_cutoff},
	{.name = "resonance",.type = PORT_TYPE_FLOAT,.pf = biquad_port_resonance},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info filter_biquad_module = {
	.mname = "filter/biquad",
	.iname = "biquad",
	.in = in_ports,
	.out = out_ports,
	.alloc = biquad_alloc,
	.free = biquad_free,
	.process = biquad_process,
};

MODULE_REGISTER(filter_biquad_module);

/*****************************************************************************/
