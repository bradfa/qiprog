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

	/* Internal address range - Used with set_address() and readn() */
	struct qiprog_address curr_addr_range;
	/* Underlying driver */
	struct qiprog_driver *drv;
	/* Per-device, specific context */
	void *priv;
};

/**
 * @defgroup endian_helpers QiProg byte-order conversion helpers
 *
 * @ingroup qiprog_private
 *
 * Since QiProg drivers need to communicate with the outside world, the byte
 * ordering of "outside world" devices needs to be accounted for. In QiProg
 * terms, outside data is a stream of bytes, whereas internal data is organized
 * in data structures. Outside data never comes in structures, and internal data
 * is never a plain byte stream. With this philosophy QiProg code does not, and
 * must not care about host endianess.
 *
 * To enforce this philosophy, these endian conversion helpers always treat host
 * data as standard integers types, and external data as streams identified by
 * a pointer.
 *
 * @note
 * Internal data must always be inserted to or extracted from an external dat
 * a stream. In other words, endian conversion macros, or byte-swaps must never
 * be used. Instead, always use these functions.
 */
/** @{ */
/**
 * @brief Extract a double-word from a little-endian data stream
 *
 * @param src Pointer to 16-bit value in the LE data stream
 * @return 16-bit value in host-endian order
 */
inline static uint16_t le16_to_h(void *src)
{
	uint8_t *b = src;
	return ((b[1] << 8) | (b[0] << 0));
};

/**
 * @brief Extract a long-word from a little-endian data stream
 *
 * @param src Pointer to 32-bit value in the LE data stream
 * @return 32-bit value in host-endian order
 */
inline static uint32_t le32_to_h(void *src)
{
	uint8_t *b = src;
	return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | (b[0] << 0));
};

/**
 * @brief Insert a double-word into a little-endian data stream
 *
 * @param val16 16-bit value to insert in the data stream
 * @param dest Location in the data stream where the value should be inserted
 */
inline static void h_to_le16(uint16_t val16, void *dest)
{
	uint8_t *b = dest;
	b[0] = (val16 >> 0) & 0xff;
	b[1] = (val16 >> 8) & 0xff;
};

/**
 * @brief Insert a long-word into a little-endian data stream
 *
 * @param val32 32-bit value to insert in the data stream
 * @param dest Location in the data stream where the value should be inserted
 */
inline static void h_to_le32(uint16_t val32, void *dest)
{
	uint8_t *b = dest;
	b[0] = (val32 >> 0) & 0xff;
	b[1] = (val32 >> 8) & 0xff;
	b[2] = (val32 >> 16) & 0xff;
	b[3] = (val32 >> 24) & 0xff;
};

/** @} */

/* util.c */
qiprog_err dev_list_init(struct dev_list *list);
qiprog_err dev_list_free(struct dev_list *list);
void dev_list_append(struct dev_list *list, struct qiprog_device *dev);
struct qiprog_device *qiprog_new_device(void);
qiprog_err qiprog_free_device(struct qiprog_device *dev);

#endif				/* QIPROG_INTERNAL_H */
