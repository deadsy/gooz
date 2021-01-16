/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * event_in sends an event to a named port on a module.
 */

void event_in(struct module *m, const char *name, const struct event *e, port_func *hdl)
{
	const struct module_info *mi = m->info;
	port_func func = NULL;

	if (hdl == NULL || *hdl == NULL) {
		/* find the port function */
		int i = 0;
		while (1) {
			const struct port_info *p = &mi->in[i];
			if (p->type == PORT_TYPE_NULL) {
				/* end of list */
				break;
			}
			/* port name found ?*/
			if (strcmp(name, p->name) == 0) {
				func = p->pf;
				break;
			}
			/*next ...*/
			i++;
		}
	} else {
		/* use the cached port function */
		func = *hdl;
	}

	if (func == NULL) {
		LOG_WRN("%s:%s not found", m->name, name);
		return;
	}

	/* cache the function pointer */
	if (hdl && *hdl == NULL) {
		*hdl = func;
	}

	/* call the port function */
	func(m, e);
}

/******************************************************************************
 * event_out sends an event-time event from the output port of a module.
 * The event will be sent to the ports connected to the output port.
 */

void event_out(struct module *m, int idx, const struct event *e)
{
	struct output_dst *ptr = m->dst[idx];

	/* iterate over the event destinations */
	while (ptr != NULL) {
		struct output_dst *next = ptr->next;
		/* call the port function for the event destination */
		ptr->func(ptr->m, e);
		ptr = next;
	}
}

/* event_out_name calls event_out on a named port */
void event_out_name(struct module *m, const char *name, const struct event *e)
{
	/* get the index of the output port */
	int idx = port_get_index(m->info->out, name);

	if (idx < 0) {
		LOG_ERR("%s does not have output port %s", m->name, name);
		return;
	}
	event_out(m, idx, e);
}

/******************************************************************************
 * event_push sends a process time event from the output port of a module.
 * At event time the event will be sent to the ports connected to the output
 * port.
 */

void event_push(struct module *m, int idx, const struct event *e)
{
	/* queue the event for later processing */
	int rc = synth_event_wr(m->top, m, idx, e);

	if (rc != 0) {
		LOG_ERR("event queue overflow");
	}
}

/* push to a named port */
void event_push_name(struct module *m, const char *name, const struct event *e)
{
	/* get the index of the output port */
	int idx = port_get_index(m->info->out, name);

	if (idx < 0) {
		LOG_ERR("%s does not have output port %s", m->name, name);
		return;
	}
	event_push(m, idx, e);
}

/*****************************************************************************/



