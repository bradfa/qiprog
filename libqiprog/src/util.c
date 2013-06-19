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
struct qiprog_device *qiprog_new_device(void)
{
	return malloc(sizeof(struct qiprog_device));
}

/**
 * @brief Free memory for a new qiprog_device
 */
qiprog_err qiprog_free_device(struct qiprog_device *dev)
{
	free(dev);
}

/** @} */
