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

#ifndef QIPROG_INTERNAL_H
#define QIPROG_INTERNAL_H

#define QIPROG_RETURN_ON_BAD_DEV(dev) do {	\
	if (dev == NULL)			\
		return QIPROG_ERR_ARG;		\
	if (dev->drv == NULL)			\
		return QIPROG_ERR_ARG;		\
	} while (0)

/* FIXME: Hook this variable into the buildsystem */
#define CONFIG_DRIVER_USB_MASTER	1

#if CONFIG_DRIVER_USB_MASTER
#include <qiprog_usb_host.h>
#endif

struct qiprog_context {
#if CONFIG_DRIVER_USB_MASTER
	struct libusb_context *libusb_host_ctx;
#endif
};

#define LIST_STEP 128
struct dev_list {
	size_t len;
	size_t capacity;
	struct qiprog_device **devs;
};

struct qiprog_driver {
	qiprog_err(*init) (struct qiprog_context * ctx);
	qiprog_err(*scan) (struct qiprog_context * ctx, struct dev_list * list);
	qiprog_err(*dev_open) (struct qiprog_device * dev);
	qiprog_err(*get_capabilities) (struct qiprog_device * dev,
				       struct qiprog_capabilities * caps);
};

struct qiprog_device {
	/* Name of device manufacturer, if available */
	const char *manufacturer;
	/* Name of device or product, if available */
	const char *product;
	/* Serial Number of device, if available */
	const char *serial;

	/* Underlying driver */
	struct qiprog_driver *drv;
	/* Per-device, specific context */
	void *priv;
};

/* util.c */
qiprog_err dev_list_init(struct dev_list *list);
qiprog_err dev_list_free(struct dev_list *list);
void dev_list_append(struct dev_list *list, struct qiprog_device *dev);
struct qiprog_device *qiprog_new_device(void);
qiprog_err qiprog_free_device(struct qiprog_device *dev);

#endif				/* QIPROG_INTERNAL_H */
