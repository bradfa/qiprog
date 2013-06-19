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

#include <qiprog.h>
#include "qiprog_internal.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: Kill this idiotic include */
#include <stdio.h>

#if CONFIG_DRIVER_USB_MASTER
extern struct qiprog_driver qiprog_usb_master_drv;
#endif

/* NULL-terminated list of drivers compiled in */
static const struct qiprog_driver *driver_list[] = {
#if CONFIG_DRIVER_USB_MASTER
	&qiprog_usb_master_drv,
#endif
	NULL,
};

/**
 * @defgroup initialization QiProg initialization/deinitialization
 *
 * @ingroup qiprog_api
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg initialization/deinitialization</b>
 *
 */
/** @{ */

/**
 * @brief
 */
qiprog_err qiprog_init(struct qiprog_context **ctx)
{
	struct qiprog_context *context;
	*ctx = 0;

	if ((context = malloc(sizeof(**ctx))) == NULL) {
		/* FIXME: Add some sort console and print a message */
		return QIPROG_ERR_MALLOC;
	}

#if CONFIG_DRIVER_USB_MASTER
	if (libusb_init(&(context->libusb_host_ctx)) != LIBUSB_SUCCESS) {
		/* FIXME: Printable error message */
		return QIPROG_ERR_MALLOC;
	}
#endif

	*ctx = context;
	return QIPROG_SUCCESS;
}

/**
 * @brief
 */
qiprog_err qiprog_exit(struct qiprog_context *ctx)
{
	if (ctx == NULL)
		return QIPROG_ERR_ARG;
#if CONFIG_DRIVER_USB_MASTER
	libusb_exit(ctx->libusb_host_ctx);
#endif
	free(ctx);

	return QIPROG_SUCCESS;
}

/** @} */


/**
 * @defgroup discovery QiProg device discovery and handling
 *
 * @ingroup qiprog_api
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg device discovery and handling</b>
 *
 */
/** @{ */

/**
 * @brief
 */
size_t qiprog_get_device_list(struct qiprog_context *ctx,
			      struct qiprog_device ***list)
{
	size_t i;
	struct qiprog_device **devs;
	const struct qiprog_driver *drv;
	struct dev_list qi_list;

	if (!ctx)
		return 0;

	if (dev_list_init(&qi_list) != QIPROG_SUCCESS) {
		/* FIXME: message here */
		return 0;
	}

	for (i = 0;; i++) {
		drv = driver_list[i];
		if (drv == NULL)
			break;
		if (!drv->scan) {
			/* FIXME: This would be bad. Print something */
			continue;
		}
		drv->scan(ctx, &qi_list);

	}

	*list = qi_list.devs;
	return qi_list.len;
}

/**
 * @brief
 */
qiprog_err qiprog_open_device(struct qiprog_device *dev)
{
	/* FIXME: Check for NULL */
	dev->drv->dev_open(dev);
}

/**
 * @brief
 */
qiprog_err qiprog_get_capabilities(struct qiprog_device *dev,
				   struct qiprog_capabilities *caps)
{
	/* FIXME: Check for NULL */
	dev->drv->get_capabilities(dev, caps);
}

/** @} */
