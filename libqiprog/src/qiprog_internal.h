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

#include <qiprog.h>

#define QIPROG_RETURN_ON_BAD_DEV(dev) do {	\
	if (dev == NULL)			\
		return QIPROG_ERR_ARG;		\
	if (dev->drv == NULL)			\
		return QIPROG_ERR_ARG;		\
	} while (0)

#if defined(CONFIG_DRIVER_USB_MASTER) && (CONFIG_DRIVER_USB_MASTER)
#include <qiprog_usb_host.h>
#endif

struct qiprog_context {
#if defined(CONFIG_DRIVER_USB_MASTER) && (CONFIG_DRIVER_USB_MASTER)
	struct libusb_context *libusb_host_ctx;
#endif
};

#define LIST_STEP 128
struct dev_list {
	size_t len;
	size_t capacity;
	struct qiprog_device **devs;
};

/*
 * TODO: Functions which take varargs are NOT IMPLEMENTED yet.
 */
struct qiprog_driver {
	qiprog_err(*scan) (struct qiprog_context * ctx, struct dev_list * list);
	qiprog_err(*dev_open) (struct qiprog_device * dev);
	qiprog_err(*get_capabilities) (struct qiprog_device * dev,
				       struct qiprog_capabilities * caps);
	qiprog_err(*set_bus) (struct qiprog_device * dev, enum qiprog_bus bus);
	qiprog_err(*set_clock) (struct qiprog_device * dev,
				uint32_t * clock_khz);
	qiprog_err(*read_chip_id) (struct qiprog_device * dev,
				   struct qiprog_chip_id ids[9]);
	qiprog_err(*set_address) (struct qiprog_device * dev, uint32_t start,
				  uint32_t end);
	qiprog_err(*set_erase_size) (struct qiprog_device * dev, ...);
	qiprog_err(*set_erase_command) (struct qiprog_device * dev, ...);
	qiprog_err(*set_write_command) (struct qiprog_device * dev, ...);
	qiprog_err(*set_spi_timing) (struct qiprog_device * dev,
				     uint16_t tpu_read_us, uint32_t tces_ns);
	qiprog_err(*read8) (struct qiprog_device * dev, uint32_t addr,
			    uint8_t * data);
	qiprog_err(*read16) (struct qiprog_device * dev, uint32_t addr,
			     uint16_t * data);
	qiprog_err(*read32) (struct qiprog_device * dev, uint32_t addr,
			     uint32_t * data);
	qiprog_err(*write8) (struct qiprog_device * dev, uint32_t addr,
			     uint8_t data);
	qiprog_err(*write16) (struct qiprog_device * dev, uint32_t addr,
			      uint16_t data);
	qiprog_err(*write32) (struct qiprog_device * dev, uint32_t addr,
			      uint32_t data);
	qiprog_err(*set_vdd) (struct qiprog_device * dev, uint16_t vdd_mv);
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
