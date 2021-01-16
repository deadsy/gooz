/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_INC_PORT_H
#define GGM_SRC_INC_PORT_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * module ports
 */

enum port_type {
	PORT_TYPE_NULL = 0,
	PORT_TYPE_AUDIO,                /* audio buffers */
	PORT_TYPE_FLOAT,                /* event with float values */
	PORT_TYPE_INT,                  /* event with integer values */
	PORT_TYPE_BOOL,                 /* event with boolean values */
	PORT_TYPE_MIDI,                 /* event with MIDI data */
};

/* port_info contains the information describing a port. The information is
 * defined by code and is known at compile time. The information is constant
 * at runtime, so the structure can be stored in read-only memory.
 */
struct port_info {
	const char *name;       /* port name */
	enum port_type type;    /* port type */
	port_func pf;           /* port event function */
	midi_func mf;           /* MIDI event conversion function */
};

#define PORT_EOL { NULL, PORT_TYPE_NULL, NULL, NULL }

/******************************************************************************
 * output port destinations: An event sent from an output port is delivered
 * to input ports on other modules or is used as an output event on the output
 * port of another module. The connectivity is constructed at runtime, so for
 * each output port of a module we maintain a linked list of event destinations.
 */

struct output_dst {
	struct output_dst *next;        /* next destination */
	struct module *m;               /* destination module */
	port_func func;                 /* port function to call */
};

/******************************************************************************
 * function prototypes
 */

unsigned int port_count(const struct port_info port[]);
unsigned int port_count_by_type(const struct port_info port[], enum port_type type);

int port_get_index(const struct port_info port[], const char *name);
const struct port_info *port_get_info(const struct port_info port[], const char *name);

int port_get_index_by_type(const struct port_info port[], enum port_type type, size_t n);
const struct port_info *port_get_info_by_type(const struct port_info port[], enum port_type type, size_t n);

void port_add_dst(struct module *m, int idx, struct module *dst, port_func func);
void port_free_dst_list(struct output_dst *ptr);

void port_connect(struct module *s, const char *sname, struct module *d, const char *dname);
void port_forward(struct module *s, const char *sname, struct module *d, const char *dname);

/*****************************************************************************/

#endif /* GGM_SRC_INC_PORT_H */

/*****************************************************************************/
