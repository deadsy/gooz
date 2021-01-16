/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * event queue - events are dispatched to modules between calls to process
 * audio buffers. Events generated during buffer processing are queued and then
 * dispatched after the buffer processing is completed.
 */

/* synth_event_rd reads an event from the event queue */
static int synth_event_rd(struct synth *s, struct qevent *e)
{
	struct event_queue *eq = &s->eq;
	int rc = 0;

	/* do we have events? */
	if (eq->rd == eq->wr) {
		/* no events */
		rc = -1;
		goto exit;
	}

	/* copy the event data */
	struct qevent *x = &eq->queue[eq->rd];
	memcpy(e, x, sizeof(struct qevent));

	/* advance the read index */
	eq->rd = (eq->rd + 1) & (NUM_EVENTS - 1);

exit:
	return rc;
}

/* synth_event_wr writes an event to the event queue */
int synth_event_wr(struct synth *s, struct module *m, int idx, const struct event *e)
{
	struct event_queue *eq = &s->eq;
	int rc = 0;

	/* do we have queue space? */
	size_t wr = (eq->wr + 1) & (NUM_EVENTS - 1);

	if (wr == eq->rd) {
		/* the queue is full */
		rc = -1;
		goto exit;
	}

	/* copy the event data */
	struct qevent *x = &eq->queue[eq->wr];
	x->m = m;
	x->idx = idx;
	memcpy(&x->e, e, sizeof(struct event));

	/* advance the write index */
	eq->wr = wr;

exit:
	return rc;
}

/******************************************************************************
 * synth_new allocates a new synth.
 */

struct synth *synth_new(void)
{
	/* allocate the synth */
	struct synth *s = ggm_calloc(1, sizeof(struct synth));

	if (s == NULL) {
		LOG_ERR("could not allocate synth");
		return NULL;
	}
	LOG_INF("synth (%d bytes)", sizeof(struct synth));
	return s;
}

/******************************************************************************
 * synth_del closes a synth and deallocates resources.
 */

void synth_del(struct synth *s)
{
	if (s == NULL) {
		return;
	}

	module_del(s->root);

	/* free the allocated audio buffers */
	ggm_free(s->bufs[0]);
	ggm_free(s);
}

/******************************************************************************
 * synth_midi_out is called when the running top-level module has a MIDI
 * message to output. It has the prototype of a port function, and the module
 * will be the root module.
 */

#if MAX_MIDI_OUT > 0
static void synth_midi_out_0(struct module *m, const struct event *e)
{
	struct synth *s = m->top;

	s->midi_out(s->driver, e, 0);
}
#endif

#if MAX_MIDI_OUT > 1
static void synth_midi_out_1(struct module *m, const struct event *e)
{
	struct synth *s = m->top;

	s->midi_out(s->driver, e, 1);
}
#endif

#if MAX_MIDI_OUT > 2
static void synth_midi_out_2(struct module *m, const struct event *e)
{
	struct synth *s = m->top;

	s->midi_out(s->driver, e, 2);
}
#endif

#if MAX_MIDI_OUT > 3
static void synth_midi_out_3(struct module *m, const struct event *e)
{
	struct synth *s = m->top;

	s->midi_out(s->driver, e, 3);
}
#endif

static port_func synth_midi_out[MAX_MIDI_OUT] = {
#if MAX_MIDI_OUT > 0
	synth_midi_out_0,
#endif
#if MAX_MIDI_OUT > 1
	synth_midi_out_1,
#endif
#if MAX_MIDI_OUT > 2
	synth_midi_out_2,
#endif
#if MAX_MIDI_OUT > 3
	synth_midi_out_3,
#endif
};

/******************************************************************************
 * Incoming MIDI CC messages are sent directly to the sub-module(s) for which
 * they are relevant. The map is between a module:port path and the MIDI
 * channel:cc numbers.
 */

