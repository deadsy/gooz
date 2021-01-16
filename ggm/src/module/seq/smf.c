/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Standard MIDI File Sequencer
 */

#include "ggm.h"
#include "seq/seq.h"

/******************************************************************************
 * private state
 */

enum {
	SMF_STATE_STOP,		/* initial state */
	SMF_STATE_RUN,
};

struct smf {
	float secs_per_tick;	/* seconds per tick */
	int state;
};

/******************************************************************************
 * module port functions
 */

#define TICKS_PER_BEAT (16.0f)

static void smf_midi_bpm(struct event *dst, const struct event *src) {
	event_set_float(dst, map_lin(event_get_midi_cc_float(src), MinBeatsPerMin, MaxBeatsPerMin));
}

static void smf_port_bpm(struct module *m, const struct event *e) {
	struct smf *this = (struct smf *)m->priv;
	float bpm = clampf(event_get_float(e), MinBeatsPerMin, MaxBeatsPerMin);

	LOG_INF("%s:bpm %f", m->name, bpm);
	this->secs_per_tick = SecsPerMin / (bpm * TICKS_PER_BEAT);
}

static void smf_port_ctrl(struct module *m, const struct event *e) {
	struct smf *this = (struct smf *)m->priv;
	int ctrl = event_get_int(e);

	switch (ctrl) {
	case SEQ_CTRL_STOP:	/* stop the sequencer */
		LOG_INF("%s:ctrl stop", m->name);
		this->state = SMF_STATE_STOP;
		break;
	case SEQ_CTRL_START:	/* start the sequencer */
		LOG_INF("%s:ctrl start", m->name);
		this->state = SMF_STATE_RUN;
		break;
	case SEQ_CTRL_RESET:	/* reset the sequencer */
		LOG_INF("%s:ctrl reset", m->name);
		this->state = SMF_STATE_STOP;
		break;
	default:
		LOG_INF("%s:ctrl unknown value %d", m->name, ctrl);
		break;
	}
}

/******************************************************************************
 * module functions
 */

static int smf_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct smf *this = ggm_calloc(1, sizeof(struct smf));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	return 0;
}

static void smf_free(struct module *m) {
	struct smf *this = (struct smf *)m->priv;

	ggm_free(this);
}

static bool smf_process(struct module *m, float *bufs[]) {
	struct smf *this = (struct smf *)m->priv;
	float *out = bufs[0];

	(void)this;
	(void)out;

	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "bpm",.type = PORT_TYPE_FLOAT,.pf = smf_port_bpm,.mf = smf_midi_bpm,},
	{.name = "ctrl",.type = PORT_TYPE_INT,.pf = smf_port_ctrl},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "midi",.type = PORT_TYPE_MIDI,},
	PORT_EOL,
};

const struct module_info seq_smf_module = {
	.mname = "seq/smf",
	.iname = "smf",
	.in = in_ports,
	.out = out_ports,
	.alloc = smf_alloc,
	.free = smf_free,
	.process = smf_process,
};

MODULE_REGISTER(seq_smf_module);

/*****************************************************************************/
