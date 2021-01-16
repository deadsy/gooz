/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * LUT based cosine function - generated by ./tools/lut.py
 */

#define COS_LUT_BITS (7U)
#define COS_LUT_SIZE (1U << COS_LUT_BITS)

static const float COS_LUT_data[COS_LUT_SIZE << 1] = {
	1.000000e+00, -1.204544e-03, 9.987955e-01, -3.610730e-03, 9.951847e-01, -6.008217e-03, 9.891765e-01, -8.391230e-03,
	9.807853e-01, -1.075403e-02, 9.700313e-01, -1.309092e-02, 9.569403e-01, -1.539627e-02, 9.415441e-01, -1.766453e-02,
	9.238795e-01, -1.989024e-02, 9.039893e-01, -2.206803e-02, 8.819213e-01, -2.419265e-02, 8.577286e-01, -2.625900e-02,
	8.314696e-01, -2.826208e-02, 8.032075e-01, -3.019708e-02, 7.730105e-01, -3.205933e-02, 7.409511e-01, -3.384434e-02,
	7.071068e-01, -3.554783e-02, 6.715590e-01, -3.716567e-02, 6.343933e-01, -3.869398e-02, 5.956993e-01, -4.012907e-02,
	5.555702e-01, -4.146749e-02, 5.141027e-01, -4.270601e-02, 4.713967e-01, -4.384164e-02, 4.275551e-01, -4.487166e-02,
	3.826834e-01, -4.579358e-02, 3.368899e-01, -4.660518e-02, 2.902847e-01, -4.730450e-02, 2.429802e-01, -4.788986e-02,
	1.950903e-01, -4.835985e-02, 1.467305e-01, -4.871333e-02, 9.801714e-02, -4.894947e-02, 4.906767e-02, -4.906767e-02,
	6.123234e-17, -4.906767e-02, -4.906767e-02, -4.894947e-02, -9.801714e-02, -4.871333e-02, -1.467305e-01, -4.835985e-02,
	-1.950903e-01, -4.788986e-02, -2.429802e-01, -4.730450e-02, -2.902847e-01, -4.660518e-02, -3.368899e-01, -4.579358e-02,
	-3.826834e-01, -4.487166e-02, -4.275551e-01, -4.384164e-02, -4.713967e-01, -4.270601e-02, -5.141027e-01, -4.146749e-02,
	-5.555702e-01, -4.012907e-02, -5.956993e-01, -3.869398e-02, -6.343933e-01, -3.716567e-02, -6.715590e-01, -3.554783e-02,
	-7.071068e-01, -3.384434e-02, -7.409511e-01, -3.205933e-02, -7.730105e-01, -3.019708e-02, -8.032075e-01, -2.826208e-02,
	-8.314696e-01, -2.625900e-02, -8.577286e-01, -2.419265e-02, -8.819213e-01, -2.206803e-02, -9.039893e-01, -1.989024e-02,
	-9.238795e-01, -1.766453e-02, -9.415441e-01, -1.539627e-02, -9.569403e-01, -1.309092e-02, -9.700313e-01, -1.075403e-02,
	-9.807853e-01, -8.391230e-03, -9.891765e-01, -6.008217e-03, -9.951847e-01, -3.610730e-03, -9.987955e-01, -1.204544e-03,
	-1.000000e+00, 1.204544e-03, -9.987955e-01, 3.610730e-03, -9.951847e-01, 6.008217e-03, -9.891765e-01, 8.391230e-03,
	-9.807853e-01, 1.075403e-02, -9.700313e-01, 1.309092e-02, -9.569403e-01, 1.539627e-02, -9.415441e-01, 1.766453e-02,
	-9.238795e-01, 1.989024e-02, -9.039893e-01, 2.206803e-02, -8.819213e-01, 2.419265e-02, -8.577286e-01, 2.625900e-02,
	-8.314696e-01, 2.826208e-02, -8.032075e-01, 3.019708e-02, -7.730105e-01, 3.205933e-02, -7.409511e-01, 3.384434e-02,
	-7.071068e-01, 3.554783e-02, -6.715590e-01, 3.716567e-02, -6.343933e-01, 3.869398e-02, -5.956993e-01, 4.012907e-02,
	-5.555702e-01, 4.146749e-02, -5.141027e-01, 4.270601e-02, -4.713967e-01, 4.384164e-02, -4.275551e-01, 4.487166e-02,
	-3.826834e-01, 4.579358e-02, -3.368899e-01, 4.660518e-02, -2.902847e-01, 4.730450e-02, -2.429802e-01, 4.788986e-02,
	-1.950903e-01, 4.835985e-02, -1.467305e-01, 4.871333e-02, -9.801714e-02, 4.894947e-02, -4.906767e-02, 4.906767e-02,
	-1.836970e-16, 4.906767e-02, 4.906767e-02, 4.894947e-02, 9.801714e-02, 4.871333e-02, 1.467305e-01, 4.835985e-02,
	1.950903e-01, 4.788986e-02, 2.429802e-01, 4.730450e-02, 2.902847e-01, 4.660518e-02, 3.368899e-01, 4.579358e-02,
	3.826834e-01, 4.487166e-02, 4.275551e-01, 4.384164e-02, 4.713967e-01, 4.270601e-02, 5.141027e-01, 4.146749e-02,
	5.555702e-01, 4.012907e-02, 5.956993e-01, 3.869398e-02, 6.343933e-01, 3.716567e-02, 6.715590e-01, 3.554783e-02,
	7.071068e-01, 3.384434e-02, 7.409511e-01, 3.205933e-02, 7.730105e-01, 3.019708e-02, 8.032075e-01, 2.826208e-02,
	8.314696e-01, 2.625900e-02, 8.577286e-01, 2.419265e-02, 8.819213e-01, 2.206803e-02, 9.039893e-01, 1.989024e-02,
	9.238795e-01, 1.766453e-02, 9.415441e-01, 1.539627e-02, 9.569403e-01, 1.309092e-02, 9.700313e-01, 1.075403e-02,
	9.807853e-01, 8.391230e-03, 9.891765e-01, 6.008217e-03, 9.951847e-01, 3.610730e-03, 9.987955e-01, 1.204544e-03,
};