/* synth_module_cfg sets the top-level synth MIDI map configuration */
int synth_set_cfg(struct synth *s, const struct synth_cfg *cfg)
{
	if (cfg == NULL) {
		LOG_ERR("synth cfg null");
		return -1;
	}
	if (s->cfg != NULL) {
		LOG_ERR("synth cfg already set");
		return -1;
	}
	s->cfg = cfg;
	return 0;
}

/* synth_lookup_cfg looks for a path match in the synth configuration.
 * If a match is found it returns the configuration structure pointer.
 */
static const void *synth_lookup_cfg(struct synth *s, const char *path)
{
	const struct synth_cfg *sc = s->cfg;

	while (sc->path != NULL) {
		if (match(sc->path, path)) {
			return sc->cfg;
		}
		sc++;
	}
	return NULL;
}

/******************************************************************************
 * Incoming MIDI CC messages are sent directly to the sub-module(s) for which
 * they are relevant. The map is between a module:port path and the MIDI
 * channel:cc numbers.
 */

/* synth_lookup_midi_map looks for the given ch/cc values in the MIDI map.
 * If alloc is true it will return a pointer to a new slot (if available).
 */
static struct midi_map *synth_lookup_midi_map(struct synth *s, int id, bool alloc)
{
	struct midi_map *mm = &s->mmap[0];

	for (int i = 0; i < NUM_MIDI_MAP_SLOTS; i++) {
		/* does this slot match? */
		if (mm[i].id == id) {
			return &mm[i];
		}
		/* is the slot empty? */
		if (mm[i].id == 0) {
			if (alloc) {
				/* allocate the slot */
				mm[i].id = id;
				return &mm[i];
			} else {
				/* end of list */
				return NULL;
			}
		}
	}
	return NULL;
}

/* synth_alloc_midi_map_entry allocates an empty  midi map entry */
static struct midi_map_entry *synth_alloc_midi_map_entry(struct midi_map *mm)
{
	struct midi_map_entry *mme = &mm->mme[0];

	for (int i = 0; i < NUM_MIDI_MAP_ENTRIES; i++) {
		if (mme[i].m == NULL) {
			return &mme[i];
		}
	}
	return NULL;
}


/* synth_midi_cc looks up the midi mapping table.
 * If it finds a matching entry the event is dispatched
 * to the module:port function.
 */
bool synth_midi_cc(struct synth *s, const struct event *e)
{
	if (!is_midi_cc(e)) {
		return false;
	}

	int id = MIDI_ID(event_get_midi_channel(e), event_get_midi_cc_num(e));

	/* find the midi map entry slot */
	struct midi_map *mm = synth_lookup_midi_map(s, id, false);
	if (mm == NULL) {
		return false;
	}

	/* call the port functions */
	struct midi_map_entry *mme = &mm->mme[0];
	for (int i = 0; i < NUM_MIDI_MAP_ENTRIES; i++) {
		if (mme[i].m == NULL) {
			/* no more ports on this ch/cc */
			break;
		}
		midi_func mf = mme[i].pi->mf;
		port_func pf = mme[i].pi->pf;
		/* convert from a MIDI event to a port event*/
		struct event pe;
		mf(&pe, e);
		/* dispatch it to the port function */
		pf(mme[i].m, &pe);
	}

	return true;
}

/******************************************************************************
 * synth_input_cfg configures the input port of a module
 */

