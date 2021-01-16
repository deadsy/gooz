/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct goom {
	struct module *amp_env; /* amplitude adsr envelope */
	struct module *lpf_env; /* low pass filter adsr envelope */
	struct module *osc;     /* goom oscillator */
	struct module *lpf;     /* low pass filter */
	float vel;              /* note velocity */
};

/******************************************************************************
 * module port functions
 */

/* goom_port_reset resets the voice state */
static void goom_port_reset(struct module *m, const struct event *e)
{
	struct goom *this = (struct goom *)m->priv;

	/* forward the reset to the sub-modules */
	event_in(this->amp_env, "reset", e, NULL);
	event_in(this->osc, "reset", e, NULL);
}

/* goom_port_gate is the voice gate event */
static void goom_port_gate(struct module *m, const struct event *e)
{
	struct goom *this = (struct goom *)m->priv;

	/* gate the envelopes */
	event_in(this->amp_env, "gate", e, NULL);
	event_in(this->lpf_env, "gate", e, NULL);
	/* record the velocity */
	this->vel = event_get_float(e);
}

/* goom_port_note is the pitch bent MIDI note (float) used to set the voice frequency */
static void goom_port_note(struct module *m, const struct event *e)
{
	struct goom *this = (struct goom *)m->priv;

	/* set the oscillator note */
	event_in(this->osc, "note", e, NULL);
}

/******************************************************************************
 * module functions
 */

static int goom_alloc(struct module *m, va_list vargs)
{
	struct module *amp_env = NULL;
	struct module *lpf_env = NULL;
	struct module *osc = NULL;
	struct module *lpf = NULL;

	/* allocate the private data */
	struct goom *this = ggm_calloc(1, sizeof(struct goom));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* amplitude adsr envelope */
	amp_env = module_new(m, "env/adsr", -1);
	if (amp_env == NULL) {
		goto error;
	}
	this->amp_env = amp_env;

	/* low pass filter adsr envelope */
	lpf_env = module_new(m, "env/adsr", -1);;
	if (lpf_env == NULL) {
		goto error;
	}
	this->lpf_env = lpf_env;

	/* goom oscillator */
	osc = module_new(m, "osc/goom", -1);;
	if (osc == NULL) {
		goto error;
	}
	this->osc = osc;

	/* low pass filter */
	lpf = module_new(m, "filter/svf", -1);;
	if (lpf == NULL) {
		goto error;
	}
	this->lpf = lpf;

	return 0;

error:
	module_del(amp_env);
	module_del(lpf_env);
	module_del(osc);
	module_del(lpf);
	ggm_free(m->priv);
	return -1;
}

static void goom_free(struct module *m)
{
	struct goom *this = (struct goom *)m->priv;

	module_del(this->amp_env);
	module_del(this->lpf_env);
	module_del(this->osc);
	module_del(this->lpf);
	ggm_free(this);
}

static bool goom_process(struct module *m, float *bufs[])
{
	struct goom *this = (struct goom *)m->priv;
	struct module *amp_env = this->amp_env;
	float env[AudioBufferSize];
	bool active = amp_env->info->process(amp_env, (float *[]){ env, });

	if (active) {
		// struct module *lpf_env = this->lpf_env;
		struct module *osc = this->osc;
		struct module *lpf = this->lpf;
		float *out = bufs[0];

		float buf[AudioBufferSize];

		// get the oscillator output
		osc->info->process(osc, (float *[]){ buf, });

		// feed it to the LPF
		lpf->info->process(lpf, (float *[]){ buf, out, });

		// apply the amplitude envelope
		block_mul(out, env);


	}

	return active;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "reset", .type = PORT_TYPE_BOOL, .pf = goom_port_reset },
	{ .name = "gate", .type = PORT_TYPE_FLOAT, .pf = goom_port_gate },
	{ .name = "note", .type = PORT_TYPE_FLOAT, .pf = goom_port_note },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "out", .type = PORT_TYPE_AUDIO, },
	PORT_EOL,
};

const struct module_info voice_goom_module = {
	.mname = "voice/goom",
	.iname = "goom",
	.in = in_ports,
	.out = out_ports,
	.alloc = goom_alloc,
	.free = goom_free,
	.process = goom_process,
};

MODULE_REGISTER(voice_goom_module);

/*****************************************************************************/
