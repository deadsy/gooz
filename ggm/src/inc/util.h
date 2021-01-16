/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Utility Functions.
 */

#ifndef GGM_SRC_INC_UTIL_H
#define GGM_SRC_INC_UTIL_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * radian/degree conversion
 */

/* d2r converts degrees to radians */
static inline float d2r(float d) {
	return d * (Pi / 180.f);
}

/* r2d converts radians to degrees */
static inline float r2d(float r) {
	return r * (180.f / Pi);
}

/******************************************************************************
 * clamp floating point values
 */

/* clamp x between a and b */
static inline float clampf(float x, float a, float b) {
	if (x < a) {
		return a;
	}
	if (x > b) {
		return b;
	}
	return x;
}

/* clamp x to >= a */
static inline float clampf_lo(float x, float a) {
	return (x < a) ? a : x;
}

/* clamp x to <= a */
static inline float clampf_hi(float x, float a) {
	return (x > a) ? a : x;
}

/******************************************************************************
 * clamp integer values
 */

/* clamp x between a and b */
static inline int clampi(int x, int a, int b) {
	if (x < a) {
		return a;
	}
	if (x > b) {
		return b;
	}
	return x;
}

/******************************************************************************
 * Simple random number generation
 * Based on a linear congruential generator.
 */

/* set an initial value for the random state */
static inline void rand_init(uint32_t seed, uint32_t * state) {
	if (seed == 0) {
		seed = 1;
	}
	*state = seed;
}

/* return a random uint32_t (0..0x7fffffff) */
static inline uint32_t rand_uint32(uint32_t * state) {
	*state = ((*state * 1103515245) + 12345) & 0x7fffffff;
	return *state;
}

/* return a float from -1..1 */
static inline float randf(uint32_t * state) {
	union {
		uint32_t ui;
		float f;
	} val;

	val.ui = (rand_uint32(state) & 0x007fffff) | (128 << 23);	/* 2..4 */
	return val.f - 3.f;	/* -1..1 */
}

/******************************************************************************
 * min/max
 */

static inline int mini(int a, int b) {
	return (a < b) ? a : b;
}

static inline int maxi(int a, int b) {
	return (a > b) ? a : b;
}

/******************************************************************************
 * miscellaneous float routines
 */

/* float2uint return the uint32_t value for the float */
static inline uint32_t float2uint(float x) {
	union {
		uint32_t ui;
		float f;
	} val;

	val.f = x;
	return val.ui;
}

/* zero_cross returns true if a and b are on different sides of 0.f */
static inline bool zero_cross(float a, float b) {
	return ((float2uint(a) ^ float2uint(b)) & (1 << 31)) != 0;
}

/******************************************************************************
 * function prototypes
 */

float map_lin(float x, float y0, float y1);
float map_exp(float x, float y0, float y1, float k);
bool match(const char *first, const char *second);

/*****************************************************************************/

#endif				/* GGM_SRC_INC_UTIL_H */

/*****************************************************************************/
