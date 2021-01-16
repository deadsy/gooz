/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_MODULE_OSC_OSC_H
#define GGM_SRC_MODULE_OSC_OSC_H

/******************************************************************************
 * LFO wave shapes
 */

enum {
	LFO_SHAPE_NULL,
	LFO_SHAPE_TRIANGLE,	/* triangle */
	LFO_SHAPE_SAWDOWN,	/* falling sawtooth */
	LFO_SHAPE_SAWUP,	/* rising sawtooth */
	LFO_SHAPE_SQUARE,	/* square */
	LFO_SHAPE_SINE,		/* sine */
	LFO_SHAPE_SAMPLEANDHOLD,	/* random sample and hold */
	LFO_SHAPE_MAX		/* must be last */
};

/******************************************************************************
 * noise types
 */

enum {
	NOISE_TYPE_NULL,
	NOISE_TYPE_WHITE,	/* white noise */
	NOISE_TYPE_BROWN,	/* brown noise */
	NOISE_TYPE_PINK1,	/* pink noise - low quality */
	NOISE_TYPE_PINK2,	/* pink noise - higher quality */
	NOISE_TYPE_MAX		/* must be last */
};

/*****************************************************************************/

#endif				/* GGM_SRC_MODULE_OSC_OSC_H */

/*****************************************************************************/
