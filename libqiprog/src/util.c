/*
 * qiprog - Reference implementation of the QiProg protocol
 *
 * Copyright (C) 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @defgroup util_file QiProg Utilities
 *
 * @ingroup qiprog_private
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg miscellaneous utilities</b>
 *
 * This file contains internal utilities used in various places in libqiprog.
 */

/** @{ */

#include <qiprog.h>
#include "qiprog_internal.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize a qiprog_device list
 */
qiprog_err dev_list_init(struct dev_list *list)
{
	memset(list, 0, sizeof(*list));

	list->devs = malloc(LIST_STEP * sizeof(*(list->devs)));
	if (list->devs == NULL)
		return QIPROG_ERR_MALLOC;

	list->capacity = LIST_STEP;
	return QIPROG_SUCCESS;
}

/**
 * @brief Free a non-empty qiprog_device list
 */
qiprog_err dev_list_free(struct dev_list *list)
{
	if ((list->capacity == 0) || (list->devs == 0))
		return QIPROG_ERR_ARG;

	free(list->devs);
	return QIPROG_SUCCESS;
}

/**
 * @brief Add a qiprog_device to the list
 */
void dev_list_append(struct dev_list *list, struct qiprog_device *dev)
{
	void *temp;
	if (list->len == list->capacity) {
		size_t newsize;
		newsize = (list->capacity + LIST_STEP) * sizeof(*(list->devs));
		temp = realloc(list->devs, newsize);
		if (temp == NULL) {
			/* realloc failure: out of RAM */
			return;
		}
		list->devs = temp;
		list->capacity += LIST_STEP;
	}

	list->devs[list->len++] = dev;
}

/**
 * @brief Allocate memory for a new qiprog_device
 */
struct qiprog_device *qiprog_new_device(struct qiprog_context *ctx)
{
	struct qiprog_device *dev;

	if ((dev = malloc(sizeof(*dev))) == NULL) {
		/* FIXME: print error */
		return NULL;
	}

	/*
	 * Although we shouldn't depend on all fields being NULL, it is still
	 * preferable to initialize all fields to 0.
	 */
	memset(dev, 0, sizeof(*dev));

	/* Store the context associated with the device */
	dev->ctx = ctx;

	return dev;
}

/**
 * @brief Free memory for a new qiprog_device
 */
qiprog_err qiprog_free_device(struct qiprog_device *dev)
{
	free(dev);
	return QIPROG_SUCCESS;
}

/*
 * Logging helpers:
 */
/* Log nothing by default */
static enum qiprog_log_level loglevel = QIPROG_LOG_NONE;

/**
 * @ingroup initialization
 */
/** @{ */
/**
 * @brief Set the verbosity of messages printed by libqiprog
 *
 * By default, libqiprog will not print any messages. This behavior can be
 * altered by changing the default loglevel.
 *
 * QIPROG_LOG_WARN or QIPROG_LOG_INFO are recommended for debugging
 * applications. More verbose levels are designed to be used when debugging
 * libqiprog itself.
 *
 * @param[in] level the message verbosity level to use
 */
void qiprog_set_loglevel(enum qiprog_log_level level)
{
	loglevel = level;
}
/** @} */

/**
 * @brief Log messages based on their severity
 *
 * Log to stdout by default
 * TODO: have configurable logfiles?
 */
void qi_log(enum qiprog_log_level level, const char *fmt, ...)
{
	va_list args;

	/* Passing QIPROG_LOG_NONE for level will not force us to log */
	if (loglevel == QIPROG_LOG_NONE)
		return;

	if (level > loglevel)
		return;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fprintf(stdout, "\n");

}

/** @} */
