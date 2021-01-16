/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Plotting Module
 *
 * When this module is triggered it writes the input signal to python code
 * that uses the plot.ly library to create a plot of the signal.
 */

#include "ggm.h"
#include "view/view.h"

#if !defined(__LINUX__)
#error "Sorry, this module only works with Linux."
#endif

/******************************************************************************
 * private state
 */

struct plot {
	struct plot_cfg *cfg;	/* plot configuration */
	uint32_t x;		/* current x-value */
	uint32_t samples;	/* number of samples to plot per trigger */
	uint32_t samples_left;	/* samples left in this trigger */
	uint32_t idx;		/* file index number */
	bool triggered;		/* are we currently triggered? */
	FILE *f;		/* output file */
};

/******************************************************************************
 * plotting functions
 */

static struct plot_cfg default_cfg = {
	.name = "plot",
	.title = "Plot",
	.x_name = "time",
	.y0_name = "amplitude",
	.duration = 30.f * SecsPerAudioBuffer,
};

static void plot_set_config(struct module *m, struct plot_cfg *cfg) {
	struct plot *this = (struct plot *)m->priv;

	if (cfg == NULL) {
		this->cfg = &default_cfg;
	} else {
		this->cfg = cfg;
		if (this->cfg->name == NULL) {
			this->cfg->name = default_cfg.name;
		}
		if (this->cfg->title == NULL) {
			this->cfg->title = default_cfg.title;
		}
		if (this->cfg->x_name == NULL) {
			this->cfg->x_name = default_cfg.x_name;
		}
		if (this->cfg->y0_name == NULL) {
			this->cfg->y0_name = default_cfg.y0_name;
		}
		if (this->cfg->duration <= 0.f) {
			this->cfg->duration = default_cfg.duration;
		}
	}
}

static void plot_fname(struct module *m, const char *suffix, char *s, size_t n) {
	struct plot *this = (struct plot *)m->priv;

	snprintf(s, n, "%s_%08x_%d.%s", this->cfg->name, m->id, this->idx, suffix);
}

#define PLOT_HEADER		   \
	"#!/usr/bin/env python3\n" \
	"import plotly\n"

/* plot_header adds a header to the python plot file. */
static int plot_header(struct module *m) {
	struct plot *this = (struct plot *)m->priv;

	return fprintf(this->f, PLOT_HEADER);
}

#define PLOT_FOOTER							\
	"data = [\n"							\
	"\tplotly.graph_objs.Scatter(\n"				\
	"\t\tx=x,\n"							\
	"\t\ty=y0,\n"							\
	"\t\tmode = 'lines',\n"						\
	"\t),\n"							\
	"]\n"								\
	"layout = plotly.graph_objs.Layout(\n"				\
	"\ttitle='%s',\n"						\
	"\txaxis=dict(\n"						\
	"\t\ttitle='%s',\n"						\
	"\t),\n"							\
	"\tyaxis=dict(\n"						\
	"\t\ttitle='%s',\n"						\
	"\t\trangemode='tozero',\n"					\
	"\t),\n"							\
	")\n"								\
	"figure = plotly.graph_objs.Figure(data=data, layout=layout)\n"	\
	"plotly.offline.plot(figure, filename='%s.html')\n"

/* plot_footer adds a footer to the python plot file. */
static int plot_footer(struct module *m) {
	struct plot *this = (struct plot *)m->priv;
	char name[128];

	plot_fname(m, "html", name, sizeof(name));
	return fprintf(this->f, PLOT_FOOTER, this->cfg->title, this->cfg->x_name, this->cfg->y0_name, name);
}

/* plot_new_variable adds a new variable to the plot file. */
static int plot_new_variable(struct module *m, const char *name) {
	struct plot *this = (struct plot *)m->priv;

	return fprintf(this->f, "%s = []\n", name);
}

/* plot_open opens a plot file. */
static int plot_open(struct module *m) {
	struct plot *this = (struct plot *)m->priv;
	char name[128];

	plot_fname(m, "py", name, sizeof(name));
	LOG_INF("open %s", name);
	FILE *f = fopen(name, "w");
	if (f == NULL) {
		LOG_ERR("unable to open plot file");
		return -1;
	}

	this->f = f;

	/* add header */
	int err = plot_header(m);
	if (err < 0) {
		LOG_ERR("unable to write plot header");
		fclose(this->f);
		return -1;
	}
	return 0;
}

