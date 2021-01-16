/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Utility Functions
 */

#include "ggm.h"

/******************************************************************************
 * map_lin returns a linear mapping from x = 0..1 to y = y0..y1
 */

float map_lin(float x, float y0, float y1)
{
	return ((y1 - y0) * x) + y0;
}

/******************************************************************************
 * map_exp returns an exponential mapping from x = 0..1 to y = y0..y1
 * k < 0 and y1 > y0 gives y'' < 0 (downwards curve)
 * k > 0 and y1 > y0 gives y'' > 0 (upwards curve)
 * k != 0 and abs(k) is typically 3..5
 */

float map_exp(float x, float y0, float y1, float k)
{
	if (k == 0) {
		LOG_ERR("k == 0, use map_lin");
		return map_lin(x, y0, y1);
	}
	float a = (y0 - y1) / (1.f - pow2(k));
	float b = y0 - a;
	return (a * pow2(k * x)) + b;
}

/******************************************************************************
 * match performs basic string matching. The first string can contain wild cards
 * * matches multiple characters
 * ? matches a single character
 */

bool match(const char *first, const char *second)
{
	/* If we reach at the end of both strings, we are done
	 */
	if (*first == '\0' && *second == '\0') {
		return true;
	}

	/* Make sure that the characters after '*' are present
	 * in second string. This function assumes that the first
	 * string will not contain two consecutive '*'
	 */
	if (*first == '*' && *(first + 1) != '\0' && *second == '\0') {
		return false;
	}

	/* If the first string contains '?', or current characters
	 * of both strings match
	 */
	if (*first == '?' || *first == *second) {
		return match(first + 1, second + 1);
	}

	/* If there is *, then there are two possibilities
	 * a) We consider current character of second string
	 * b) We ignore current character of second string.
	 */
	if (*first == '*') {
		return match(first + 1, second) || match(first, second + 1);
	}
	return false;
}

/*****************************************************************************/
