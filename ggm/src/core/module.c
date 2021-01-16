/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * the set of all registered modules
 */

extern struct module_info delay_delay_module;
extern struct module_info env_adsr_module;
extern struct module_info filter_biquad_module;
extern struct module_info filter_svf_module;
extern struct module_info midi_mono_module;
extern struct module_info midi_poly_module;
extern struct module_info mix_pan_module;
extern struct module_info osc_goom_module;
extern struct module_info osc_ks_module;
extern struct module_info osc_lfo_module;
extern struct module_info osc_noise_module;
extern struct module_info osc_sine_module;
extern struct module_info pm_breath_module;
extern struct module_info root_metro_module;
extern struct module_info root_poly_module;
extern struct module_info seq_seq_module;
extern struct module_info seq_smf_module;
extern struct module_info voice_goom_module;
extern struct module_info voice_osc_module;
#if defined(__LINUX__)
extern struct module_info view_plot_module;
#endif

/* module_list is a list off all the system modules */
static const struct module_info *module_list[] = {
	&delay_delay_module,
	&env_adsr_module,
	&filter_biquad_module,
	&filter_svf_module,
	&midi_mono_module,
	&midi_poly_module,
	&mix_pan_module,
	&osc_goom_module,
	&osc_ks_module,
	&osc_lfo_module,
	&osc_noise_module,
	&osc_sine_module,
	&pm_breath_module,
	&root_metro_module,
	&root_poly_module,
	&seq_seq_module,
	&seq_smf_module,
	&voice_goom_module,
	&voice_osc_module,
#if defined(__LINUX__)
	&view_plot_module,
#endif
	NULL,
};

/* module_find finds a module by name */
static const struct module_info *module_find(const char *name) {
	const struct module_info *mi;
	int i = 0;

	while ((mi = module_list[i]) != NULL) {
		if (strcmp(mi->mname, name) == 0) {
			return mi;
		}
		i++;
	}
	return NULL;
}

/******************************************************************************
 * module functions
 */

/* module_name constructs the full path name of the module */
static char *module_name(struct module *p, const char *iname, int id) {
	char name[128];

	if (p == NULL) {
		/* this is the root module */
		if (id >= 0) {
			snprintf(name, sizeof(name), "%s%d", iname, id);
		} else {
			snprintf(name, sizeof(name), "%s", iname);
		}
	} else {
		/* this is a child module */
		if (id >= 0) {
			snprintf(name, sizeof(name), "%s.%s%d", p->name, iname, id);
		} else {
			snprintf(name, sizeof(name), "%s.%s", p->name, iname);
		}
	}
	/* copy the name string into an allocated buffer */
	size_t n = strlen(name);
	char *s = ggm_calloc(n + 1, sizeof(char));
	if (s == NULL) {
		return NULL;
	}
	return strncpy(s, name, n);
}

/* module_create creates a module */
static struct module *module_create(struct synth *s, struct module *p, const char *name, int id, va_list vargs) {
	/* find the module */
	const struct module_info *mi = module_find(name);

	if (mi == NULL) {
		LOG_ERR("could not find module %s", name);
		return NULL;
	}

	/* allocate the module */
	struct module *m = ggm_calloc(1, sizeof(struct module));
	if (m == NULL) {
		goto error;
	}

	/* fill in the module data */
	m->info = mi;
	m->id = id;
	m->name = module_name(p, mi->iname, m->id);
	m->parent = p;
	m->top = s;

	LOG_INF("%s", m->name);

	/* allocate link list headers for the output port destinations */
	int n = port_count(mi->out);
	if (n > 0) {
		struct output_dst **dst = ggm_calloc(n, sizeof(void *));
		if (dst == NULL) {
			goto error;
		}
		m->dst = dst;
	}

	/* allocate and initialise the module private data */
	int err = mi->alloc(m, vargs);
	if (err != 0) {
		goto error;
	}

	/* iterate across input ports to setup MIDI mappings
	 * and set default values.
	 */
	if (mi->in != NULL) {
		const struct port_info *port = mi->in;
		int i = 0;
		while (port[i].type != PORT_TYPE_NULL) {
			synth_input_cfg(m->top, m, &port[i]);
			i++;
		}
	}

	return m;

 error:
	LOG_ERR("could not create module %s", name);
	if (m != NULL) {
		ggm_free(m->dst);
		ggm_free((void *)m->name);
		ggm_free(m);
	}
	return NULL;
}

/******************************************************************************
 * functions to create/delete  modules
 */

/* module_root creates an instance of a root module */
struct module *module_root(struct synth *top, const char *name, int id, ...) {
	va_list vargs;

	va_start(vargs, id);
	struct module *m = module_create(top, NULL, name, id, vargs);
	va_end(vargs);
	return m;
}

/* module_new returns a new instance of a module. */
struct module *module_new(struct module *parent, const char *name, int id, ...) {
	va_list vargs;

	va_start(vargs, id);
	struct module *m = module_create(parent->top, parent, name, id, vargs);
	va_end(vargs);
	return m;
}

/* module_del deallocates a module and it's sub-modules */
void module_del(struct module *m) {
	if (m == NULL) {
		return;
	}

	LOG_INF("%s", m->name);

	/* free the sub-modules and private data */
	m->info->free(m);

	/* deallocate the lists of output destinations */
	int n = port_count(m->info->out);
	for (int i = 0; i < n; i++) {
		port_free_dst_list(m->dst[i]);
	}

	ggm_free(m->dst);
	ggm_free((void *)m->name);
	ggm_free(m);
}

/*****************************************************************************/
