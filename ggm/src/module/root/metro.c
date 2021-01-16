/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This is the root patch for a metronome.
 */

#include "ggm.h"
#include "seq/seq.h"


/******************************************************************************
 * MIDI setup
 */

#define MIDI_CH 0

static const struct synth_cfg cfg[] = {
	{ "root.mono.voice.adsr:attack",
	  &(struct port_float_cfg){ .init = 0.1f, .id = MIDI_ID(MIDI_CH, 1), }, },
	{ "root.mono.voice.adsr:decay",
	  &(struct port_float_cfg){ .init = 0.5f, .id = MIDI_ID(MIDI_CH, 2), }, },
	{ "root.mono.voice.adsr:sustain",
	  &(struct port_float_cfg){ .init = 0.8f, .id = MIDI_ID(MIDI_CH, 3), }, },
	{ "root.mono.voice.adsr:release",
	  &(struct port_float_cfg){ .init = 1.f, .id = MIDI_ID(MIDI_CH, 4), }, },
	{ "root.seq:bpm",
	  &(struct port_float_cfg){ .init = 60.f, .id = MIDI_ID(MIDI_CH, 7), }, },
	{ "root.pan:vol",
	  &(struct port_float_cfg){ .init = 0.8f, .id = MIDI_ID(MIDI_CH, 8), }, },
	SYNTH_CFG_EOL
};

/******************************************************************************
 * patterns
 */

/* 4/4 signature */
static const uint8_t signature_4_4[] = {
	SEQ_OP_NOTE, MIDI_CH, 69, 100, 4,
	SEQ_OP_REST, 12,
	SEQ_OP_NOTE, MIDI_CH, 60, 100, 4,
	SEQ_OP_REST, 12,
	SEQ_OP_NOTE, MIDI_CH, 60, 100, 4,
	SEQ_OP_REST, 12,
	SEQ_OP_NOTE, MIDI_CH, 60, 100, 4,
	SEQ_OP_REST, 12,
	SEQ_OP_LOOP,
};

/******************************************************************************
 * private state
 */

struct metro {
	struct module *seq;     /* sequencer */
	struct module *mono;    /* monophonic voice for audio output */
	struct module *pan;     /* mix mono to stereo output */
};

static struct module *voice_osc0(struct module *m, int id)
{
	return module_new(m, "osc/sine", id);
}

static struct module *mono_voice0(struct module *m, int id)
{
	return module_new(m, "voice/osc", id, voice_osc0);
}

/******************************************************************************
 * module port functions
 */

static void metro_port_midi(struct module *m, const struct event *e)
{
	/* consume external cc events */
	synth_midi_cc(m->top, e);
}

/******************************************************************************
 * module functions
 */

static int metro_alloc(struct module *m, va_list vargs)
{
	struct module *seq = NULL;
	struct module *mono = NULL;
	struct module *pan = NULL;

	/* allocate the private data */
	struct metro *this = ggm_calloc(1, sizeof(struct metro));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set the synth configuration */
	int err = synth_set_cfg(m->top, cfg);
	if (err < 0) {
		goto error;
	}

	/* sequencer */
	seq = module_new(m, "seq/seq", -1, signature_4_4);
	if (seq == NULL) {
		goto error;
	}
	event_in_float(seq, "bpm", 120.f, NULL);
	event_in_int(seq, "ctrl", SEQ_CTRL_START, NULL);
	this->seq = seq;

	/* mono */
	mono = module_new(m, "midi/mono", -1, MIDI_CH, mono_voice0);
	if (mono == NULL) {
		goto error;
	}
	this->mono = mono;

	/* pan */
	pan = module_new(m, "mix/pan", -1);
	if (pan == NULL) {
		goto error;
	}
	this->pan = pan;

	/* forward the sequencer MIDI output to the MIDI output */
	port_forward(seq, "midi", m, "midi");

	/* connect the sequencer MIDI output to the mono MIDI input */
	port_connect(seq, "midi", mono, "midi");

	return 0;

error:
	module_del(seq);
	module_del(mono);
	module_del(pan);
	ggm_free(this);
	return -1;
}

static void metro_free(struct module *m)
{
	struct metro *this = (struct metro *)m->priv;

	module_del(this->seq);
	module_del(this->mono);
	module_del(this->pan);
	ggm_free(this);
}

static bool metro_process(struct module *m, float *bufs[])
{
	struct metro *this = (struct metro *)m->priv;
	struct module *seq = this->seq;
	struct module *mono = this->mono;
	float tmp[AudioBufferSize];

	seq->info->process(seq, NULL);

	bool active = mono->info->process(mono, (float *[]){ tmp, });
	if (active) {
		struct module *pan = this->pan;
		float *out0 = bufs[0];
		float *out1 = bufs[1];
		pan->info->process(pan, (float *[]){ tmp, out0, out1, });
	}

	return active;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "midi", .type = PORT_TYPE_MIDI, .pf = metro_port_midi },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "midi", .type = PORT_TYPE_MIDI, },
	{ .name = "out0", .type = PORT_TYPE_AUDIO },
	{ .name = "out1", .type = PORT_TYPE_AUDIO },
	PORT_EOL,
};

const struct module_info root_metro_module = {
	.mname = "root/metro",
	.iname = "root",
	.in = in_ports,
	.out = out_ports,
	.alloc = metro_alloc,
	.free = metro_free,
	.process = metro_process,
};

MODULE_REGISTER(root_metro_module);

/*****************************************************************************/
