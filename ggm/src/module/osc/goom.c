/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Goom Waves
 *
 * A Goom Wave is a wave shape with the following segments:
 *
 * 1) s0: A falling (1 to -1) sine curve
 * 2) f0: A flat piece at the bottom
 * 3) s1: A rising (-1 to 1) sine curve
 * 4) f1: A flat piece at the top
 *
 * Shape is controlled by two parameters:
 * duty = split the total period between s0,f0 and s1,f1
 * slope = split s0f0 and s1f1 between slope and flat.
 *
 * The idea for goom waves comes from: https://www.quinapalus.com/goom.html
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct goom {
	float freq;             /* base frequency */
	float duty;             /* wave duty cycle */
	float slope;            /* wave slope */
	float tp;               /* s0f0 to s1f1 transition point */
	float k0;               /* scaling factor for slope 0 */
	float k1;               /* scaling factor for slope 1 */
	uint32_t x;             /* phase position */
	uint32_t xstep;         /* phase step per sample */
	uint32_t xreset;        /* phase value for zero output */
};

/******************************************************************************
 * goom functions
 */

/* goom_sample returns a single sample for the current phase value. */
static float goom_sample(struct module *m)
{
	struct goom *this = (struct goom *)m->priv;
	uint32_t ofs = 0;
	float x;

	/* what portion of the goom wave are we in? */
	if (this->x < this->tp) {
		/* we are in the s0/f0 portion */
		x = (float)(this->x) * this->k0;
	} else {
		/* we are in the s1/f1 portion */
		x = (float)(this->x - this->tp) * this->k1;
		ofs = HalfCycle;
	}
	// clamp x to 1
	if (x > 1.f) {
		x = 1.f;
	}
	return cos_lookup((uint32_t)(x * (float)HalfCycle) + ofs);
}

static void goom_set_shape(struct module *m, float duty, float slope)
{
	struct goom *this = (struct goom *)m->priv;

	/* update duty cycle */
	this->duty = duty;
	this->tp = (uint32_t)((float)FullCycle * map_lin(duty, 0.05f, 0.5f));
	/* update the slope */
	this->slope = slope;
	/* Work out the portion of s0f0/s1f1 that is sloped. */
	slope = map_lin(slope, 0.1f, 1.f);
	/* scaling constant for s0, map the slope to the LUT. */
	this->k0 = 1.f / ((float)(this->tp) * slope);
	/* scaling constant for s1, map the slope to the LUT. */
	this->k1 = 1.f / ((float)(FullCycle - 1 - this->tp) * slope);
	/* this phase reset value gives zero output */
	this->xreset = (uint32_t)((float)(this->tp) * slope * 0.5f);
}

static void goom_set_frequency(struct module *m, float freq)
{
	struct goom *this = (struct goom *)m->priv;

	this->freq = freq;
	this->xstep = (uint32_t)(freq * FrequencyScale);
}

/******************************************************************************
 * MIDI to port event conversion functions
 */

static void goom_midi_duty(struct event *dst, const struct event *src)
{
	/* 0..1 */
	event_set_float(dst, event_get_midi_cc_float(src));
}

static void goom_midi_slope(struct event *dst, const struct event *src)
{
	/* 0..1 */
	event_set_float(dst, event_get_midi_cc_float(src));
}

/******************************************************************************
 * module port functions
 */

/* goom_port_frequency sets the oscillator frequency */
static void goom_port_frequency(struct module *m, const struct event *e)
{
	float freq = clampf_lo(event_get_float(e), 0.f);

	LOG_DBG("%s:frequency %f Hz", m->name, freq);
	goom_set_frequency(m, freq);
}

/* goom_port_note is the pitch bent MIDI note (float) used to set frequency */
static void goom_port_note(struct module *m, const struct event *e)
{
	float note = event_get_float(e);

	LOG_DBG("%s:note %f", m->name, note);
	goom_set_frequency(m, midi_to_frequency(note));
}

/* goom_port_duty sets the wave duty cycle */
static void goom_port_duty(struct module *m, const struct event *e)
{
	struct goom *this = (struct goom *)m->priv;
	float duty = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("%s:duty %f", m->name, duty);
	goom_set_shape(m, duty, this->slope);
}

/* goom_port_slope sets the wave slope */
static void goom_port_slope(struct module *m, const struct event *e)
{
	struct goom *this = (struct goom *)m->priv;
	float slope = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("%s:slope %f", m->name, slope);
	goom_set_shape(m, this->duty, slope);
}

/* goom_port_reset resets the phase of the oscillator */
static void goom_port_reset(struct module *m, const struct event *e)
{
	bool reset = event_get_bool(e);

	if (reset) {
		struct goom *this = (struct goom *)m->priv;
		LOG_DBG("%s:reset phase", m->name);
		/* start at a phase that gives a zero output */
		this->x = this->xreset;
	}
}

/******************************************************************************
 * module functions
 */

static int goom_alloc(struct module *m, va_list vargs)
{
	/* allocate the private data */
	struct goom *this = ggm_calloc(1, sizeof(struct goom));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set initial shape values */
	goom_set_shape(m, 0.5f, 0.5f);
	/* start at a phase that gives a zero output */
	this->x = this->xreset;
	return 0;
}

static void goom_free(struct module *m)
{
	struct goom *this = (struct goom *)m->priv;

	ggm_free(this);
}

static bool goom_process(struct module *m, float *bufs[])
{
	struct goom *this = (struct goom *)m->priv;
	float *out = bufs[0];

	for (int i = 0; i < AudioBufferSize; i++) {
		out[i] = goom_sample(m);
		/* step the phase */
		this->x += this->xstep;
		// fm: m.x += uint32((m.freq + fm[i]) * core.FrequencyScale)
		// pm: m.x += uint32(float32(m.xstep) + (pm[i] * core.PhaseScale))
	}
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "frequency", .type = PORT_TYPE_FLOAT, .pf = goom_port_frequency },
	{ .name = "note", .type = PORT_TYPE_FLOAT, .pf = goom_port_note },
	{ .name = "duty", .type = PORT_TYPE_FLOAT, .pf = goom_port_duty, .mf = goom_midi_duty },
	{ .name = "slope", .type = PORT_TYPE_FLOAT, .pf = goom_port_slope, .mf = goom_midi_slope },
	{ .name = "reset", .type = PORT_TYPE_BOOL, .pf = goom_port_reset },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "out", .type = PORT_TYPE_AUDIO, },
	PORT_EOL,
};

const struct module_info osc_goom_module = {
	.mname = "osc/goom",
	.iname = "goom",
	.in = in_ports,
	.out = out_ports,
	.alloc = goom_alloc,
	.free = goom_free,
	.process = goom_process,
};

MODULE_REGISTER(osc_goom_module);

/*****************************************************************************/
