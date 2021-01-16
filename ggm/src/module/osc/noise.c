/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Noise Generator Module
 *
 * https://noisehack.com/generate-noise-web-audio-api/
 * http://www.musicdsp.org/files/pink.txt
 * https://en.wikipedia.org/wiki/Pink_noise
 * https://en.wikipedia.org/wiki/White_noise
 * https://en.wikipedia.org/wiki/Brownian_noise
 *
 * Arguments:
 * int, type of noise to generate
 */

#include "ggm.h"
#include "osc/osc.h"

/******************************************************************************
 * private state
 */

struct noise {
	int type;		/* noise type */
	uint32_t rand;		/* random state */
	float b0, b1, b2, b3;	/* state variables */
	float b4, b5, b6;	/* state variables */
};

/******************************************************************************
 * module port functions
 */

static void noise_port_null(struct module *m, const struct event *e) {
	// do nothing ...
}

/******************************************************************************
 * noise generating functions
 */

static void generate_white(struct module *m, float *out) {
	struct noise *this = (struct noise *)m->priv;

	for (int i = 0; i < AudioBufferSize; i++) {
		out[i] = randf(&this->rand);
	}
}

static void generate_brown(struct module *m, float *out) {
	struct noise *this = (struct noise *)m->priv;
	float b0 = this->b0;

	for (int i = 0; i < AudioBufferSize; i++) {
		float white = randf(&this->rand);
		b0 = (b0 + (0.02f * white)) * (1.0f / 1.02f);
		out[i] = b0 * (1.0f / 0.38f);
	}
	this->b0 = b0;
}

static void generate_pink1(struct module *m, float *out) {
	struct noise *this = (struct noise *)m->priv;
	float b0 = this->b0;
	float b1 = this->b1;
	float b2 = this->b2;

	for (int i = 0; i < AudioBufferSize; i++) {
		float white = randf(&this->rand);
		b0 = 0.99765f * b0 + white * 0.0990460f;
		b1 = 0.96300f * b1 + white * 0.2965164f;
		b2 = 0.57000f * b2 + white * 1.0526913f;
		float pink = b0 + b1 + b2 + white * 0.1848f;
		out[i] = pink * (1.0f / 10.4f);
	}
	this->b0 = b0;
	this->b1 = b1;
	this->b2 = b2;
}

static void generate_pink2(struct module *m, float *out) {
	struct noise *this = (struct noise *)m->priv;
	float b0 = this->b0;
	float b1 = this->b1;
	float b2 = this->b2;
	float b3 = this->b3;
	float b4 = this->b4;
	float b5 = this->b5;
	float b6 = this->b6;

	for (int i = 0; i < AudioBufferSize; i++) {
		float white = randf(&this->rand);
		b0 = 0.99886f * b0 + white * 0.0555179f;
		b1 = 0.99332f * b1 + white * 0.0750759f;
		b2 = 0.96900f * b2 + white * 0.1538520f;
		b3 = 0.86650f * b3 + white * 0.3104856f;
		b4 = 0.55000f * b4 + white * 0.5329522f;
		b5 = -0.7616f * b5 - white * 0.0168980f;
		float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
		b6 = white * 0.115926f;
		out[i] = pink * (1.0f / 10.2f);
	}
	this->b0 = b0;
	this->b1 = b1;
	this->b2 = b2;
	this->b3 = b3;
	this->b4 = b4;
	this->b5 = b5;
	this->b6 = b6;
}

/******************************************************************************
 * module functions
 */

static int noise_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct noise *this = ggm_calloc(1, sizeof(struct noise));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set the noise type */
	this->type = va_arg(vargs, int);
	if ((this->type <= 0) || (this->type >= NOISE_TYPE_MAX)) {
		LOG_ERR("bad noise type %d", this->type);
		goto error;
	}

	/* initialise the random seed */
	rand_init(0, &this->rand);

	return 0;

 error:
	ggm_free(this);
	return -1;
}

static void noise_free(struct module *m) {
	struct noise *this = (struct noise *)m->priv;

	ggm_free(this);
}

static bool noise_process(struct module *m, float *bufs[]) {
	struct noise *this = (struct noise *)m->priv;
	float *out = bufs[0];

	switch (this->type) {
	case NOISE_TYPE_PINK1:
		generate_pink1(m, out);
		break;
	case NOISE_TYPE_PINK2:
		generate_pink2(m, out);
		break;
	case NOISE_TYPE_WHITE:
		generate_white(m, out);
		break;
	case NOISE_TYPE_BROWN:
		generate_brown(m, out);
		break;
	default:
		LOG_ERR("bad noise type %d", this->type);
		break;
	}
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "reset",.type = PORT_TYPE_BOOL,.pf = noise_port_null},
	{.name = "frequency",.type = PORT_TYPE_FLOAT,.pf = noise_port_null},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info osc_noise_module = {
	.mname = "osc/noise",
	.iname = "noise",
	.in = in_ports,
	.out = out_ports,
	.alloc = noise_alloc,
	.free = noise_free,
	.process = noise_process,
};

MODULE_REGISTER(osc_noise_module);

/*****************************************************************************/
