/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define GGM_MAIN

#include <signal.h>
#include <unistd.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "ggm.h"
#include "module.h"

/******************************************************************************
 * jack data
 */

struct jack {
	struct synth *synth;
	jack_client_t *client;
	size_t n_audio_in;	/* number of input audio ports */
	size_t n_audio_out;	/* number of output audio ports */
	size_t n_midi_in;	/* number of input MIDI ports */
	size_t n_midi_out;	/* number of output MIDI ports */
	jack_port_t *audio_in[MAX_AUDIO_IN];	/* audio input jack ports */
	jack_port_t *audio_out[MAX_AUDIO_OUT];	/* audio output jack ports */
	jack_port_t *midi_in[MAX_MIDI_IN];	/* MIDI input jack ports */
	jack_port_t *midi_out[MAX_MIDI_OUT];	/* MIDI output jack ports */
	port_func midi_in_pf[MAX_MIDI_IN];	/* MIDI input port functions */
	void *midi_out_buf[MAX_MIDI_OUT];	/* MIDI output buffers */
};

/******************************************************************************
 * convert jack MIDI buffer to MIDI event
 */

static struct event *jack_convert_midi_event(struct event *dst, jack_midi_event_t * src) {
	uint8_t *buf = src->buffer;
	unsigned int n = src->size;

	// LOG_INF("time %d size %d buffer %p", src->time, src->size, src->buffer);

	if (n == 0) {
		LOG_WRN("jack midi event has no data");
		return NULL;
	}

	uint8_t status = buf[0];
	if (status < MIDI_STATUS_COMMON) {
		/* channel message */
		switch (status & 0xf0) {
		case MIDI_STATUS_NOTEOFF:
		case MIDI_STATUS_NOTEON:
		case MIDI_STATUS_POLYPHONICAFTERTOUCH:
		case MIDI_STATUS_CONTROLCHANGE:
		case MIDI_STATUS_PITCHWHEEL:{
				if (n == 3) {
					event_set_midi(dst, buf[0], buf[1], buf[2]);
					return dst;
				}
				LOG_WRN("jack midi event size != 3");
				break;
			}
		case MIDI_STATUS_PROGRAMCHANGE:
		case MIDI_STATUS_CHANNELAFTERTOUCH:{
				if (n == 2) {
					event_set_midi(dst, buf[0], buf[1], 0);
					return dst;
				}
				LOG_WRN("jack midi event size != 2");
				break;
			}
		default:
			LOG_WRN("unhandled channel msg %02x", status);
			break;
		}
	} else if (status < MIDI_STATUS_REALTIME) {
		/* system common message */
		LOG_WRN("unhandled system commmon msg %02x", status);
	} else {
		/* system real time message */
		LOG_WRN("unhandled system realtime msg %02x", status);
	}

	return NULL;
}

/******************************************************************************
 * jack ports
 */

static void jack_unregister_ports(jack_client_t * client, jack_port_t ** port, size_t n, const char *base_name) {
	if (n == 0) {
		return;
	}

	for (size_t i = 0; i < n; i++) {
		if (port[i] != NULL) {
			int err = jack_port_unregister(client, port[i]);
			if (err != 0) {
				LOG_ERR("unable to unregister %s_%lu", base_name, i);
			} else {
				LOG_DBG("unregistered %s_%lu", base_name, i);
			}
			port[i] = NULL;
		}
	}
}

static size_t jack_register_ports(jack_client_t * client, jack_port_t ** port, size_t n, const char *base_name, const char *type, unsigned long flags) {
	for (size_t i = 0; i < n; i++) {
		char name[128];
		snprintf(name, sizeof(name), "%s_%lu", base_name, i);
		port[i] = jack_port_register(client, name, type, flags, 0);
		if (port[i] == NULL) {
			LOG_ERR("unable to register port %s", name);
			goto error;
		}
		LOG_DBG("registered %s", name);
	}
	return n;

 error:
	jack_unregister_ports(client, port, n, base_name);
	return 0;
}

/******************************************************************************
 * jack callbacks
 */

