/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Low Freqeuncy Oscillator
 *
 */

#include "ggm.h"
#include "osc/osc.h"

/******************************************************************************
 * private state
 */

struct lfo {
	int shape;              /* wave shape */
	float depth;            /* wave amplitude */
	uint32_t x;             /* current x-value */
	uint32_t xstep;         /* current x-step */
	uint32_t rand_state;    /* random state for s&h */
};

/******************************************************************************
 * module port functions
 */

static void lfo_port_rate(struct module *m, const struct event *e)
{
	struct lfo *this = (struct lfo *)m->priv;
	float rate = clampf_lo(event_get_float(e), 0.f);

	LOG_INF("set rate %f Hz", rate);
	this->xstep = (uint32_t)(rate * FrequencyScale);
}

static void lfo_port_depth(struct module *m, const struct event *e)
{
	struct lfo *this = (struct lfo *)m->priv;
	float depth = clampf_lo(event_get_float(e), 0.f);

	LOG_INF("set depth %f", depth);
	this->depth = depth;
}

static void lfo_port_shape(struct module *m, const struct event *e)
{
	struct lfo *this = (struct lfo *)m->priv;
	int shape = clampi(event_get_int(e), 0, LFO_SHAPE_MAX - 1);

	LOG_INF("set wave shape %d", shape);
	this->shape = shape;
}

static void lfo_port_sync(struct module *m, const struct event *e)
{
	if (event_get_bool(e)) {
		struct lfo *this = (struct lfo *)m->priv;
		LOG_INF("lfo sync");
		this->x = 0;
	}
}

/******************************************************************************
 * module functions
 */

static int lfo_alloc(struct module *m, va_list vargs)
{
	/* allocate the private data */
	struct lfo *this = ggm_calloc(1, sizeof(struct lfo));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	return 0;
}

static void lfo_free(struct module *m)
{
	ggm_free(m->priv);
}

static float lfo_sample(struct module *m)
{
	struct lfo *this = (struct lfo *)m->priv;

	/* calculate samples as q8.24 */
	int32_t sample = 0;

	switch (this->shape) {
	case LFO_SHAPE_TRIANGLE: {
		uint32_t x = this->x + (1 << 30);
		sample = (int32_t)(x >> 6);
		sample ^= -(int32_t)(x >> 31);
		sample &= (1 << 25) - 1;
		sample -= (1 << 24);
		break;
	}
	case LFO_SHAPE_SAWDOWN: {
		sample = -(int32_t)(this->x) >> 7;
		break;
	}
	case LFO_SHAPE_SAWUP: {
		sample = (int32_t)(this->x) >> 7;
		break;
	}
	case LFO_SHAPE_SQUARE: {
		sample = (int32_t)(this->x & (1 << 31));
		sample = (sample >> 6) | (1 << 24);
		break;
	}
	case LFO_SHAPE_SINE: {
		uint32_t x = this->x - (1 << 30);
		return cos_lookup(x);
	}
	case LFO_SHAPE_SAMPLEANDHOLD: {
		if (this->x < this->xstep) {
			/* 0..253, cycle length = 128, 64 values with bit 7 = 1 */
			this->rand_state = ((this->rand_state * 179) + 17) & 0xff;
		}
		sample = (int32_t)(this->rand_state << 24) >> 7;
		break;
	}
	}

	/* convert q8.24 to float */
	return (float)sample / (float)(1 << 24);
}

static bool lfo_process(struct module *m, float *bufs[])
{
	struct lfo *this = (struct lfo *)m->priv;
	float *out = bufs[0];

	for (int i = 0; i < AudioBufferSize; i++) {
		this->x += this->xstep;
		out[i] = this->depth * lfo_sample(m);
	}

	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "rate", .type = PORT_TYPE_FLOAT, .pf = lfo_port_rate },
	{ .name = "depth", .type = PORT_TYPE_FLOAT, .pf = lfo_port_depth },
	{ .name = "shape", .type = PORT_TYPE_INT, .pf = lfo_port_shape },
	{ .name = "sync", .type = PORT_TYPE_BOOL, .pf = lfo_port_sync },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "out", .type = PORT_TYPE_AUDIO, },
	PORT_EOL,
};

const struct module_info osc_lfo_module = {
	.mname = "osc/lfo",
	.iname = "lfo",
	.in = in_ports,
	.out = out_ports,
	.alloc = lfo_alloc,
	.free = lfo_free,
	.process = lfo_process,
};

MODULE_REGISTER(osc_lfo_module);

/*****************************************************************************/
