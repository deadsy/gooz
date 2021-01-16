/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Monophonic Module
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct mono {
	uint8_t ch;		/* MIDI channel we are using */
	uint8_t note;		/* the MIDI note for this voice */
	float bend;		/* pitch bend value */
	struct module *voice;	/* the voice module */
};

/******************************************************************************
 * module port functions
 */

static void mono_port_midi(struct module *m, const struct event *e) {
	struct mono *this = (struct mono *)m->priv;
	struct module *voice = this->voice;

	if (!is_midi_ch(e, this->ch)) {
		/* it's not for this channel */
		return;
	}

	switch (event_get_midi_msg(e)) {

	case MIDI_STATUS_NOTEON:{
			uint8_t note = event_get_midi_note(e);
			float vel = event_get_midi_velocity_float(e);
			if (note != this->note) {
				/* set the note */
				event_in_float(voice, "note", (float)note + this->bend, NULL);
				this->note = note;
			}
			/* note: vel = 0 is the same as note off (gate=0) */
			event_in_float(voice, "gate", vel, NULL);
			break;
		}

	case MIDI_STATUS_NOTEOFF:{
			/* send a note off control event, ignore the note off velocity (for now) */
			event_in_float(voice, "gate", 0.f, NULL);
			break;
		}

	case MIDI_STATUS_PITCHWHEEL:{
			/* get the pitch bend value */
			this->bend = midi_pitch_bend(event_get_midi_pitch_wheel(e));
			/* update the voice */
			event_in_float(voice, "note", (float)(this->note) + this->bend, NULL);
			break;
		}

	default:{
			/* pass through the MIDI event to the voice */
			event_in(voice, "midi", e, NULL);
			break;
		}

	}
}

/******************************************************************************
 * module functions
 */

static int mono_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct mono *this = ggm_calloc(1, sizeof(struct mono));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* get the MIDI channel */
	this->ch = va_arg(vargs, int);

	/* allocate the voice */
	module_func new_voice = va_arg(vargs, module_func);
	this->voice = new_voice(m, -1);
	if (this->voice == NULL) {
		goto error;
	}

	return 0;

 error:
	module_del(this->voice);
	ggm_free(this);
	return -1;
}

static void mono_free(struct module *m) {
	struct mono *this = (struct mono *)m->priv;

	module_del(this->voice);
	ggm_free(this);
}

static bool mono_process(struct module *m, float *bufs[]) {
	struct mono *this = (struct mono *)m->priv;
	struct module *voice = this->voice;
	float *out = bufs[0];

	return voice->info->process(voice, (float *[]) { out, });
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "midi",.type = PORT_TYPE_MIDI,.pf = mono_port_midi},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info midi_mono_module = {
	.mname = "midi/mono",
	.iname = "mono",
	.in = in_ports,
	.out = out_ports,
	.alloc = mono_alloc,
	.free = mono_free,
	.process = mono_process,
};

MODULE_REGISTER(midi_mono_module);

/*****************************************************************************/