static int jack_process(jack_nframes_t nframes, void *arg) {
	struct jack *j = (struct jack *)arg;
	struct synth *s = (struct synth *)j->synth;
	size_t i;

	// LOG_DBG("nframes %d", nframes);

	/* read MIDI input events */
	for (i = 0; i < j->n_midi_in; i++) {
		void *buf = jack_port_get_buffer(j->midi_in[i], nframes);
		if (buf == NULL) {
			LOG_ERR("jack_port_get_buffer() returned NULL");
			continue;
		}
		jack_nframes_t n = jack_midi_get_event_count(buf);
		for (unsigned int idx = 0; idx < n; idx++) {
			jack_midi_event_t event;
			int err = jack_midi_event_get(&event, buf, idx);
			if (err != 0) {
				LOG_ERR("jack_midi_event_get() returned %d", err);
				continue;
			}
			/* forward the MIDI event to the root module of the synth  */
			port_func func = j->midi_in_pf[i];
			if (func != NULL) {
				struct event e;
				func(s->root, jack_convert_midi_event(&e, &event));
			} else {
				LOG_WRN("midi_in_%d has a null port function", i);
			}
		}
	}

	/* prepare the MIDI output buffers */
	for (i = 0; i < j->n_midi_out; i++) {
		void *buf = jack_port_get_buffer(j->midi_out[i], nframes);
		jack_midi_clear_buffer(buf);
		j->midi_out_buf[i] = buf;
	}

	/* read from the audio input buffers */
	for (i = 0; i < j->n_audio_in; i++) {
		float *buf = (float *)jack_port_get_buffer(j->audio_in[i], nframes);
		block_copy(s->bufs[i], buf);
	}

	/* run the synth loop */
	bool active = synth_loop(s);

	/* write to the audio output buffers */
	unsigned int ofs = j->n_audio_in;
	for (i = 0; i < j->n_audio_out; i++) {
		float *buf = (float *)jack_port_get_buffer(j->audio_out[i], nframes);
		if (active) {
			block_copy(buf, s->bufs[ofs + i]);
		} else {
			block_zero(buf);
		}
	}

	return 0;
}

static bool synth_running;

static void jack_shutdown(void *arg) {
	LOG_INF("jackd stopped, exiting");
	synth_running = false;
}

/******************************************************************************
 * jack_midi_out is a called from the synth loop to send a MIDI
 * message on a specific MIDI output port. It follows the midi_out_func
 * prototype.
 */

static void jack_midi_out(void *arg, const struct event *e, int idx) {
	struct jack *j = (struct jack *)arg;
	uint8_t *msg = jack_midi_event_reserve(j->midi_out_buf[idx], 0, 3);

	if (msg) {
		msg[0] = e->u.midi.status;
		msg[1] = e->u.midi.arg0;
		msg[2] = e->u.midi.arg1;
	} else {
		LOG_ERR("unable to output to midi_out_%d", idx);
	}
}

/******************************************************************************
 * jack new/del
 */

static void jack_del(struct jack *j) {
	LOG_INF("");
	if (j == NULL) {
		return;
	}
	if (j->client != NULL) {
		jack_deactivate(j->client);
		jack_unregister_ports(j->client, j->audio_in, j->n_audio_in, "audio_in");
		jack_unregister_ports(j->client, j->audio_out, j->n_audio_out, "audio_out");
		jack_unregister_ports(j->client, j->midi_in, j->n_midi_in, "midi_in");
		jack_unregister_ports(j->client, j->midi_out, j->n_midi_out, "midi_out");
		jack_client_close(j->client);
	}
	ggm_free(j);
}

static struct jack *jack_new(struct synth *s) {
	jack_status_t status;
	size_t n;
	int err;

	LOG_INF("jack version %s", jack_get_version_string());

	if (!synth_has_root(s)) {
		LOG_ERR("synth does not have a root patch");
		return NULL;
	}

	struct jack *j = ggm_calloc(1, sizeof(struct jack));
	if (j == NULL) {
		LOG_ERR("cannot allocate jack data");
		return NULL;
	}

	/* setup the synth/jack link */
	j->synth = s;
	s->driver = (void *)j;
	s->midi_out = jack_midi_out;

	/* count and check the in/out ports */
	struct module *m = s->root;
	j->n_audio_in = port_count_by_type(m->info->in, PORT_TYPE_AUDIO);
	if (j->n_audio_in > MAX_AUDIO_IN) {
		LOG_ERR("number of audio inputs(%d) > MAX_AUDIO_IN", j->n_audio_in);
		goto error;
	}
	j->n_audio_out = port_count_by_type(m->info->out, PORT_TYPE_AUDIO);
	if (j->n_audio_out > MAX_AUDIO_OUT) {
		LOG_ERR("number of audio outputs(%d) > MAX_AUDIO_OUT", j->n_audio_out);
		goto error;
	}
	j->n_midi_in = port_count_by_type(m->info->in, PORT_TYPE_MIDI);
	if (j->n_midi_in > MAX_MIDI_IN) {
		LOG_ERR("number of midi inputs(%d) > MAX_MIDI_IN", j->n_midi_in);
		goto error;
	}
	j->n_midi_out = port_count_by_type(m->info->out, PORT_TYPE_MIDI);
	if (j->n_midi_out > MAX_MIDI_OUT) {
		LOG_ERR("number of midi outputs(%d) > MAX_MIDI_OUT", j->n_midi_out);
		goto error;
	}

