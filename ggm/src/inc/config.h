/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module Configuration
 */

#ifndef GGM_SRC_INC_CONFIG_H
#define GGM_SRC_INC_CONFIG_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * Module Configuration
 */

struct synth_cfg {
	const char *path;	/* path name to module or module:port */
	const void *cfg;	/* pointer to item config structure */
};

#define SYNTH_CFG_EOL { NULL, NULL }

/******************************************************************************
 * Port Configuration
 */

/* port_float_cfg defines the configuration for a port with float events */
struct port_float_cfg {
	float init;		/* initial value */
	int id;			/* MIDI channel/cc id */
};

struct port_int_cfg {
	int init;		/* initial value */
	int id;			/* MIDI channel/cc id */
};

struct port_bool_cfg {
	bool init;		/* initial value */
	int id;			/* MIDI channel/cc id */
};

/* MIDI_CC encodes the MIDI channel and CC number as an integer.
 * A value of 0 means MIDI CC is not configured for the port.
 */
#define MIDI_ID(ch, cc) (((ch) << 16) | ((cc) << 8) | 255)
#define MIDI_ID_CC(id) ((id >> 8) & 255)
#define MIDI_ID_CH(id) ((id >> 16) & 255)

/*****************************************************************************/

#endif				/* GGM_SRC_INC_CONFIG_H */

/*****************************************************************************/
