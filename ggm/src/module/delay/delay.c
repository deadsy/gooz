/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Audio Sample Delay Line
 */

#include "ggm.h"

/******************************************************************************
 * private state
 */

struct delay {
	float *buf;     /* delay line buffer */
	float t;        /* delay line length (secs) */
	int n;          /* delay line length (samples) */
	int wr;         /* write index */
};

/******************************************************************************
 * module port functions
 */

/******************************************************************************
 * module functions
 */

static int delay_alloc(struct module *m, va_list vargs)
{
	/* allocate the private data */
	struct delay *this = ggm_calloc(1, sizeof(struct delay));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* delay line length (samples) */
	int samples = va_arg(vargs, int);
	if (samples <= 0) {
		LOG_ERR("delay samples must be > 0");
		goto error;
	}

	/* allocate the delay line */
	this->n = (size_t)samples;
	this->t = (float)this->n * AudioSamplePeriod;
	this->buf = (float *)ggm_calloc(this->n, sizeof(float));
	if (this->buf == NULL) {
		LOG_ERR("unable to allocate delay line of %d samples", this->n);
		goto error;
	}

	LOG_DBG("%s %d samples %f secs", m->name, this->n, this->t);

	return 0;

error:

	ggm_free(this->buf);
	ggm_free(this);
	return -1;
}

static void delay_free(struct module *m)
{
	struct delay *this = (struct delay *)m->priv;

	ggm_free(this->buf);
	ggm_free(this);
}

static bool delay_process(struct module *m, float *bufs[])
{
	struct delay *this = (struct delay *)m->priv;
	float *in = bufs[0];
	float *out = bufs[1];
	int eob = this->n - 1;

	for (int i = 0; i < AudioBufferSize; i++) {
		/* read index */
		int rd = this->wr - 1;
		if (rd < 0) {
			rd = eob;
		}
		/* write/read */
		this->buf[this->wr] = in[i];
		out[i] = this->buf[rd];
		/* advance the write index */
		this->wr += 1;
		if (this->wr > eob) {
			this->wr = 0;
		}
	}
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{ .name = "in", .type = PORT_TYPE_AUDIO, },
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{ .name = "out", .type = PORT_TYPE_AUDIO, },
	PORT_EOL,
};

const struct module_info delay_delay_module = {
	.mname = "delay/delay",
	.iname = "delay",
	.in = in_ports,
	.out = out_ports,
	.alloc = delay_alloc,
	.free = delay_free,
	.process = delay_process,
};

MODULE_REGISTER(delay_delay_module);

/*****************************************************************************/
