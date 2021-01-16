/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Polyphonic Module
 * Manage concurrent instances (voices) of a given sub-module.
 * Note: The single channel output is the sum of outputs from each single channel voice.
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

#define MAX_POLYPHONY 5		/* N active + 1 in soft reset */

struct voice {
	struct module *m;	/* the voice module */
	uint8_t note;		/* the MIDI note for this voice */
	bool reset;		/* indicates a voice in soft reset mode */
};

struct poly {
	uint8_t ch;		/* MIDI channel we are using */
	struct voice voice[MAX_POLYPHONY];	/* voices */
	int idx;		/* round robin voice index */
	float bend;		/* pitch bend value for all voices */
};

/******************************************************************************
 * voice functions
 */

/* voice_lookup returns the voice module for this MIDI note (or NULL) */
static struct voice *voice_lookup(struct module *m, uint8_t note) {
	struct poly *this = (struct poly *)m->priv;

	for (int i = 0; i < MAX_POLYPHONY; i++) {
		struct voice *v = &this->voice[i];
		if (v->note == note && (v->reset == false)) {
			return &this->voice[i];
		}
	}
	return NULL;
}

/* voice_alloc allocates a new voice module for the MIDI note */
static struct voice *voice_alloc(struct module *m, uint8_t note) {
	struct poly *this = (struct poly *)m->priv;

	LOG_INF("allocate voice %d to note %d", this->idx, note);

	/* do round-robin voice allocation */
	struct voice *v = &this->voice[this->idx];
	this->idx += 1;
	if (this->idx == MAX_POLYPHONY) {
		this->idx = 0;
	}

	/* send a hard reset to the new voice */
	event_in_bool(v->m, "reset", true, NULL);

	/* set the voice note */
	event_in_float(v->m, "note", (float)note + this->bend, NULL);
	v->note = note;
	v->reset = false;

	/* Send a soft reset to the next voice so it will be idle
	 * when we need to use it.
	 */
	struct voice *next_v = &this->voice[this->idx];
	event_in_bool(next_v->m, "reset", false, NULL);
	next_v->reset = true;

	return v;
}

/******************************************************************************
 * module port functions
 */

static void poly_port_midi(struct module *m, const struct event *e) {
	struct poly *this = (struct poly *)m->priv;

	if (!is_midi_ch(e, this->ch)) {
		/* it's not for this channel */
		return;
	}

	switch (event_get_midi_msg(e)) {

	case MIDI_STATUS_NOTEON:{
			uint8_t note = event_get_midi_note(e);
			float vel = event_get_midi_velocity_float(e);
			struct voice *v = voice_lookup(m, note);
			if (v == NULL) {
				v = voice_alloc(m, note);
			}
			/* note: vel = 0 is the same as note off (gate=0) */
			event_in_float(v->m, "gate", vel, NULL);
			break;
		}

	case MIDI_STATUS_NOTEOFF:{
			struct voice *v = voice_lookup(m, event_get_midi_note(e));
			if (v != NULL) {
				/* send a note off control event, ignore the note off velocity (for now) */
				event_in_float(v->m, "gate", 0.f, NULL);
			}
			break;
		}

	case MIDI_STATUS_PITCHWHEEL:{
			/* get the pitch bend value */
			this->bend = midi_pitch_bend(event_get_midi_pitch_wheel(e));
			/* update all voices */
			for (int i = 0; i < MAX_POLYPHONY; i++) {
				struct voice *v = &this->voice[i];
				event_in_float(v->m, "note", (float)(v->note) + this->bend, NULL);
			}
			break;
		}

	default:{
			/* pass through the MIDI event to the voices */
			for (int i = 0; i < MAX_POLYPHONY; i++) {
				event_in(this->voice[i].m, "midi", e, NULL);
			}
			break;
		}

	}

}

/******************************************************************************
 * module functions
 */

static int poly_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct poly *this = ggm_calloc(1, sizeof(struct poly));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* get the MIDI channel */
	this->ch = va_arg(vargs, int);

	/* allocate the voices */
	module_func new_voice = va_arg(vargs, module_func);
	for (int i = 0; i < MAX_POLYPHONY; i++) {
		this->voice[i].m = new_voice(m, i);
		if (this->voice[i].m == NULL) {
			goto error;
		}
	}

	return 0;

 error:
	for (int i = 0; i < MAX_POLYPHONY; i++) {
		module_del(this->voice[i].m);
	}
	ggm_free(this);
	return -1;
}

static void poly_free(struct module *m) {
	struct poly *this = (struct poly *)m->priv;

	for (int i = 0; i < MAX_POLYPHONY; i++) {
		module_del(this->voice[i].m);
	}
	ggm_free(this);
}

static bool poly_process(struct module *m, float *bufs[]) {
	struct poly *this = (struct poly *)m->priv;
	float *out = bufs[0];
	bool active = false;

	// zero the output buffer
	block_zero(out);

	// run each voice
	for (int i = 0; i < MAX_POLYPHONY; i++) {
		struct module *vm = this->voice[i].m;
		float vbuf[AudioBufferSize];

		if (vm->info->process(vm, (float *[]) { vbuf, })) {
			block_add(out, vbuf);
			active = true;
		}
	}

	return active;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "midi",.type = PORT_TYPE_MIDI,.pf = poly_port_midi},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info midi_poly_module = {
	.mname = "midi/poly",
	.iname = "poly",
	.in = in_ports,
	.out = out_ports,
	.alloc = poly_alloc,
	.free = poly_free,
	.process = poly_process,
};

MODULE_REGISTER(midi_poly_module);

/*****************************************************************************/
