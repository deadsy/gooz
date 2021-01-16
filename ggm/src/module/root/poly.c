/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"
#include "osc/osc.h"

/******************************************************************************
 * MIDI setup
 */

#define MIDI_CH 0

#define SYNTH_SIMPLE_GOOM

/******************************************************************************
 * polyphonic synth with envelope on goom wave oscillator
 */

#if defined(SYNTH_SIMPLE_GOOM)

static const struct synth_cfg cfg[] = {
	{ "root.poly.voice*.adsr:attack",
	  &(struct port_float_cfg){ .init = 0.2f, .id = MIDI_ID(MIDI_CH, 1), }, },
	{ "root.poly.voice*.adsr:decay",
	  &(struct port_float_cfg){ .init = 0.1f, .id = MIDI_ID(MIDI_CH, 2), }, },
	{ "root.poly.voice*.adsr:sustain",
	  &(struct port_float_cfg){ .init = 0.3f, .id = MIDI_ID(MIDI_CH, 3), }, },
	{ "root.poly.voice*.adsr:release",
	  &(struct port_float_cfg){ .init = 0.3f, .id = MIDI_ID(MIDI_CH, 4), }, },
	{ "root.poly.voice*.goom:duty",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 5), }, },
	{ "root.poly.voice*.goom:slope",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 6), }, },
	{ "root.pan:pan",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 7), }, },
	{ "root.pan:vol",
	  &(struct port_float_cfg){ .init = 0.8f, .id = MIDI_ID(MIDI_CH, 8), }, },
	SYNTH_CFG_EOL
};

static struct module *voice_osc(struct module *m, int id)
{
	return module_new(m, "osc/goom", id);
}

static struct module *poly_voice(struct module *m, int id)
{
	return module_new(m, "voice/osc", id, voice_osc);
}

/******************************************************************************
 * polyphonic synth with envelope on sine wave oscillator
 */

#elif defined(SYNTH_SIMPLE_SINE)

static const struct synth_cfg cfg[] = {
	{ "root.poly.voice*.adsr:attack",
	  &(struct port_float_cfg){ .init = 0.2f, .id = MIDI_ID(MIDI_CH, 1), }, },
	{ "root.poly.voice*.adsr:decay",
	  &(struct port_float_cfg){ .init = 0.1f, .id = MIDI_ID(MIDI_CH, 2), }, },
	{ "root.poly.voice*.adsr:sustain",
	  &(struct port_float_cfg){ .init = 0.3f, .id = MIDI_ID(MIDI_CH, 3), }, },
	{ "root.poly.voice*.adsr:release",
	  &(struct port_float_cfg){ .init = 0.3f, .id = MIDI_ID(MIDI_CH, 4), }, },
	{ "root.pan:pan",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 7), }, },
	{ "root.pan:vol",
	  &(struct port_float_cfg){ .init = 0.8f, .id = MIDI_ID(MIDI_CH, 8), }, },
	SYNTH_CFG_EOL
};

static struct module *voice_osc(struct module *m, int id)
{
	return module_new(m, "osc/sine", id);
}

static struct module *poly_voice(struct module *m, int id)
{
	return module_new(m, "voice/osc", id, voice_osc);
}

/******************************************************************************
 * polyphonic synth with karplus-strong voices
 */

#elif defined(SYNTH_KS)

static const struct synth_cfg cfg[] = {
	{ "root.poly.ks*:attenuation",
	  &(struct port_float_cfg){ .init = 1.f, .id = MIDI_ID(MIDI_CH, 1), }, },
	{ "root.pan:pan",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 7), }, },
	{ "root.pan:vol",
	  &(struct port_float_cfg){ .init = 0.8f, .id = MIDI_ID(MIDI_CH, 8), }, },
	SYNTH_CFG_EOL
};

static struct module *poly_voice(struct module *m, int id)
{
	return module_new(m, "osc/ks", id);
}

#else
#error "please define a synth configuration"
#endif

/******************************************************************************
 * private state
 */

struct poly {
	struct module *poly;    /* polyphonic control */
	struct module *pan;     /* ouput left/right panning */
};

/******************************************************************************
 * module port functions
 */

static void poly_port_midi(struct module *m, const struct event *e)
{
	struct poly *this = (struct poly *)m->priv;
	bool consumed = synth_midi_cc(m->top, e);

	/* did the synth level midi cc map dispatch the event? */
	if (consumed) {
		return;
	}

	char tmp[64];
	LOG_DBG("%s", log_strdup(midi_str(tmp, sizeof(tmp), e)));
	/* forward the MIDI events */
	event_in(this->poly, "midi", e, NULL);
}

/******************************************************************************
 * module functions
 */

static int poly_alloc(struct module *m, va_list vargs)
{
	struct module *poly = NULL;
	struct module *pan = NULL;

	/* allocate the private data */
	struct poly *this = ggm_calloc(1, sizeof(struct poly));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set the synth configuration */
	int err = synth_set_cfg(m->top, cfg);
	if (err < 0) {
		goto error;
	}

	/* polyphony */
	poly = module_new(m, "midi/poly", -1, MIDI_CH, poly_voice);
	if (poly == NULL) {
		goto error;
	}
	this->poly = poly;

	/* pan */
	pan = module_new(m, "mix/pan", -1);
	if (poly == NULL) {
		goto error;
	}
	this->pan = pan;

	return 0;

error:
	module_del(poly);
	module_del(pan);
	ggm_free(this);
	return -1;
}

static void poly_free(struct module *m)
{
	struct poly *this = (struct poly *)m->priv;

	module_del(this->poly);
	module_del(this->pan);
	ggm_free(this);
}

static bool poly_process(struct module *m, float *bufs[])
{
	struct poly *this = (struct poly *)m->priv;
	struct module *poly = this->poly;
	struct module *pan = this->pan;
	float *out0 = bufs[0];
	float *out1 = bufs[1];
	float tmp[AudioBufferSize];

	poly->info->process(poly, (float *[]){ tmp, });
	pan->info->process(pan, (float *[]){ tmp, out0, out1, });
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "midi", .type = PORT_TYPE_MIDI, .pf = poly_port_midi },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "out0", .type = PORT_TYPE_AUDIO },
	{ .name = "out1", .type = PORT_TYPE_AUDIO },
	PORT_EOL,
};

const struct module_info root_poly_module = {
	.mname = "root/poly",
	.iname = "root",
	.in = in_ports,
	.out = out_ports,
	.alloc = poly_alloc,
	.free = poly_free,
	.process = poly_process,
};

MODULE_REGISTER(root_poly_module);

/*****************************************************************************/