/* plot_close closes the plot file. */
static void plot_close(struct module *m) {
	struct plot *this = (struct plot *)m->priv;

	LOG_INF("close plot file");
	/* add footer */
	plot_footer(m);
	fclose(this->f);
	this->idx++;
}

static int plot_append(struct module *m, const char *name, float *buf, int n) {
	struct plot *this = (struct plot *)m->priv;

	fprintf(this->f, "%s.extend([\n", name);
	for (int i = 0; i < n; i++) {
		fprintf(this->f, "%f,", buf[i]);
		if ((i & 15) == 15) {
			fprintf(this->f, "\n");
		}
	}
	fprintf(this->f, "])\n");
	return 0;
}

/******************************************************************************
 * module port functions
 */

static void plot_port_trigger(struct module *m, const struct event *e) {
	struct plot *this = (struct plot *)m->priv;
	bool trigger = event_get_bool(e);

	if (!trigger) {
		return;
	}

	if (this->triggered) {
		LOG_INF("%s already triggered", m->name);
		return;
	}
	// trigger!
	int rc = plot_open(m);
	if (rc != 0) {
		LOG_INF("can't open plot file");
		return;
	}

	plot_new_variable(m, "x");
	plot_new_variable(m, "y0");
	this->triggered = true;
	this->samples_left = this->samples;
}

/******************************************************************************
 * module functions
 */

static int plot_alloc(struct module *m, va_list vargs) {
	/* allocate the private data */
	struct plot *this = ggm_calloc(1, sizeof(struct plot));

	if (this == NULL) {
		return -1;
	}
	m->priv = (void *)this;

	/* plot configuration */
	struct plot_cfg *cfg = va_arg(vargs, struct plot_cfg *);
	plot_set_config(m, cfg);

	/* set the sampling duration */
	if (this->cfg->duration <= 0) {
		/* get N buffers of samples */
		this->samples = 4 * AudioBufferSize;
	} else {
		this->samples = maxi(16, (int)(this->cfg->duration / AudioSamplePeriod));
	}

	return 0;
}

static void plot_free(struct module *m) {
	struct plot *this = (struct plot *)m->priv;

	if (this->triggered) {
		plot_close(m);
	}
	ggm_free(this);
}

static bool plot_process(struct module *m, float *bufs[]) {
	struct plot *this = (struct plot *)m->priv;

	if (this->triggered) {
		float *x = bufs[0];
		float *y0 = bufs[1];

		/* how many samples should we plot? */
		int n = mini(this->samples_left, AudioBufferSize);

		/* plot x */
		if (x != NULL) {
			plot_append(m, "x", x, n);
		} else {
			/* no x data - use the internal timebase */
			float time[n];
			float base = (float)this->x * AudioSamplePeriod;
			for (int i = 0; i < n; i++) {
				time[i] = base;
				base += AudioSamplePeriod;
			}
			plot_append(m, "x", time, n);
		}

		/* plot y */
		if (y0 != NULL) {
			plot_append(m, "y0", y0, n);
		}
		this->samples_left -= n;
		/* are we done? */
		if (this->samples_left == 0) {
			plot_close(m);
			this->triggered = false;
		}
	}

	/* increment the internal time base */
	this->x += AudioBufferSize;
	return false;
}

/******************************************************************************
 * module information
 */

static const struct port_info in_ports[] = {
	{.name = "x",.type = PORT_TYPE_AUDIO},
	{.name = "y0",.type = PORT_TYPE_AUDIO},
	{.name = "trigger",.type = PORT_TYPE_BOOL,.pf = plot_port_trigger},
	PORT_EOL,
};

const struct module_info view_plot_module = {
	.mname = "view/plot",
	.iname = "plot",
	.in = in_ports,
	.alloc = plot_alloc,
	.free = plot_free,
	.process = plot_process,
};

MODULE_REGISTER(view_plot_module);

/*****************************************************************************/
