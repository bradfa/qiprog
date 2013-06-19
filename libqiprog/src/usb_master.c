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
 * @defgroup usb_master_file QiProg USB host driver
 *
 * @ingroup qiprog_drivers
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg USB host driver</b>
 *
 * This file contains the host-side driver for communicating with USB QiProg
 * devices. The driver serializes QiProg calls to messages over the USB bus.
 */

/** @{ */

#include <qiprog_usb_host.h>
#include "qiprog_internal.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct qiprog_driver qiprog_usb_master_drv;

/**
 * @brief Private per-device context for USB devices
 */
struct usb_master_priv {
	libusb_device_handle *handle;
	libusb_device *usb_dev;
};

/**
 * @brief Helper to create a new USB QiProg device
 */
static struct qiprog_device *new_usb_prog(libusb_device * libusb_dev)
{
	/*
	 * Peter Stuge is the person who started it all. He is also the de-facto
	 * USB expert for free software hackers to go to with questions. As a
	 * result, every QiProg device connected via USB shall be named after
	 * him.
	 */
	struct qiprog_device *peter_stuge;
	struct usb_master_priv *priv;

	peter_stuge = qiprog_new_device();

	if (peter_stuge == NULL)
		return NULL;

	if ((priv = malloc(sizeof(*priv))) == NULL) {
		qiprog_free_device(peter_stuge);
		return NULL;
	}

	peter_stuge->drv = &qiprog_usb_master_drv;
	priv->usb_dev = libusb_dev;
	/* Don't create a handle until the device is opened */
	priv->handle = NULL;
	peter_stuge->priv = priv;

	return peter_stuge;
}

/**
 * @brief QiProg driver 'init' member
 */
static qiprog_err init(struct qiprog_context *ctx)
{
	return QIPROG_SUCCESS;
}

/**
 * @brief Decide if given USB device is a QiProg device
 */
static bool is_interesting(libusb_device * dev)
{
	int ret;
	struct libusb_device_descriptor descr;

	ret = libusb_get_device_descriptor(dev, &descr);
	if (ret != LIBUSB_SUCCESS) {
		/* FIXME: print message */
		return false;
	}

	if ((descr.idVendor != USB_VID_OPENMOKO) ||
	    (descr.idProduct != USB_PID_OPENMOKO_VULTUREPROG)) {
		return false;
	}

	return true;
}

/**
 * @brief QiProg driver 'scan' member
 */
qiprog_err scan(struct qiprog_context * ctx, struct dev_list * qi_list)
{
	libusb_device **list;
	libusb_device *device;
	struct qiprog_device *qi_dev;
	ssize_t cnt;
	ssize_t i;

	/* Discover devices */
	cnt = libusb_get_device_list(NULL, &list);

	/* Not finding any devices is not an error */
	if (cnt < 0)
		return QIPROG_SUCCESS;

	for (i = 0; i < cnt; i++) {
		device = list[i];
		if (is_interesting(device)) {
			qi_dev = new_usb_prog(device);
			if (qi_dev == NULL) {
				/* OOPS */
				libusb_free_device_list(list, 1);
				return QIPROG_ERR_MALLOC;
			}
			dev_list_append(qi_list, qi_dev);
		}
	}

	libusb_free_device_list(list, 1);
	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'dev_open' member
 */
static qiprog_err dev_open(struct qiprog_device *dev)
{
	int ret;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	ret = libusb_open(priv->usb_dev, &(priv->handle));
	if (ret != LIBUSB_SUCCESS) {
		/* FIXME: print message */
		return QIPROG_ERR;
	}

	ret = libusb_claim_interface(priv->handle, 0);
	if (ret != LIBUSB_SUCCESS) {
		/* FIXME: print message */
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'get_capabilities' member
 */
static qiprog_err get_capabilities(struct qiprog_device *dev,
				   struct qiprog_capabilities *caps)
{
	int ret;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_GET_CAPABILITIES, 0, 0,
				      (void *)caps, sizeof(*caps), 3000);
	if (ret < LIBUSB_SUCCESS) {
		/* FIXME: print message */
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief The actual USB host driver structure
 */
struct qiprog_driver qiprog_usb_master_drv = {
	.init = init,
	.scan = scan,
	.dev_open = dev_open,
	.get_capabilities = get_capabilities,
};

/** @} */
