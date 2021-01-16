/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Block Operations
 * Note: Unrolling these loops and breaking up the operations tends to speed them
 * up because it gives the compiler a chance to pipeline loads and stores.
 * I haven't tried anything fancy with the order of operations within the loop
 * because the compiler will re-order them as it sees fit.
 *
 * The block_mul/add() function seem immune to improvements. They use vldmia/vstmia
 * and it maybe that other functions could benefit from multiple load/store also. *
 */

#include "ggm.h"

/*****************************************************************************/

/* block_zero sets a buffer to zero */
void block_zero(float *out)
{
	for (size_t i = 0; i < AudioBufferSize; i++) {
		out[i] = 0.f;
	}
}

/* block_mul multiplies two buffers */
void block_mul(float *out, float *buf)
{
	for (size_t i = 0; i < AudioBufferSize; i++) {
		out[i] *= buf[i];
	}
}

/* block_add adds two buffers */
void block_add(float *out, float *buf)
{
	for (size_t i = 0; i < AudioBufferSize; i++) {
		out[i] += buf[i];
	}
}

/* block_mul_k multiplies a block by a scalar */
void block_mul_k(float *out, float k)
{
	size_t n = AudioBufferSize;

	/* unroll x4 */
	while (n > 0) {
		out[0] *= k;
		out[1] *= k;
		out[2] *= k;
		out[3] *= k;
		out += 4;
		n -= 4;
	}
}

/* block_add_k adds a scalar to a buffer */
void block_add_k(float *out, float k)
{
	size_t n = AudioBufferSize;

	/* unroll x4 */
	while (n > 0) {
		out[0] += k;
		out[1] += k;
		out[2] += k;
		out[3] += k;
		out += 4;
		n -= 4;
	}
}

/* block_copy copies a block */
void block_copy(float *dst, const float *src)
{
	size_t n = AudioBufferSize;

	/* unroll x4 */
	while (n > 0) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		src += 4;
		dst += 4;
		n -= 4;
	}
}

/* block_copy_mul_k copies a block and multiplies by k */
void block_copy_mul_k(float *dst, const float *src, float k)
{
	size_t n = AudioBufferSize;

	/* unroll x4 */
	while (n > 0) {
		dst[0] = src[0] * k;
		dst[1] = src[1] * k;
		dst[2] = src[2] * k;
		dst[3] = src[3] * k;
		src += 4;
		dst += 4;
		n -= 4;
	}
}

/*****************************************************************************/
