/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * port counting functions
 */

/* port_count returns the number of ports */
unsigned int port_count(const struct port_info port[])
{
	unsigned int i = 0;

	if (port == NULL) {
		return 0;
	}
	while (port[i].type != PORT_TYPE_NULL) {
		i++;
	}
	return i;
}

/* port_count_by_type returns the number of ports of a given type */
unsigned int port_count_by_type(const struct port_info port[], enum port_type type)
{
	int i = 0;
	unsigned int n = 0;

	if (port == NULL) {
		return 0;
	}
	while (port[i].type != PORT_TYPE_NULL) {
		if (port[i].type == type) {
			n++;
		}
		i++;
	}
	return n;
}

/******************************************************************************
 * port lookup functions
 */

/* port_get_index returns the array index of a named port */
int port_get_index(
	const struct port_info port[],
	const char *name
	)
{
	int i = 0;

	while (port[i].type != PORT_TYPE_NULL) {
		if (strcmp(name, port[i].name) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/* port_get_info returns the port info of a named port */
const struct port_info *port_get_info(
	const struct port_info port[],
	const char *name
	)
{
	int i = port_get_index(port, name);

	return (i >= 0) ? &port[i] : NULL;
}

/* port_get_index_by_type gets the index of the n-th port of a given type */
int port_get_index_by_type(
	const struct port_info port[],
	enum port_type type,
	size_t n)
{
	unsigned int k = 0;
	int i = 0;

	while (port[i].type != PORT_TYPE_NULL) {
		if (port[i].type == type) {
			if (k == n) {
				return i;
			}
			k++;
		}
		i++;
	}
	return -1;
}

/* port_get_info_by_type returns the port info of the n-th port of a given type */
const struct port_info *port_get_info_by_type(
	const struct port_info port[],
	enum port_type type,
	size_t n
	)
{
	int i = port_get_index_by_type(port, type, n);

	return (i >= 0) ? &port[i] : NULL;
}

/******************************************************************************
 * port_fwd_X is the port function used when an output port has
 * been connected to another output port. It will call the event_out
 * function from the destination port therby forwarding the event.
 * X is the port index of the output port of the destination module.
 */

#define NUM_PORT_FWD 8

static void port_fwd_0(struct module *m, const struct event *e)
{
	event_out(m, 0, e);
}

static void port_fwd_1(struct module *m, const struct event *e)
{
	event_out(m, 1, e);
}

static void port_fwd_2(struct module *m, const struct event *e)
{
	event_out(m, 2, e);
}

static void port_fwd_3(struct module *m, const struct event *e)
{
	event_out(m, 3, e);
}

static void port_fwd_4(struct module *m, const struct event *e)
{
	event_out(m, 4, e);
}

static void port_fwd_5(struct module *m, const struct event *e)
{
	event_out(m, 5, e);
}

static void port_fwd_6(struct module *m, const struct event *e)
{
	event_out(m, 6, e);
}

static void port_fwd_7(struct module *m, const struct event *e)
{
	event_out(m, 7, e);
}

static port_func port_fwd[NUM_PORT_FWD] = {
	port_fwd_0,
	port_fwd_1,
	port_fwd_2,
	port_fwd_3,
	port_fwd_4,
	port_fwd_5,
	port_fwd_6,
	port_fwd_7,
};

/******************************************************************************
 * output destination list functions
 */

/* port_add_dst adds a destination port to the output */
void port_add_dst(struct module *m, int idx, struct module *dst, port_func func)
{
	/* allocate the output destination */
	struct output_dst *x = ggm_calloc(1, sizeof(struct output_dst));

	if (x == NULL) {
		LOG_ERR("unable to allocate output destination list element");
		return;
	}

	/* fill in the output destination */
	x->m = dst;
	x->func = func;

	/* add the output destination to the head of the list */
	struct output_dst *head = m->dst[idx];
	if (head != NULL) {
		x->next = head;
	}
	m->dst[idx] = x;
}

/* port_free_dst_list frees a list of output destination elements */
void port_free_dst_list(struct output_dst *ptr)
{
	while (ptr != NULL) {
		struct output_dst *next = ptr->next;
		ggm_free(ptr);
		ptr = next;
	}
}

/******************************************************************************
 * output port connection functions
 */

/* port_connect connects an output port to an input port */
void port_connect(struct module *s, const char *sname, struct module *d, const char *dname)
{
	const struct module_info *si = s->info;
	const struct module_info *di = d->info;

	LOG_INF("%s:%s to %s:%s", s->name, sname,  d->name, dname);

	/* does the output port exist on the source module? */
	int s_idx = port_get_index(si->out, sname);
	if (s_idx < 0) {
		LOG_ERR("output port does not exist");
		return;
	}

	/* does the input port exist on the destination module? */
	int d_idx = port_get_index(di->in, dname);
	if (d_idx < 0) {
		LOG_ERR("input port does not exist");
		return;
	}

	/* do the port type match? */
	int s_type = si->out[s_idx].type;
	int d_type = di->in[d_idx].type;
	if (s_type != d_type) {
		LOG_ERR("port types must be the same");
		return;
	}

	/* make sure we are not trying to connect audio ports */
	if (s_type == PORT_TYPE_AUDIO) {
		LOG_ERR("ports must be event ports (not audio)");
		return;
	}

	/* check for a non-null destination port function */
	port_func d_pf = di->in[d_idx].pf;
	if (d_pf == NULL) {
		LOG_ERR("input port must have a port function");
		return;
	}

	port_add_dst(s, s_idx, d, d_pf);
}

/* port_forward connects an output port to an output port for event forwarding */
void port_forward(struct module *s, const char *sname, struct module *d, const char *dname)
{
	const struct module_info *si = s->info;
	const struct module_info *di = d->info;

	LOG_INF("%s:%s to %s:%s", s->name, sname,  d->name, dname);

	/* does the output port exist on the source module? */
	int s_idx = port_get_index(si->out, sname);
	if (s_idx < 0) {
		LOG_ERR("output port does not exist");
		return;
	}

	/* does the output port exist on the destination module? */
	int d_idx = port_get_index(di->out, dname);
	if (d_idx < 0) {
		LOG_ERR("output port does not exist");
		return;
	}

	/* do the port type match? */
	int s_type = si->out[s_idx].type;
	int d_type = di->out[d_idx].type;
	if (s_type != d_type) {
		LOG_ERR("port types must be the same");
		return;
	}

	/* make sure we are not trying to connect audio ports */
	if (s_type == PORT_TYPE_AUDIO) {
		LOG_ERR("ports must be event ports (not audio)");
		return;
	}

	/* check the limit on the destination port index */
	if (d_idx >= NUM_PORT_FWD) {
		LOG_ERR("d_idx >= NUM_PORT_FWD (see port.c)");
		return;
	}

	port_add_dst(s, s_idx, d, port_fwd[d_idx]);
}

/*****************************************************************************/
