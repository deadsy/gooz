/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "ggm.h"

/******************************************************************************
 * midi_to_frequency converts a MIDI note to a frequency value (Hz).
 * The note value is a float for pitch bending, tuning, etc.
 */

float midi_to_frequency(float note) {
	return 440.f * pow2((note - 69.f) * (1.f / 12.f));
}

/******************************************************************************
 * midi_pitch_bend maps a pitch bend value onto a MIDI note offset.
 */
float midi_pitch_bend(uint16_t val) {
	/* 0..8192..16383 maps to -/+ 2 semitones */
	return ((float)val - 8192.f) * (2.f / 8192.f);
}

/******************************************************************************
 * MIDI note names
 */

#if 0

const notesInOctave = 12 var sharpNotes =[notesInOctave] string {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
}

func(n MidiNote) sharpString()string {
	return sharpNotes[n % notesInOctave]
}

var flatNotes =[notesInOctave] string {
	"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B",
}

func(n MidiNote) flatString()string {
	return flatNotes[n % notesInOctave]
}

// Octave returns the MIDI octave of the MIDI note.
func(n MidiNote) Octave()
int {
	return int (n) / notesInOctave
} func(n MidiNote) String()string {
	return fmt.Sprintf("%s%d", n.sharpString(), n.Octave())
}

#endif

/******************************************************************************
 * midi_str returns a descriptive string for a MIDI event
 */

static const char *midi_msg_channel[] = {
	"?(00)",		/* 0x00 */
	"?(10)",		/* 0x10 */
	"?(20)",		/* 0x20 */
	"?(30)",		/* 0x30 */
	"?(40)",		/* 0x40 */
	"?(50)",		/* 0x50 */
	"?(60)",		/* 0x60 */
	"?(70)",		/* 0x70 */
	"note off",		/* 0x80 */
	"note on",		/* 0x90 */
	"polyphonic aftertouch",	/* 0xa0 */
	"control change",	/* 0xb0 */
	"program change",	/* 0xc0 */
	"channel aftertouch",	/* 0xd0 */
	"pitch wheel",		/* 0xe0 */
	NULL,			/* 0xf0 */
};

static const char *midi_msg_system[] = {
	"sysex start",		/* 0xf0 */
	"quarter frame",	/* 0xf1 */
	"song pointer",		/* 0xf2 */
	"song select",		/* 0xf3 */
	"?(f4)",		/* 0xf4 */
	"?(f5)",		/* 0xf5 */
	"tune request",		/* 0xf6 */
	"sysex end",		/* 0xf7 */
	"timing clock",		/* 0xf8 */
	"?(f9)",		/* 0xf9 */
	"start",		/* 0xfa */
	"continue",		/* 0xfb */
	"stop",			/* 0xfc */
	"?(fd)",		/* 0xfd */
	"active sensing",	/* 0xfe */
	"reset",		/* 0xff */
};

char *midi_str(char *s, size_t n, const struct event *e) {
	if (e->type != EVENT_TYPE_MIDI) {
		return NULL;
	}

	uint8_t status = e->u.midi.status;
	const char *msg;
	if ((status & 0xf0) == 0xf0) {
		msg = midi_msg_system[status & 15];
	} else {
		msg = midi_msg_channel[status >> 4];
	}

	switch (status & 0xf0) {
	case MIDI_STATUS_NOTEOFF:
	case MIDI_STATUS_NOTEON:{
			uint8_t ch = event_get_midi_channel(e);
			uint8_t note = event_get_midi_note(e);
			uint8_t vel = event_get_midi_velocity_int(e);
			snprintf(s, n, "%s ch %d note %d vel %d", msg, ch, note, vel);
			return s;
		}

	case MIDI_STATUS_CONTROLCHANGE:{
			uint8_t ch = event_get_midi_channel(e);
			uint8_t ctrl = event_get_midi_cc_num(e);
			uint8_t val = event_get_midi_cc_int(e);
			snprintf(s, n, "%s ch %d ctrl %d val %d", msg, ch, ctrl, val);
			return s;
		}

	case MIDI_STATUS_PITCHWHEEL:{
			uint8_t ch = event_get_midi_channel(e);
			uint16_t val = event_get_midi_pitch_wheel(e);
			snprintf(s, n, "%s ch %d val %d", msg, ch, val);
			return s;
		}

	case MIDI_STATUS_PROGRAMCHANGE:{
			uint8_t ch = event_get_midi_channel(e);
			uint16_t prog = event_get_midi_program(e);
			snprintf(s, n, "%s ch %d prog %d", msg, ch, prog);
			return s;
		}

	case MIDI_STATUS_CHANNELAFTERTOUCH:{
			uint8_t ch = event_get_midi_channel(e);
			uint8_t pressure = event_get_midi_pressure(e);
			snprintf(s, n, "%s ch %d pressure %d", msg, ch, pressure);
			return s;
		}

	case MIDI_STATUS_POLYPHONICAFTERTOUCH:{
			uint8_t ch = event_get_midi_channel(e);
			uint8_t note = event_get_midi_note(e);
			uint8_t pressure = event_get_midi_velocity_int(e);
			snprintf(s, n, "%s ch %d note %d pressure %d", msg, ch, note, pressure);
			return s;
		}
	}

	snprintf(s, n, "%s status %d arg0 %d arg1 %d", msg, e->u.midi.status, e->u.midi.arg0, e->u.midi.arg1);
	return s;
}

/*****************************************************************************/
