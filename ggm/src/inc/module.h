/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_INC_MODULE_H
#define GGM_SRC_INC_MODULE_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * module
 */

struct module {
	const struct module_info *info;	/* module info */
	int id;			/* module identifier */
	const char *name;	/* full instance name */
	struct module *parent;	/* parent module */
	struct synth *top;	/* top level synth */
	struct output_dst **dst;	/* output port destinations */
	void *priv;		/* pointer to private module data */
};

/* module_info stores descriptive information common to all module instances of
 * a given type. The information is defined by code and is known at compile time.
 * The information is constant at runtime, so the structure can be stored in
 * read-only memory.
 */
struct module_info {
	const char *mname;	/* module name */
	const char *iname;	/* instance name */
	const struct port_info *in;	/* input ports */
	const struct port_info *out;	/* output ports */
	int (*alloc)(struct module * m, va_list vargs);	/* allocate and initialise the module */
	void (*free)(struct module * m);	/* stop and deallocate the module */
	bool (*process)(struct module * m, float *buf[]);	/* process buffers for this module */
};

typedef struct module *(*module_func) (struct module * m, int id);

#define MODULE_REGISTER(x)

/******************************************************************************
 * function prototypes
 */

struct module *module_root(struct synth *top, const char *name, int id, ...);
struct module *module_new(struct module *parent, const char *name, int id, ...);
void module_del(struct module *m);

/*****************************************************************************/

#endif				/* GGM_SRC_INC_MODULE_H */

/*****************************************************************************/