	/* open the client */
	j->client = jack_client_open("ggm", JackNoStartServer, &status);
	if (j->client == NULL) {
		LOG_ERR("jack server not running");
		goto error;
	}

	/* check sample rate */
	jack_nframes_t rate = jack_get_sample_rate(j->client);
	if (rate != AudioSampleFrequency) {
		LOG_ERR("jack sample rate %d != ggm sample rate %d", rate, AudioSampleFrequency);
		goto error;
	}

	/* check audio buffer size */
	jack_nframes_t bufsize = jack_get_buffer_size(j->client);
	if (bufsize != AudioBufferSize) {
		LOG_ERR("jack buffer size %d != ggm buffer size %d", bufsize, AudioBufferSize);
		goto error;
	}

	/* tell the JACK server to call jack_process() whenever there is work to be done. */
	err = jack_set_process_callback(j->client, jack_process, (void *)j);
	if (err != 0) {
		LOG_ERR("jack_set_process_callback() error %d", err);
		goto error;
	}

	/* tell the JACK server to call shutdown() if it ever shuts down,
	 * either entirely, or if it just decides to stop calling us.
	 */
	jack_on_shutdown(j->client, jack_shutdown, (void *)j);

	/* audio input ports */
	n = jack_register_ports(j->client, j->audio_in, j->n_audio_in, "audio_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
	if (n != j->n_audio_in) {
		goto error;
	}

	/* audio output ports */
	n = jack_register_ports(j->client, j->audio_out, j->n_audio_out, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
	if (n != j->n_audio_out) {
		goto error;
	}

	/* MIDI input ports */
	n = jack_register_ports(j->client, j->midi_in, j->n_midi_in, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
	if (n != j->n_midi_in) {
		goto error;
	}

	/* MIDI output ports */
	n = jack_register_ports(j->client, j->midi_out, j->n_midi_out, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
	if (n != j->n_midi_out) {
		goto error;
	}

	/* cache the MIDI input port functions */
	for (size_t i = 0; i < j->n_midi_in; i++) {
		const struct port_info *pi = port_get_info_by_type(m->info->in, PORT_TYPE_MIDI, i);
		j->midi_in_pf[i] = pi->pf;
	}

	/* Tell the JACK server we are ready to roll.
	 * Our process() callback will start running now.
	 */
	err = jack_activate(j->client);
	if (err != 0) {
		LOG_ERR("jack_activate() error %d", err);
		goto error;
	}

	return j;

 error:

	if (j->client != NULL) {
		jack_unregister_ports(j->client, j->audio_in, j->n_audio_in, "audio_in");
		jack_unregister_ports(j->client, j->audio_out, j->n_audio_out, "audio_out");
		jack_unregister_ports(j->client, j->midi_in, j->n_midi_in, "midi_in");
		jack_unregister_ports(j->client, j->midi_out, j->n_midi_out, "midi_out");
		jack_client_close(j->client);
	}

	ggm_free(j);

	return NULL;
}

/******************************************************************************
 * main
 */

static void ctrl_c_handler(int sig) {
	LOG_INF("caught ctrl-c, exiting");
	synth_running = false;
}

int main(void) {
	struct jack *j = NULL;
	int err;

	log_set_prefix("ggm/src/");

	LOG_INF("GooGooMuck %s (%s)", GGM_VERSION, CONFIG_BOARD);

	struct synth *s = synth_new();
	if (s == NULL) {
		goto exit;
	}

	struct module *m = module_root(s, "root/poly", -1);
	if (m == NULL) {
		goto exit;
	}

	err = synth_set_root(s, m);
	if (err != 0) {
		goto exit;
	}

	j = jack_new(s);
	if (j == NULL) {
		goto exit;
	}

	/* catch ctrl-c */
	void *hdl = signal(SIGINT, ctrl_c_handler);
	if (hdl == SIG_ERR) {
		LOG_ERR("can't set SIGINT signal handler");
		goto exit;
	}

	synth_running = true;
	while (synth_running) {
		sleep(1);
	}

 exit:
	jack_del(j);
	synth_del(s);
	return 0;
}

/*****************************************************************************/
