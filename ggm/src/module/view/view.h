/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_MODULE_VIEW_VIEW_H
#define GGM_SRC_MODULE_VIEW_VIEW_H

/******************************************************************************
 * Plot Configuration
 */

struct plot_cfg {
	const char *name;       /* name of output python file */
	const char *title;      /* title of plot */
	const char *x_name;     /* x-axis name */
	const char *y0_name;    /* y0-axis name */
	float duration;         /* sample time in seconds */
};

/*****************************************************************************/

#endif /* GGM_SRC_MODULE_VIEW_VIEW_H */

/*****************************************************************************/
