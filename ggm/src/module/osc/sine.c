/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct sine {
	float freq;		/* base frequency */
	uint32_t x;		/* current x-value */
	uint32_t xstep;		/* current x-step */
};

/******************************************************************************
 * sine functions
 */

static void sine_set_frequency(struct module *m, float freq) {
	struct sine *this = (struct sine *)m->priv;

	LOG_DBG("%s set frequency %f Hz", m->name, freq);
	this->freq = freq;
	this->xstep = (uint32_t) (freq * FrequencyScale);
}

/******************************************************************************
 * module port functions
 */

/* sine_port_reset resets the phase of the oscillator */
static void sine_port_reset(struct module *m, const struct event *e) {
	bool reset = event_get_bool(e);

	if (reset) {
		struct sine *this = (struct sine *)m->priv;
		LOG_DBG("%s phase reset", m->name);
		/* start at a phase that gives a zero output */
		this->x = QuarterCycle;
	}
}

/* sine_port_frequency sets the frequency of the oscillator */
static void sine_port_frequency(struct module *m, const struct event *e) {
	float freq = clampf_lo(event_get_float(e), 0);

	sine_set_frequency(m, freq);
}

/* sine_port_note is the pitch bent MIDI note (float) used to set frequency */
static void sine_port_note(struct module *m, const struct event *e) {
	float freq = midi_to_frequency(event_get_float(e));

	sine_set_frequency(m, freq);
}

/******************************************************************************
 * module functions
 */

static int sine_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct sine *this = ggm_calloc(1, sizeof(struct sine));

	if (this == NULL) {
		return -1;
	}

	m->priv = (void *)this;

	/* start at a phase that gives a zero output */
	this->x = QuarterCycle;
	return 0;
}

static void sine_free(struct module *m) {
	ggm_free(m->priv);
}

static bool sine_process(struct module *m, float *buf[]) {
	struct sine *this = (struct sine *)m->priv;
	float *out = buf[0];

	for (int i = 0; i < AudioBufferSize; i++) {
		out[i] = cos_lookup(this->x);
		this->x += this->xstep;
		// fm: this->x += (uint32_t)((this->freq + fm[i]) * FrequencyScale);
		// pm: this->x += (uint32_t)((float)this->xstep + (pm[i] * PhaseScale));
	}
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "reset",.type = PORT_TYPE_BOOL,.pf = sine_port_reset},
	{.name = "frequency",.type = PORT_TYPE_FLOAT,.pf = sine_port_frequency},
	{.name = "note",.type = PORT_TYPE_FLOAT,.pf = sine_port_note},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info osc_sine_module = {
	.mname = "osc/sine",
	.iname = "sine",
	.in = in_ports,
	.out = out_ports,
	.alloc = sine_alloc,
	.free = sine_free,
	.process = sine_process,
};

MODULE_REGISTER(osc_sine_module);

/*****************************************************************************/
