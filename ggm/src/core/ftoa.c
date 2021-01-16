/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ggm.h"

/******************************************************************************
 * float to string conversion
 * https://blog.benoitblanchon.fr/lightweight-float-to-string/
 */

struct float_parts {
	float val;      /* float value */
	uint32_t i;     /* whole number part */
	uint32_t f;     /* fractional part */
	int32_t e;      /* exponent */
};

#define POS_EXP_THOLD 1e7f
#define NEG_EXP_THOLD 1e-5f

#define POW10_2(x) (1e ## x ## f)
#define POW10(x) POW10_2(x)

#define FRAC_DIGITS 6
#define FRAC_FLOAT (float)(POW10(FRAC_DIGITS))
#define FRAC_UINT  (uint32_t)(POW10(FRAC_DIGITS))

static void normalize_float(struct float_parts *fp)
{
	float val = fp->val;
	int32_t e = 0;

	if (val >= POS_EXP_THOLD) {
		if (val >= 1e32f) {
			val /= 1e32f;
			e += 32;
		}
		if (val >= 1e16f) {
			val /= 1e16f;
			e += 16;
		}
		if (val >= 1e8f) {
			val /= 1e8f;
			e += 8;
		}
		if (val  >= 1e4f) {
			val /= 1e4f;
			e += 4;
		}
		if (val >= 1e2f) {
			val /= 1e2f;
			e += 2;
		}
		if (val >= 1e1f) {
			val /= 1e1f;
			e += 1;
		}
	}

	if (val > 0 && val <= NEG_EXP_THOLD) {
		if (val < 1e-31f) {
			val *= 1e32f;
			e -= 32;
		}
		if (val < 1e-15f) {
			val *= 1e16f;
			e -= 16;
		}
		if (val < 1e-7f) {
			val *= 1e8f;
			e -= 8;
		}
		if (val < 1e-3f) {
			val *= 1e4f;
			e -= 4;
		}
		if (val < 1e-1f) {
			val *= 1e2f;
			e -= 2;
		}
		if (val < 1e0f) {
			val *= 1e1f;
			e -= 1;
		}
	}

	fp->val = val;
	fp->i = 0;
	fp->f = 0;
	fp->e = e;
}

static void split_float(struct float_parts *fp)
{
	normalize_float(fp);

	uint32_t i = (uint32_t)fp->val;
	int32_t e = fp->e;
	float rem = (fp->val - (float)i) * FRAC_FLOAT;
	uint32_t f = (uint32_t)rem;

	/* rounding */
	rem -= f;
	if (rem >= 0.5) {
		f++;
		if (f >= FRAC_UINT) {
			f = 0;
			i++;
			if (e != 0 && i >= 10) {
				e++;
				i = 1;
			}
		}
	}

	fp->i = i;
	fp->f = f;
	fp->e = e;
}

/* str2str copies a string into a buffer */
static int str2str(char *str, char *buf)
{
	int i = 0;

	while (str[i] != 0) {
		buf[i] = str[i];
		i++;
	}
	buf[i] = 0;
	return i;
}

/* int2str creates a decimal number string in a buffer */
static int int2str(uint32_t val, char *buf)
{
	int i = 0;

	/* work out the decimal string */
	do {
		buf[i++] = (val % 10) + '0';
		val /= 10;
	} while (val);
	buf[i] = 0;
	int n = i;
	i -= 1;
	/* reverse the string */
	for (int j = 0; j < i; j++, i--) {
		char tmp = buf[j];
		buf[j] = buf[i];
		buf[i] = tmp;
	}
	return n;
}

/* frac2str creates a fractional decimal string in a buffer */
static int frac2str(uint32_t val, char *buf)
{
	int j = FRAC_DIGITS + 1;

	/* remove trailing zeroes */
	while (val % 10 == 0) {
		val /= 10;
		j--;
	}
	/* null terminate the string, record the length */
	buf[j] = 0;
	int i = j;
	/* write the non-zero fraction digits */
	j--;
	while (val) {
		buf[j--] = (val % 10) + '0';
		val /= 10;
	}
	/* add the leading zeroes */
	while (j > 0) {
		buf[j--] = '0';
	}
	/* add the decimal point */
	buf[0] = '.';
	return i;
}

/* float2str formats a floating point number as a string */
int float2str(float val, char *buf)
{
	int i = 0;

	if (isnanf(val)) {
		i += str2str("nan", &buf[i]);
		return i;
	}

	if (val < 0.f) {
		i += str2str("-", &buf[i]);
		val = -val;
	}

	if (isinff(val)) {
		i += str2str("inf", &buf[i]);
		return i;
	}

	struct float_parts fp;
	fp.val = val;
	split_float(&fp);

	i += int2str(fp.i, &buf[i]);

	if (fp.f) {
		i += frac2str(fp.f, &buf[i]);
	}

	if (fp.e < 0) {
		i += str2str("e-", &buf[i]);
		i += int2str(-fp.e, &buf[i]);
	}

	if (fp.e > 0) {
		i += str2str("e", &buf[i]);
		i += int2str(fp.e, &buf[i]);
	}

	return i;
}

/* ftoa formats a floating point number as a string */
char *ftoa(float val, char *buf)
{
	float2str(val, buf);
	return buf;
}

/*****************************************************************************/