void synth_input_cfg(struct synth *s, struct module *m, const struct port_info *pi)
{
	/* build the full module:port name*/
	char path[128];

	snprintf(path, sizeof(path), "%s:%s", m->name, pi->name);

	/* look for a match in the top-level synth configuration */
	const void *ptr = synth_lookup_cfg(s, path);
	if (ptr == NULL) {
		// LOG_DBG("%s not found", path);
		return;
	}

	/* send events for the initial configuration to the port */

	int id = 0; /* MIDI ch/cc id */
	switch (pi->type) {
	case PORT_TYPE_FLOAT: {
		struct port_float_cfg *cfg = (struct port_float_cfg *)ptr;
		id = cfg->id;
		event_in_float(m, pi->name, cfg->init, NULL);
		break;
	}
	case PORT_TYPE_INT: {
		struct port_int_cfg *cfg = (struct port_int_cfg *)ptr;
		id = cfg->id;
		event_in_int(m, pi->name, cfg->init, NULL);
		break;
	}
	case PORT_TYPE_BOOL: {
		struct port_bool_cfg *cfg = (struct port_bool_cfg *)ptr;
		id = cfg->id;
		event_in_bool(m, pi->name, cfg->init, NULL);
		break;
	}
	default:
		LOG_ERR("is this port configurable? %s", path);
		return;
	}

	/* now setup the MIDI cc mapping */

	if (id == 0) {
		/* We're not using MIDI cc for this input port */
		return;
	}

	if (pi->mf == NULL) {
		/* The port has no MIDI function to convert a MIDI event
		 * into a port event, ignore it.
		 */
		LOG_ERR("%s doesn't have a MIDI function", path);
		return;
	}

	/* find the midi map entry slot */
	struct midi_map *mm = synth_lookup_midi_map(s, id, true);
	if (mm == NULL) {
		LOG_ERR("not enough midi map slots (NUM_MIDI_MAP_SLOTS)");
		return;
	}

	/* allocate a midi map entry */
	struct midi_map_entry *mme = synth_alloc_midi_map_entry(mm);
	if (mme == NULL) {
		LOG_ERR("not enough midi map entries (NUM_MIDI_MAP_ENTRIES)");
		return;
	}

	/* fill in the midi map entry */
	mme->m = m;
	mme->pi = pi;
	LOG_DBG("%s mapped to cc %d/%d", path, MIDI_ID_CH(id), MIDI_ID_CC(id));
}

/******************************************************************************
 * synth_set_root sets the root patch of the synth.
 */

int synth_set_root(struct synth *s, struct module *m)
{
	int nports;

	LOG_INF("%s", m->name);

	/* TODO - test for unique input/output names */

	/* check the number of MIDI input ports */
	nports = port_count_by_type(m->info->in, PORT_TYPE_MIDI);
	if (nports > MAX_MIDI_IN) {
		LOG_ERR("number of MIDI input ports > MAX_MIDI_IN");
		return -1;
	}

	/* check the number of MIDI output ports */
	nports = port_count_by_type(m->info->out, PORT_TYPE_MIDI);
	if (nports > MAX_MIDI_OUT) {
		LOG_ERR("number of MIDI output ports > MAX_MIDI_OUT");
		return -1;
	}

	/* hookup the MIDI output ports to the MIDI driver callback */
	for (int i = 0; i < nports; i++) {
		int idx = port_get_index_by_type(m->info->out, PORT_TYPE_MIDI, i);
		port_add_dst(m, idx, m, synth_midi_out[i]);
	}

	/* how many audio buffers do we need? */
	size_t nbufs = port_count_by_type(m->info->in, PORT_TYPE_AUDIO);
	nbufs += port_count_by_type(m->info->out, PORT_TYPE_AUDIO);
	if (nbufs > MAX_AUDIO_PORTS) {
		LOG_ERR("number of audio input + output ports > MAX_AUDIO_PORTS");
		return -1;
	}

	/* allocate the audio buffers */
	float *buf = ggm_calloc(nbufs, AudioBufferSize * sizeof(float));
	if (buf == NULL) {
		LOG_ERR("could not allocate audio buffers");
		return -1;
	}

	/* setup the audio buffer list */
	for (size_t i = 0; i < nbufs; i++) {
		s->bufs[i] = &buf[i * AudioBufferSize];
	}

	s->root = m;
	return 0;
}

bool synth_has_root(struct synth *s)
{
	return s->root != NULL;
}

/******************************************************************************
 * synth_loop runs the top-level synth loop - returns true if the output
 * buffers are non-zero.
 */

bool synth_loop(struct synth *s)
{
	struct module *m = s->root;
	struct qevent q;

	/* run the buffer processing */
	bool active = m->info->process(m, s->bufs);

	/* process all queued events */
	while (synth_event_rd(s, &q) == 0) {
		event_out(q.m, q.idx, &q.e);
	}

	return active;
}

/*****************************************************************************/
