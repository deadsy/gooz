/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Karplus Strong Oscillator Module
 *
 * KS generally has a delay line buffer size that determines the fundamental frequency
 * of the sound. That has some practical problems. The delay line buffer is too
 * large for low frequencies and it makes it hard to provide fine resolution
 * control over the frequency. This implementation uses a fixed buffer size and
 * steps through it with a 32 bit phase value. The step size determines the
 * frequency of the sound. When the step position falls between samples we do
 * linear interpolation to get the output value. When we move beyond a sample
 * we do the low pass filtering on it (in this case simple averaging).
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

enum ks_state {
	KS_STATE_IDLE = 0,	/* initial state */
	KS_STATE_PLUCKED,	/* string is plucked and attenuating normally */
	KS_STATE_RELEASE,	/* string is released and being moderately attenuated */
	KS_STATE_RESET,		/* string is agressively attenuated */
	KS_STATE_MAX		/* must be last */
};

#define KS_DELAY_BITS 7
#define KS_DELAY_SIZE (1 << KS_DELAY_BITS)

#define KS_DELAY_MASK (KS_DELAY_SIZE - 1)
#define KS_FRAC_BITS (32 - KS_DELAY_BITS)
#define KS_FRAC_MASK ((1 << KS_FRAC_BITS) - 1)
#define KS_FRAC_SCALE (1.f / (float)(1 << KS_FRAC_BITS))

struct ks {
	int state;		/* string state */
	uint32_t rand;		/* random state */
	float delay[KS_DELAY_SIZE];	/* delay line */
	float kval[KS_STATE_MAX];	/* attenuation per string state */
	float freq;		/* base frequency */
	uint32_t x;		/* phase position */
	uint32_t xstep;		/* phase step per sample */
};

/******************************************************************************
 * ks functions
 */

/* ks_set_frequency sets the string frequency */
static void ks_set_frequency(struct module *m, float freq) {
	struct ks *this = (struct ks *)m->priv;

	LOG_DBG("%s frequency %f", m->name, freq);
	this->freq = freq;
	this->xstep = (uint32_t) (freq * FrequencyScale);
}

/* ks_pluck_buffer initialises the delay buffer with random samples
 * between -1 and 1. The values should sum to zero so multiple rounds
 * of filtering will make all values fall to zero.
 */
static void ks_pluck_buffer(struct module *m, float gate) {
	struct ks *this = (struct ks *)m->priv;
	float sum = 0.f;

	/* map the gate value */
	gate = clampf(gate, 0.f, 1.f);
	gate = map_exp(gate, 0.f, 1.f, -4);

	for (int i = 0; i < KS_DELAY_SIZE - 1; i++) {
		float val = gate * randf(&this->rand);
		float x = sum + val;
		if ((x > 1.f) || (x < -1.f)) {
			val = -val;
		}
		sum += val;
		this->delay[i] = val;
	}
	this->delay[KS_DELAY_SIZE - 1] = -sum;
}

/* ks_zero_buffer resets the delay buffer */
static void ks_zero_buffer(struct module *m) {
	struct ks *this = (struct ks *)m->priv;

	for (int i = 0; i < KS_DELAY_SIZE - 1; i++) {
		this->delay[i] = 0.f;
	}
}

/******************************************************************************
 * MIDI to port event conversion functions
 */

static void ks_midi_attenuation(struct event *dst, const struct event *src) {
	/* 0.75 to 1.0 */
	float x = event_get_midi_cc_float(src);

	x = map_lin(x, 0.75f, 1.f);
	event_set_float(dst, x);
}

/******************************************************************************
 * module port functions
 */

static void ks_port_reset(struct module *m, const struct event *e) {
	struct ks *this = (struct ks *)m->priv;
	bool reset = event_get_bool(e);

	if (reset) {
		LOG_DBG("%s hard reset", m->name);
		ks_zero_buffer(m);
		this->state = KS_STATE_IDLE;
	} else {
		LOG_DBG("%s soft reset", m->name);
		this->state = KS_STATE_RESET;
	}
}

static void ks_port_gate(struct module *m, const struct event *e) {
	struct ks *this = (struct ks *)m->priv;
	float gate = event_get_float(e);

	LOG_DBG("%s gate %f", m->name, gate);

	if (gate > 0) {
		ks_pluck_buffer(m, gate);
		this->state = KS_STATE_PLUCKED;
	} else {
		this->state = KS_STATE_RELEASE;
	}
}

static void ks_port_attenuation(struct module *m, const struct event *e) {
	struct ks *this = (struct ks *)m->priv;
	float attenuation = clampf(event_get_float(e), 0.f, 1.f);

	LOG_DBG("%s attenuation %f", m->name, attenuation);
	this->kval[KS_STATE_PLUCKED] = 0.5 * attenuation;
}

static void ks_port_frequency(struct module *m, const struct event *e) {
	ks_set_frequency(m, clampf_lo(event_get_float(e), 0.f));
}

static void ks_port_note(struct module *m, const struct event *e) {
	ks_set_frequency(m, midi_to_frequency(event_get_float(e)));
}

/******************************************************************************
 * module functions
 */

static int ks_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct ks *this = ggm_calloc(1, sizeof(struct ks));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* initialise the random seed */
	rand_init(0, &this->rand);

	/* set default attenuation values */
	this->kval[KS_STATE_PLUCKED] = 0.5f;
	this->kval[KS_STATE_RELEASE] = 0.8f * 0.5f;
	this->kval[KS_STATE_RESET] = 0.1f * 0.1f * 0.5f;

	return 0;
}

static void ks_free(struct module *m) {
	struct ks *this = (struct ks *)m->priv;

	ggm_free(this);
}

static bool ks_process(struct module *m, float *bufs[]) {
	struct ks *this = (struct ks *)m->priv;
	float *out = bufs[0];

	if (this->state == KS_STATE_IDLE) {
		/* no output */
		return false;
	}

	for (int i = 0; i < AudioBufferSize; i++) {
		uint32_t x0 = this->x >> KS_FRAC_BITS;
		uint32_t x1 = (x0 + 1) & KS_DELAY_MASK;
		float y0 = this->delay[x0];
		float y1 = this->delay[x1];
		/* interpolate */
		out[i] = y0 + (y1 - y0) * KS_FRAC_SCALE * (float)(this->x & KS_FRAC_MASK);
		/* step the x position */
		this->x += this->xstep;
		/* filter: once we have moved beyond the delay line index we
		 * will average it's amplitude with the next value.
		 */
		if (x0 != (this->x >> KS_FRAC_BITS)) {
			float k = this->kval[this->state];
			this->delay[x0] = k * (y0 + y1);
		}
	}

	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "reset",.type = PORT_TYPE_BOOL,.pf = ks_port_reset},
	{.name = "gate",.type = PORT_TYPE_FLOAT,.pf = ks_port_gate},
	{.name = "note",.type = PORT_TYPE_FLOAT,.pf = ks_port_note},
	{.name = "frequency",.type = PORT_TYPE_FLOAT,.pf = ks_port_frequency},
	{.name = "attenuation",.type = PORT_TYPE_FLOAT,.pf = ks_port_attenuation,.mf = ks_midi_attenuation,},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info osc_ks_module = {
	.mname = "osc/ks",
	.iname = "ks",
	.in = in_ports,
	.out = out_ports,
	.alloc = ks_alloc,
	.free = ks_free,
	.process = ks_process,
};

MODULE_REGISTER(osc_ks_module);

/*****************************************************************************/
