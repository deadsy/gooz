/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_INC_EVENT_H
#define GGM_SRC_INC_EVENT_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * events
 */

enum event_type {
	EVENT_TYPE_NULL = 0,
	EVENT_TYPE_FLOAT,	/* float value event */
	EVENT_TYPE_INT,		/* integer value event */
	EVENT_TYPE_BOOL,	/* boolean value event */
	EVENT_TYPE_MIDI,	/* MIDI event */
};

struct event {
	enum event_type type;
	union {
		float fval;
		int ival;
		bool bval;
		struct {
			uint8_t status;
			uint8_t arg0;
			uint8_t arg1;
		} midi;
	} u;
};

/******************************************************************************
 * function prototypes
 */

typedef void (*port_func)(struct module * m, const struct event * e);
typedef void (*midi_func)(struct event * dst, const struct event * src);
typedef void (*midi_out_func)(void *arg, const struct event * e, int idx);

void event_in(struct module *m, const char *name, const struct event *e, port_func * hdl);
void event_out(struct module *m, int idx, const struct event *e);
void event_out_name(struct module *m, const char *name, const struct event *e);
void event_push(struct module *m, int idx, const struct event *e);
void event_push_name(struct module *m, const char *name, const struct event *e);

/******************************************************************************
 * MIDI events
 */

/* Channel Messages */
#define MIDI_STATUS_NOTEOFF (8 << 4)
#define MIDI_STATUS_NOTEON  (9 << 4)
#define MIDI_STATUS_POLYPHONICAFTERTOUCH  (10 << 4)
#define MIDI_STATUS_CONTROLCHANGE  (11 << 4)
#define MIDI_STATUS_PROGRAMCHANGE  (12 << 4)
#define MIDI_STATUS_CHANNELAFTERTOUCH  (13 << 4)
#define MIDI_STATUS_PITCHWHEEL  (14 << 4)

/* System Common Messages */
#define MIDI_STATUS_SYSEXSTART  0Xf0
#define MIDI_STATUS_QUARTERFRAME  0Xf1
#define MIDI_STATUS_SONGPOINTER  0Xf2
#define MIDI_STATUS_SONGSELECT  0Xf3
#define MIDI_STATUS_TUNEREQUEST  0Xf6
#define MIDI_STATUS_SYSEXEND  0Xf7

/* System Realtime Messages */
#define MIDI_STATUS_TIMINGCLOCK  0Xf8
#define MIDI_STATUS_START  0Xfa
#define MIDI_STATUS_CONTINUE  0Xfb
#define MIDI_STATUS_STOP  0Xfc
#define MIDI_STATUS_ACTIVESENSING  0Xfe
#define MIDI_STATUS_RESET  0Xff

/* Delimiters */
#define MIDI_STATUS_COMMON  0Xf0
#define MIDI_STATUS_REALTIME  0Xf8

/* event_set_midi_note formats a MIDI note on/off event */
static inline void event_set_midi_note(struct event *e, uint8_t msg, uint8_t chan, uint8_t note, uint8_t velocity) {
	e->type = EVENT_TYPE_MIDI;
	e->u.midi.status = msg | (chan & 15);
	e->u.midi.arg0 = note & 127;
	e->u.midi.arg1 = velocity & 127;
}

static inline void event_set_midi(struct event *e, uint8_t status, uint8_t arg0, uint8_t arg1) {
	e->type = EVENT_TYPE_MIDI;
	e->u.midi.status = status;
	e->u.midi.arg0 = arg0;
	e->u.midi.arg1 = arg1;
}

/* event_get_midi_channel returns the MIDI channel number */
static inline uint8_t event_get_midi_channel(const struct event *e) {
	return e->u.midi.status & 0xf;
}

/* event_get_midi_note returns the MIDI note number */
static inline uint8_t event_get_midi_note(const struct event *e) {
	return e->u.midi.arg0;
}