#define FRAC_BITS (32U - COS_LUT_BITS)
#define FRAC_MASK ((1U << FRAC_BITS) - 1)
#define FRAC_SCALE (1.f / (float)FRAC_MASK)

float cos_lookup(uint32_t x) {
	uint32_t idx = (x >> FRAC_BITS) << 1;
	float frac = (x & FRAC_MASK) * FRAC_SCALE;
	float y = COS_LUT_data[idx];
	float dy = COS_LUT_data[idx + 1];

	return y + (dy * frac);
}

/******************************************************************************
 * LUT based exponential functions - generated by ./tools/exp.py
 */

static const uint16_t exp0_table[64] = {
	0x8000, 0x8165, 0x82ce, 0x843a, 0x85ab, 0x871f, 0x8898, 0x8a15, 0x8b96, 0x8d1b, 0x8ea4, 0x9032, 0x91c4, 0x935a, 0x94f5, 0x9694,
	0x9838, 0x99e0, 0x9b8d, 0x9d3f, 0x9ef5, 0xa0b0, 0xa270, 0xa435, 0xa5ff, 0xa7ce, 0xa9a1, 0xab7a, 0xad58, 0xaf3b, 0xb124, 0xb312,
	0xb505, 0xb6fe, 0xb8fc, 0xbaff, 0xbd09, 0xbf18, 0xc12c, 0xc347, 0xc567, 0xc78d, 0xc9ba, 0xcbec, 0xce25, 0xd063, 0xd2a8, 0xd4f3,
	0xd745, 0xd99d, 0xdbfc, 0xde61, 0xe0cd, 0xe340, 0xe5b9, 0xe839, 0xeac1, 0xed4f, 0xefe5, 0xf281, 0xf525, 0xf7d1, 0xfa84, 0xfd3e,
};

static const uint16_t exp1_table[64] = {
	0x8000, 0x8006, 0x800b, 0x8011, 0x8016, 0x801c, 0x8021, 0x8027, 0x802c, 0x8032, 0x8037, 0x803d, 0x8043, 0x8048, 0x804e, 0x8053,
	0x8059, 0x805e, 0x8064, 0x806a, 0x806f, 0x8075, 0x807a, 0x8080, 0x8085, 0x808b, 0x8090, 0x8096, 0x809c, 0x80a1, 0x80a7, 0x80ac,
	0x80b2, 0x80b8, 0x80bd, 0x80c3, 0x80c8, 0x80ce, 0x80d3, 0x80d9, 0x80df, 0x80e4, 0x80ea, 0x80ef, 0x80f5, 0x80fa, 0x8100, 0x8106,
	0x810b, 0x8111, 0x8116, 0x811c, 0x8122, 0x8127, 0x812d, 0x8132, 0x8138, 0x813e, 0x8143, 0x8149, 0x814e, 0x8154, 0x815a, 0x815f,
};

/* pow2_int returns powf(2.f, x) where x is an integer [-126,127] */
static float pow2_int(int x) {
	union {
		unsigned int ui;
		float f;
	} val;

	/* make a float per IEEE754 */
	val.ui = (127 + x) << 23;
	return val.f;
}

/* pow2_frac returns powf(2.f, x) where x = [0,1) */
static float pow2_frac(float x) {
	int n = (int)(x * (float)(1U << 12));
	uint16_t x0 = exp0_table[(n >> 6) & 0x3f];
	uint16_t x1 = exp1_table[n & 0x3f];

	return (float)(x0 * x1) * (1.f / (float)(1U << 30));
}

/* pow2 returns powf(2.f, x) */
float pow2(float x) {
	float nf = truncf(x);
	float ff = x - nf;

	if (ff < 0) {
		nf -= 1.f;
		ff += 1.f;
	}
	return pow2_frac(ff) * pow2_int((int)nf);
}

/*****************************************************************************/
