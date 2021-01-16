/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_INC_GGM_H
#define GGM_SRC_INC_GGM_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>

#include "osal.h"

#include "const.h"
#include "util.h"
#include "module.h"
#include "event.h"
#include "port.h"
#include "config.h"
#include "synth.h"

/******************************************************************************
 * version
 */

#define GGM_VERSION "0.1"

/******************************************************************************
 * Lookup Table Functions
 */

float cos_lookup(uint32_t x);
float pow2(float x);

/******************************************************************************
 * 32-bit float math functions.
 */

float truncf(float x);
float fabsf(float x);

int isinff(float x);
int isnanf(float x);

float cosf(float x);
float sinf(float x);
float tanf(float x);

float powe(float x);

/******************************************************************************
 * MIDI
 */

float midi_to_frequency(float note);
float midi_pitch_bend(uint16_t val);

/******************************************************************************
 * block operations
 */

void block_zero(float *out);
void block_mul(float *out, float *buf);
void block_mul_k(float *out, float k);
void block_add(float *out, float *buf);
void block_add_k(float *out, float k);
void block_copy(float *dst, const float *src);
void block_copy_mul_k(float *dst, const float *src, float k);

/*****************************************************************************/

#endif /* GGM_SRC_INC_GGM_H */

/*****************************************************************************/