/* event_get_midi_cc_num returns the MIDI continuous controller number */
static inline uint8_t event_get_midi_cc_num(const struct event *e) {
	return e->u.midi.arg0;
}

/* event_get_midi_cc_int returns the MIDI continuous controller value (int) */
static inline uint8_t event_get_midi_cc_int(const struct event *e) {
	return e->u.midi.arg1;
}

/* event_get_midi_cc_float returns the MIDI continuous controller value (float) */
static inline float event_get_midi_cc_float(const struct event *e) {
	return (float)(e->u.midi.arg1 & 0x7f) * (1.f / 127.f);
}

/* event_get_midi_velocity_int returns the MIDI note velocity (int) */
static inline uint8_t event_get_midi_velocity_int(const struct event *e) {
	return e->u.midi.arg1;
}

/* event_get_midi_velocity_float returns the MIDI note velocity (float) */
static inline float event_get_midi_velocity_float(const struct event *e) {
	return (float)(e->u.midi.arg1 & 0x7f) * (1.f / 127.f);
}

/* event_get_midi_pitch_wheel returns the MIDI pitch wheel value */
static inline uint16_t event_get_midi_pitch_wheel(const struct event *e) {
	return (uint16_t) (e->u.midi.arg1 << 7) | (uint16_t) (e->u.midi.arg0);
}

/* event_get_midi_program returns the MIDI program number */
static inline uint8_t event_get_midi_program(const struct event *e) {
	return e->u.midi.arg0;
}

/* event_get_midi_pressure returns the MIDI pressure value */
static inline uint8_t event_get_midi_pressure(const struct event *e) {
	return e->u.midi.arg0;
}

/* event_get_midi_msg returns the MIDI message */
static inline uint8_t event_get_midi_msg(const struct event *e) {
	uint8_t status = e->u.midi.status;

	if ((status & 0xf0) == 0xf0) {
		return status;
	}
	return status & 0xf0;
}

static inline bool is_midi_cc(const struct event *e) {
	if (e->type != EVENT_TYPE_MIDI) {
		return false;
	}
	return (e->u.midi.status & 0xf0) == MIDI_STATUS_CONTROLCHANGE;
}

static inline bool is_midi_ch(const struct event *e, uint8_t ch) {
	if (e->type != EVENT_TYPE_MIDI) {
		return false;
	}
	return event_get_midi_channel(e) == ch;
}

char *midi_str(char *s, size_t n, const struct event *e);

/******************************************************************************
 * float events
 */

static inline void event_set_float(struct event *e, float x) {
	e->type = EVENT_TYPE_FLOAT;
	e->u.fval = x;
}

static inline float event_get_float(const struct event *e) {
	return e->u.fval;
}

static inline void event_in_float(struct module *m, const char *name, float val, port_func * hdl) {
	struct event e;

	event_set_float(&e, val);
	event_in(m, name, &e, hdl);
}

/******************************************************************************
 * integer events
 */

static inline void event_set_int(struct event *e, int x) {
	e->type = EVENT_TYPE_INT;
	e->u.ival = x;
}

static inline int event_get_int(const struct event *e) {
	return e->u.ival;
}

static inline void event_in_int(struct module *m, const char *name, int val, port_func * hdl) {
	struct event e;

	event_set_int(&e, val);
	event_in(m, name, &e, hdl);
}

/******************************************************************************
 * boolean events
 */

static inline void event_set_bool(struct event *e, bool x) {
	e->type = EVENT_TYPE_BOOL;
	e->u.bval = x;
}

static inline bool event_get_bool(const struct event *e) {
	return e->u.bval;
}

static inline void event_in_bool(struct module *m, const char *name, bool val, port_func * hdl) {
	struct event e;

	event_set_bool(&e, val);
	event_in(m, name, &e, hdl);
}

/*****************************************************************************/

#endif				/* GGM_SRC_INC_EVENT_H */

/*****************************************************************************/
