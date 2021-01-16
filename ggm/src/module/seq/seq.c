/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"
#include "seq/seq.h"

/******************************************************************************
 * private state
 */

enum {
	SEQ_STATE_STOP, /* initial state */
	SEQ_STATE_RUN,
};

enum {
	OP_STATE_INIT, /* initial state */
	OP_STATE_WAIT,
};

struct seq_sm {
	uint8_t *prog;          /* program operations */
	int pc;                 /* program counter */
	int seq_state;          /* sequencer state */
	int op_state;           /* operation state */
	int duration;           /* operation duration */
};

struct seq {
	float secs_per_tick;    /* seconds per tick */
	float tick_error;       /* current tick error*/
	uint32_t ticks;         /* full ticks*/
	struct seq_sm sm;       /* state machine */
};

/******************************************************************************
 * state machine operations
 */

struct note_args {
	uint8_t op;
	uint8_t chan;
	uint8_t note;
	uint8_t vel;
	uint8_t dur;
};

struct rest_args {
	uint8_t op;
	uint8_t dur;
};

/* op_nop is a no operation (op) */
static int op_nop(struct module *m)
{
	return 1;
}

/* op_loop returns to the beginning of the program (op) */
static int op_loop(struct module *m)
{
	struct seq *this = (struct seq *)m->priv;
	struct seq_sm *sm = &this->sm;

	sm->pc = -1;
	return 1;
}

/* op_note generates note on/off events (op, channel, note, velocity, duration) */
static int op_note(struct module *m)
{
	struct seq *this = (struct seq *)m->priv;
	struct seq_sm *sm = &this->sm;
	struct note_args *args = (struct note_args *)&sm->prog[sm->pc];

	if (sm->op_state == OP_STATE_INIT) {
		/* init */
		sm->duration = args->dur;
		sm->op_state = OP_STATE_WAIT;
		LOG_INF("note on %d (%d)", args->note, this->ticks);
		struct event e;
		event_set_midi_note(&e, MIDI_STATUS_NOTEON, args->chan, args->note, args->vel);
		event_push_name(m, "midi", &e);
	}
	sm->duration -= 1;
	if (sm->duration == 0) {
		/* done */
		sm->op_state = OP_STATE_INIT;
		LOG_INF("note off (%d)", this->ticks);
		struct event e;
		event_set_midi_note(&e, MIDI_STATUS_NOTEOFF, args->chan, args->note, 0);
		event_push_name(m, "midi", &e);
		return sizeof(struct note_args);
	}
	/* waiting... */
	return 0;
}

/* op_rest rests for a duration (op, duration) */
static int op_rest(struct module *m)
{
	struct seq *this = (struct seq *)m->priv;
	struct seq_sm *sm = &this->sm;
	struct rest_args *args = (struct rest_args *)&sm->prog[sm->pc];

	if (sm->op_state == OP_STATE_INIT) {
		/* init */
		sm->duration = args->dur;
		sm->op_state = OP_STATE_WAIT;
	}
	sm->duration -= 1;
	if (sm->duration == 0) {
		/* done */
		sm->op_state = OP_STATE_INIT;
		return sizeof(struct rest_args);
	}
	/* waiting... */
	return 0;
}

static int (*op_table[SEQ_OP_NUM]) (struct module *m) = {
	op_nop,                 /* SEQ_OP_NOP */
	op_loop,                /* SEQ_OP_LOOP */
	op_note,                /* SEQ_OP_NOTE */
	op_rest,                /* SEQ_OP_REST */
};

static void seq_tick(struct module *m)
{
	struct seq *this = (struct seq *)m->priv;
	struct seq_sm *sm = &this->sm;

	/* auto stop zero length programs */
	if (sm->prog == NULL) {
		sm->seq_state = SEQ_STATE_STOP;
	}
	/* run the program */
	if (sm->seq_state == SEQ_STATE_RUN) {
		int (*op) (struct module *m) = op_table[sm->prog[sm->pc]];
		sm->pc += op(m);
	}
}

/******************************************************************************
 * module port functions
 */

#define TICKS_PER_BEAT (16.0f)

static void seq_midi_bpm(struct event *dst, const struct event *src)
{
	event_set_float(dst, map_lin(event_get_midi_cc_float(src), MinBeatsPerMin, MaxBeatsPerMin));
}

static void seq_port_bpm(struct module *m, const struct event *e)
{
	struct seq *this = (struct seq *)m->priv;
	float bpm = clampf(event_get_float(e), MinBeatsPerMin, MaxBeatsPerMin);

	LOG_INF("%s:bpm %f", m->name, bpm);
	this->secs_per_tick = SecsPerMin / (bpm * TICKS_PER_BEAT);
}

static void seq_port_ctrl(struct module *m, const struct event *e)
{
	struct seq *this = (struct seq *)m->priv;
	struct seq_sm *sm = &this->sm;
	int ctrl = event_get_int(e);

	switch (ctrl) {
	case SEQ_CTRL_STOP: /* stop the sequencer */
		LOG_INF("%s:ctrl stop", m->name);
		sm->seq_state = SEQ_STATE_STOP;
		break;
	case SEQ_CTRL_START: /* start the sequencer */
		LOG_INF("%s:ctrl start", m->name);
		sm->seq_state = SEQ_STATE_RUN;
		break;
	case SEQ_CTRL_RESET: /* reset the sequencer */
		LOG_INF("%s:ctrl reset", m->name);
		sm->seq_state = SEQ_STATE_STOP;
		sm->op_state = OP_STATE_INIT;
		sm->pc = 0;
		break;
	default:
		LOG_INF("%s:ctrl unknown value %d", m->name, ctrl);
		break;
	}
}

/******************************************************************************
 * module functions
 */

static int seq_alloc(struct module *m, va_list vargs)
{
	/* allocate the private data */
	struct seq *this = ggm_calloc(1, sizeof(struct seq));

	if (this == NULL) {
		LOG_ERR("could not allocate private data");
		return -1;
	}
	m->priv = (void *)this;

	/* setup the sequencer program */
	this->sm.prog = va_arg(vargs, uint8_t *);

	return 0;
}

static void seq_free(struct module *m)
{
	ggm_free(m->priv);
}

static bool seq_process(struct module *m, float *buf[])
{
	struct seq *this = (struct seq *)m->priv;

	/* This routine is being used as a periodic call for timed event generation.
	 * The sequencer does not process audio buffers.
	 * The desired BPM will generally not correspond to an integral number
	 * of audio blocks, so accumulate an error and tick when needed.
	 * ie- Bresenham style.
	 */

	this->tick_error += SecsPerAudioBuffer;
	if (this->tick_error > this->secs_per_tick) {
		this->tick_error -= this->secs_per_tick;
		this->ticks++;
		/* tick the state machine */
		seq_tick(m);
	}
	return false;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "bpm", .type = PORT_TYPE_FLOAT, .pf = seq_port_bpm, .mf = seq_midi_bpm, },
	{ .name = "ctrl", .type = PORT_TYPE_INT, .pf = seq_port_ctrl },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "midi", .type = PORT_TYPE_MIDI, },
	PORT_EOL,
};

const struct module_info seq_seq_module = {
	.mname = "seq/seq",
	.iname = "seq",
	.in = in_ports,
	.out = out_ports,
	.alloc = seq_alloc,
	.free = seq_free,
	.process = seq_process,
};

MODULE_REGISTER(seq_seq_module);

/*****************************************************************************/
