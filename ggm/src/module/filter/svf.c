/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * State Variable Filters
 *
 * SVF_TYPE_HC:
 * See: Hal Chamberlin's "Musical Applications of Microprocessors" pp.489-492.
 *
 * SVF_TYPE_TRAPEZOIDAL:
 * See: https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
 */

#include "ggm.h"
#include "filter/filter.h"

/******************************************************************************
 * private state
 */

struct svf {

	int type;		/* filter type */
	/* SVF_TYPE_HC */
	float kf;		/* constant for cutoff frequency */
	float kq;		/* constant for filter resonance */
	float bp;		/* bandpass state variable */
	float lp;		/* low pass state variable */
	/* SVF_TYPE_TRAPEZOIDAL */
	float g;		/* constant for cutoff frequency */
	float k;		/* constant for filter resonance */
	float ic1eq;		/* state variable */
	float ic2eq;		/*state variable */
};

/******************************************************************************
 * svf functions
 */

static void svf_filter_hc(struct module *m, float *in, float *out) {
	struct svf *this = (struct svf *)m->priv;
	float lp = this->lp;
	float bp = this->bp;
	float kf = this->kf;
	float kq = this->kq;

	for (int i = 0; i < AudioBufferSize; i++) {
		lp += kf * bp;
		float hp = in[i] - lp - (kq * bp);
		bp += kf * hp;
		out[i] = lp;
	}

	// update the state variables
	this->lp = lp;
	this->bp = bp;
}

static void svf_filter_trapezoidal(struct module *m, float *in, float *out) {
	struct svf *this = (struct svf *)m->priv;
	float ic1eq = this->ic1eq;
	float ic2eq = this->ic2eq;
	float a1 = 1.f / (1.f + (this->g * (this->g + this->k)));
	float a2 = this->g * a1;
	float a3 = this->g * a2;

	for (int i = 0; i < AudioBufferSize; i++) {
		float v0 = in[i];
		float v3 = v0 - ic2eq;
		float v1 = (a1 * ic1eq) + (a2 * v3);
		float v2 = ic2eq + (a2 * ic1eq) + (a3 * v3);
		ic1eq = (2.f * v1) - ic1eq;
		ic2eq = (2.f * v2) - ic2eq;
		out[i] = v2;	// low
		// low := v2
		// band := v1
		// high := v0 - (this->k * v1) - v2
		// notch := v0 - (this->k * v1)
		// peak := v0 - (this->k * v1) - (2.0 * v2)
		// all := v0 - (2.0 * this->k * v1)
	}
	// update the state variables
	this->ic1eq = ic1eq;
	this->ic2eq = ic2eq;
}

/******************************************************************************
 * module port functions
 */

static void svf_port_cutoff(struct module *m, const struct event *e) {
	struct svf *this = (struct svf *)m->priv;
	float cutoff = clampf(event_get_float(e), 0.f, 0.5f * AudioSampleFrequency);

	LOG_INF("set cutoff frequency %f Hz", cutoff);
	switch (this->type) {
	case SVF_TYPE_HC:
		this->kf = 2.f * sinf(Pi * cutoff * AudioSamplePeriod);
		break;
	case SVF_TYPE_TRAPEZOIDAL:
		this->g = tanf(Pi * cutoff * AudioSamplePeriod);
		break;
	default:
		LOG_ERR("bad filter type %d", this->type);
		break;
	}
}

static void svf_port_resonance(struct module *m, const struct event *e) {
	struct svf *this = (struct svf *)m->priv;
	float resonance = clampf(event_get_float(e), 0.f, 1.f);

	LOG_INF("set resonance %f", resonance);
	switch (this->type) {
	case SVF_TYPE_HC:
		this->kq = 2.f - 2.f * resonance;
		break;
	case SVF_TYPE_TRAPEZOIDAL:
		this->k = 2.f - 2.f * resonance;
		break;
	default:
		LOG_ERR("bad filter type %d", this->type);
		break;
	}
}

/******************************************************************************
 * module functions
 */

static int svf_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct svf *this = ggm_calloc(1, sizeof(struct svf));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* set the filter type */
	this->type = va_arg(vargs, int);
	if ((this->type <= 0) || (this->type >= SVF_TYPE_MAX)) {
		LOG_ERR("bad filter type %d", this->type);
		goto error;
	}

	return 0;

 error:
	ggm_free(this);
	return -1;
}

static void svf_free(struct module *m) {
	struct svf *this = (struct svf *)m->priv;

	ggm_free(this);
}

static bool svf_process(struct module *m, float *bufs[]) {
	struct svf *this = (struct svf *)m->priv;
	float *in = bufs[0];
	float *out = bufs[1];

	switch (this->type) {
	case SVF_TYPE_HC:
		svf_filter_hc(m, in, out);
		break;
	case SVF_TYPE_TRAPEZOIDAL:
		svf_filter_trapezoidal(m, in, out);
		break;
	default:
		LOG_ERR("bad filter type %d", this->type);
		break;
	}
	return true;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "in",.type = PORT_TYPE_AUDIO,},
	{.name = "cutoff",.type = PORT_TYPE_FLOAT,.pf = svf_port_cutoff},
	{.name = "resonance",.type = PORT_TYPE_FLOAT,.pf = svf_port_resonance},
	PORT_EOL,
};

static const struct port_info out_ports[] = {
	{.name = "out",.type = PORT_TYPE_AUDIO,},
	PORT_EOL,
};

const struct module_info filter_svf_module = {
	.mname = "filter/svf",
	.iname = "svf",
	.in = in_ports,
	.out = out_ports,
	.alloc = svf_alloc,
	.free = svf_free,
	.process = svf_process,
};

MODULE_REGISTER(filter_svf_module);

/*****************************************************************************/
