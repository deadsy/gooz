/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * OS Abstraction Layer
 *
 */

#ifndef GGM_SRC_INC_OSAL_H
#define GGM_SRC_INC_OSAL_H

/*****************************************************************************/
#if defined(__ZEPHYR__)

#include <zephyr.h>
#include <logging/log.h>

/* Display all messages */
#define LOG_LEVEL LOG_LEVEL_DBG

/* Set the module name for log messages */
#define LOG_MODULE_NAME ggm

#ifdef GGM_MAIN
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#else
LOG_MODULE_DECLARE(LOG_MODULE_NAME);
#endif

static inline void ggm_mdelay(long ms)
{
	k_sleep(K_MSEC(ms));
}

static inline void *ggm_calloc(size_t num, size_t size)
{
	return k_calloc(num, size);
}

static inline void ggm_free(void *ptr)
{
	k_free(ptr);
}

/*****************************************************************************/
#elif defined(__LINUX__)

#include <string.h>

#include "linux/log.h"

#define CONFIG_BOARD "linux"

#define LOG_INF log_info
#define LOG_DBG log_debug
#define LOG_ERR log_error
#define LOG_WRN log_warn

static inline const char *log_strdup(const char *s)
{
	return s;
}

void ggm_mdelay(long ms);
void *ggm_calloc(size_t num, size_t size);
void ggm_free(void *ptr);

/*****************************************************************************/

#else
#error "define the OS type for this build"
#endif

/*****************************************************************************/

#endif /* GGM_SRC_INC_OSAL_H */

/*****************************************************************************/
