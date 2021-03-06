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

////#include <stdio.h>
////#include <sys/time.h>

#define LOG_DOMAIN "usb_host: "
#define qi_err(str, ...)	qi_perr(LOG_DOMAIN str,  ##__VA_ARGS__)
#define qi_warn(str, ...)	qi_pwarn(LOG_DOMAIN str, ##__VA_ARGS__)
#define qi_info(str, ...)	qi_pinfo(LOG_DOMAIN str, ##__VA_ARGS__)
#define qi_dbg(str, ...)	qi_pdbg(LOG_DOMAIN str,  ##__VA_ARGS__)
#define qi_spew(str, ...)	qi_pspew(LOG_DOMAIN str, ##__VA_ARGS__)

/*
 * The maximum number of USB transfers that may be active at any given time
 * Each device may have at most MAX_CONCURRENT_TRANSFERS during bulk operations.
 */
#define MAX_CONCURRENT_TRANSFERS	((unsigned int)32)

struct qiprog_driver qiprog_usb_master_drv;

/**
 * @brief Private per-device context for USB devices
 */
struct usb_master_priv {
	libusb_device_handle *handle;
	libusb_device *usb_dev;
	uint16_t ep_size_in;
	uint16_t ep_size_out;
	/* Buffer used to store 'leftover' bulk data */
	uint8_t * buf;
	size_t buflen;
};

/**
 * @brief Helper to create a new USB QiProg device
 */
static struct qiprog_device *new_usb_prog(libusb_device *libusb_dev,
					  struct qiprog_context *ctx)
{
	/*
	 * Peter Stuge is the person who started it all. He is also the de-facto
	 * USB expert for free software hackers to go to with questions. As a
	 * result, every QiProg device connected via USB shall be named after
	 * him.
	 */
	int ep_in, ep_out;
	struct qiprog_device *peter_stuge;
	struct usb_master_priv *priv = NULL;

	peter_stuge = qiprog_new_device(ctx);

	if (peter_stuge == NULL)
		return NULL;

	if ((priv = malloc(sizeof(*priv))) == NULL) {
		qi_warn("Could not allocate memory for device. Aborting");
		goto cleanup;
	}
	memset(priv, 0, sizeof(*priv));

	peter_stuge->drv = &qiprog_usb_master_drv;
	priv->usb_dev = libusb_dev;
	/* Don't create a handle until the device is opened */
	priv->handle = NULL;
	peter_stuge->priv = priv;
	/* Get the maximum packet sizes */
	ep_in = libusb_get_max_packet_size(libusb_dev, 0x81);
	ep_out = libusb_get_max_packet_size(libusb_dev, 1);

	if ((ep_in < 0) || (ep_out < 0)) {
		qi_warn("Could not get endpoint size. Aborting");
		goto cleanup;
	}

	priv->ep_size_in = ep_in;
	priv->ep_size_out = ep_out;

	qi_spew("Max packet size: %i IN, %i OUT", ep_in, ep_out);

	if ((priv->buf = malloc(MAX(ep_in, ep_out))) == NULL) {
		qi_warn("Could not allocate memory.");
		goto cleanup;
	}

	return peter_stuge;

 cleanup:
	if (priv && priv->buf)
		free(priv->buf);
	free(priv);
	qiprog_free_device(peter_stuge);
	return NULL;
}

/**
 * @brief Decide if given USB device is a QiProg device
 */
static bool is_interesting(libusb_device *dev)
{
	int ret;
	struct libusb_device_descriptor descr;

	ret = libusb_get_device_descriptor(dev, &descr);
	if (ret != LIBUSB_SUCCESS) {
		qi_warn("Could not get descriptor: %s", libusb_error_name(ret));
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
static qiprog_err scan(struct qiprog_context *ctx, struct dev_list *qi_list)
{
	libusb_device **list;
	libusb_device *device;
	struct qiprog_device *qi_dev;
	ssize_t cnt;
	ssize_t i;

	/* Discover devices */
	cnt = libusb_get_device_list(ctx->libusb_host_ctx, &list);

	/* Not finding any devices is not an error */
	if (cnt < 0)
		return QIPROG_SUCCESS;

	for (i = 0; i < cnt; i++) {
		device = list[i];
		if (is_interesting(device)) {
			qi_dev = new_usb_prog(device, ctx);
			if (qi_dev == NULL) {
				libusb_free_device_list(list, 1);
				qi_err("Malloc failure");
				return QIPROG_ERR_MALLOC;
			}
			dev_list_append(qi_list, qi_dev);
		}
	}

	libusb_free_device_list(list, 0);
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
		qi_err("Could not open device: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	ret = libusb_claim_interface(priv->handle, 0);
	if (ret != LIBUSB_SUCCESS) {
		qi_warn("Could not claim interface: %s",
			libusb_error_name(ret));
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
	int ret, i;
	uint8_t buf[64];
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_GET_CAPABILITIES, 0, 0,
				      (void *)buf, 0x20, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	/* USB is LE, we are host-endian */
	caps->instruction_set = le16_to_h(buf + 0);
	caps->bus_master = le32_to_h(buf + 2);
	caps->max_direct_data = le32_to_h(buf + 6);
	for (i = 0; i < 10; i++)
		caps->voltages[i] = le16_to_h((buf + 10) + (2 * i));

	return QIPROG_SUCCESS;
}

static qiprog_err set_bus(struct qiprog_device *dev, enum qiprog_bus bus)
{
	int ret;
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;
	if (!bus)
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the QIPROG_BUS_ constant */
	wValue = bus >> 16;
	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = bus & 0xffff;

	/*
	 * FIXME: This doesn't seem to return an error when the device NAKs the
	 * request.
	 */
	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_BUS, wValue, wIndex,
				      NULL, 0, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'read_chip_id' member
 */
static qiprog_err read_chip_id(struct qiprog_device *dev,
			       struct qiprog_chip_id ids[9])
{
	int ret, i;
	uint8_t buf[64];
	uint8_t * base;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_READ_DEVICE_ID, 0, 0,
				      (void *)buf, 0x3f * 9, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	/* USB is LE, we are host-endian */
	for (i = 0; i < 9; i++) {
		base = buf + (i * 7);
		ids[i].id_method = *(base + 0);
		ids[i].vendor_id = le16_to_h(base + 1);
		ids[i].device_id = le32_to_h(base + 3);
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_chip_size' member
 */
static qiprog_err set_chip_size(struct qiprog_device *dev, uint8_t chip_idx,
				uint32_t size)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	h_to_le32(size, buf);

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_CHIP_SIZE, 0, wIndex,
				      buf, 0x04, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'read8' member
 *
 * TODO: Try to unify read 8/16/32 into one common function
 */
static qiprog_err read8(struct qiprog_device *dev, uint32_t addr,
			uint8_t *data)
{
	int ret;
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_READ8, wValue, wIndex,
				      (void *)data, sizeof(*data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'read16' member
 */
static qiprog_err read16(struct qiprog_device *dev, uint32_t addr,
			 uint16_t *data)
{
	int ret;
	uint8_t buf[64];
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_READ16, wValue, wIndex,
				      (void *)buf, sizeof(*data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	/* USB is LE, we are host-endian */
	*data = le16_to_h(buf);

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'read32' member
 */
static qiprog_err read32(struct qiprog_device *dev, uint32_t addr,
			 uint32_t *data)
{
	int ret;
	uint8_t buf[64];
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	ret = libusb_control_transfer(priv->handle, 0xc0,
				      QIPROG_READ32, wValue, wIndex,
				      (void *)buf, sizeof(*data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	/* USB is LE, we are host-endian */
	*data = le32_to_h(buf);

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'write8' member
 *
 * TODO: Try to unify write 8/16/32 into one common function
 */
static qiprog_err write8(struct qiprog_device *dev, uint32_t addr, uint8_t data)
{
	int ret;
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_WRITE8, wValue, wIndex,
				      (void *)&data, sizeof(data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'write16' member
 */
static qiprog_err write16(struct qiprog_device *dev, uint32_t addr,
			  uint16_t data)
{
	int ret;
	uint8_t buf[64];
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	/* USB is LE, we are host-endian */
	h_to_le16(data, buf);

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_WRITE16, wValue, wIndex,
				      (void *)buf, sizeof(data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'write32' member
 */
static qiprog_err write32(struct qiprog_device *dev, uint32_t addr,
			  uint32_t data)
{
	int ret;
	uint8_t buf[64];
	uint16_t wValue, wIndex;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Most significant 16 bits of the memory address to read from */
	wValue = addr >> 16;
	/* Least significant 16 bits of the memory address to read from */
	wIndex = addr & 0xffff;

	/* USB is LE, we are host-endian */
	h_to_le32(data, buf);

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_WRITE32, wValue, wIndex,
				      (void *)buf, sizeof(data), 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief Tell the programmer what address range we want to operate on
 */
static qiprog_err set_address(struct qiprog_device *dev, uint32_t start,
			      uint32_t end)
{
	int ret;
	uint8_t buf[64];
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	qi_spew("Setting address range 0x%.8lx -> 0x%.8lx\n", start, end);

	/*
	 * Contents of the buffer are no longer valid. Device will start to
	 * read at the new address. We get those contents with the next readn()
	 * call.
	 */
	priv->buflen = 0;

	/* USB is LE, we are host-endian */
	h_to_le32(start, buf + 0);
	h_to_le32(end, buf + 4);

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_ADDRESS, 0, 0,
			              (void *)buf, 0x08, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	/* We have successfully set the address, now keep track of it */
	dev->addr.end = end;
	/* Read and write pointers are reset when setting a new range */
	dev->addr.pread = dev->addr.pwrite = dev->addr.start = start;

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_erase_size' member
 */
static qiprog_err set_erase_size(struct qiprog_device *dev, uint8_t chip_idx,
				 enum qiprog_erase_type *types, uint32_t *sizes,
				 size_t num_sizes)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	size_t i;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;
	if (num_sizes == 0)
		return QIPROG_ERR_ARG;
	/* We can only fit 12 erase sizes in a control packet */
	if (num_sizes > 12)
		return QIPROG_ERR_LARGE_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	for (i = 0; i < num_sizes; i++) {
		buf[i*5] = (uint8_t)types[i];
		h_to_le32(sizes[i], buf + i*5 + 1);
	}

	ret = libusb_control_transfer(priv->handle, 0x40, QIPROG_SET_ERASE_SIZE,
				      0, wIndex, buf, num_sizes * 5, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_erase_command' member
 */
static qiprog_err set_erase_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_erase_cmd cmd,
				    enum qiprog_erase_subcmd subcmd,
				    uint16_t flags)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	buf[0] = (uint8_t)cmd;
	buf[1] = (uint8_t)subcmd;
	h_to_le16(flags, buf + 2);

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_ERASE_COMMAND, 0, wIndex, buf,
				      0x04, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_custom_erase_command' member
 */
static qiprog_err set_custom_erase_command(struct qiprog_device *dev,
					   uint8_t chip_idx, uint32_t *addr,
					   uint8_t *data, size_t num_bytes)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	size_t i;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;
	if (num_bytes == 0)
		return QIPROG_ERR_ARG;
	/* We can only fit 12 erase steps in a control packet */
	if (num_bytes > 12)
		return QIPROG_ERR_LARGE_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	buf[0] = (uint8_t)QIPROG_ERASE_CMD_CUSTOM;
	buf[1] = (uint8_t)QIPROG_ERASE_SUBCMD_CUSTOM;
	h_to_le16(0, buf + 2);
	for (i = 0; i < num_bytes; i++) {
		buf[4 + i*5] = data[i];
		h_to_le32(addr[i], buf + 4 + i*5);
	}

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_ERASE_COMMAND, 0, wIndex, buf,
				      4 + num_bytes * 5, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_write_command' member
 */
static qiprog_err set_write_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_write_cmd cmd,
				    enum qiprog_write_subcmd subcmd)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	buf[0] = (uint8_t)cmd;
	buf[1] = (uint8_t)subcmd;
	/* bytes 2,3 ignored (?) TODO! */

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_WRITE_COMMAND, 0, wIndex, buf,
				      0x04, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'set_custom_write_command' member
 */
static qiprog_err set_custom_write_command(struct qiprog_device *dev,
					   uint8_t chip_idx, uint32_t *addr,
					   uint8_t *data, size_t num_bytes)
{
	int ret;
	uint16_t wIndex;
	uint8_t buf[64];
	size_t i;
	struct usb_master_priv *priv;

	if (!dev)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;
	if (num_bytes == 0)
		return QIPROG_ERR_ARG;
	/* We can only fit 12 erase steps in a control packet */
	if (num_bytes > 12)
		return QIPROG_ERR_LARGE_ARG;

	/* Least significant 16 bits of the QIPROG_BUS_ constant */
	wIndex = chip_idx;

	/* USB is LE, we are host-endian */
	buf[0] = (uint8_t)QIPROG_WRITE_CMD_CUSTOM;
	buf[1] = (uint8_t)QIPROG_WRITE_SUBCMD_CUSTOM;
	h_to_le16(0, buf + 2);
	for (i = 0; i < num_bytes; i++) {
		buf[4 + i*5] = data[i];
		h_to_le32(addr[i], buf + 4 + i*5);
	}

	ret = libusb_control_transfer(priv->handle, 0x40,
				      QIPROG_SET_WRITE_COMMAND, 0, wIndex, buf,
				      4 + num_bytes * 5, 3000);
	if (ret < LIBUSB_SUCCESS) {
		qi_err("Control transfer failed: %s", libusb_error_name(ret));
		return QIPROG_ERR;
	}

	return QIPROG_SUCCESS;
}

/*==============================================================================
 *= Bulk transaction handlers
 *----------------------------------------------------------------------------*/
/** Callback data passed to asynchronous USB transfers */
struct usb_host_cb_data {
	/** When the transfers were started */
	double starttime;
	/** The total number of bytes transferred up until now */
	volatile uint32_t *transferred_bytes;
	/** The number of transfers which are still active */
	volatile uint32_t *active_transfers;
	/** The total number of transfers needed to complete the transaction */
	uint32_t total_transfers;
	/** Queue depth, or maximum number of concurrent transfers */
	uint32_t queue_depth;
	/** The sequential number assigned to this transfer */
	uint32_t transfer_number;
};

static inline double get_time()
{
	////struct timespec timer;
	////clock_gettime(CLOCK_MONOTONIC, &timer);
	////return (double)timer.tv_sec + (double)(timer.tv_nsec)/1E9;
	return 0.0;
}

static void async_cb(struct libusb_transfer *transfer)
{
	int ret;
	double endtime, time, avg_speed;
	struct usb_host_cb_data *cb_data = transfer->user_data;
	const uint32_t next = cb_data->transfer_number + cb_data->queue_depth;

	/*
	 * Error handling
	 */
	ret = QIPROG_SUCCESS;
	/* A failed transfer can mess up the data, so halt if we meet one */
	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		qi_err("Transfer failed: %s",
		       libusb_error_name(transfer->status));
		/* FIXME: How to exit elegantly? */
		ret = QIPROG_ERR;
	}

	if (transfer->actual_length != transfer->length) {
		/* FIXME: This can be serious. figure out how to handle */
		qi_warn("Transfer of %u bytes only brought %u bytes",
			transfer->length, transfer->actual_length);
		ret = QIPROG_ERR;
	}

	/*
	 * Print timing information
	 */
	endtime = get_time();
	time = endtime - cb_data->starttime;
	*(cb_data->transferred_bytes) += transfer->actual_length;

	avg_speed = *(cb_data->transferred_bytes) / time;
	(void)avg_speed;

	////printf("\rSpeed %.1f KiB/s", avg_speed/1024);
	////fflush(stdout);

	/*
	 * Resubmit another transfer if needed
	 */
	if ((next < cb_data->total_transfers) && (ret == QIPROG_SUCCESS)) {
		cb_data->transfer_number = next;
		transfer->buffer += transfer->length * cb_data->queue_depth;
		if (libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
			qi_err("Failed to resubmit transfer");
			(*(cb_data->active_transfers))--;
		}
	} else {
		/* Careful how we dereference this one !!! */
		(*(cb_data->active_transfers))--;
	}
}

/*
 * This function handles both in and out transactions equally well. The
 * direction is given by the ep parameter. We do not do anything to distinguish
 * between IN and OUT transactions, hence why we can use this function for
 * either case.
 */
static int do_async_bulk_transfers(libusb_context *usb_ctx,
				   libusb_device_handle *handle,
				   unsigned char ep, uint16_t ep_size,
				   void *data, uint32_t n)
{
	int ret;
	size_t i;
	uint8_t *curr_buf;
	volatile double starttime;
	volatile uint32_t transferred_bytes;
	volatile uint32_t active_transfers;
	struct libusb_transfer *transfers[MAX_CONCURRENT_TRANSFERS];
	struct usb_host_cb_data cbds[MAX_CONCURRENT_TRANSFERS];

	const uint32_t transz = ep_size;
	/* Intentionally round down. Leftover packets are not handled here */
	const uint32_t total = n / transz;
	const uint32_t depth = MIN(total , MAX_CONCURRENT_TRANSFERS);

	/*
	 * We transfer in multiples of ep_size. When there is a leftover packet,
	 * we don't transfer it here, so we may end up with zero transfers for
	 * small enough transfers.
	 */
	if (total == 0)
		return QIPROG_SUCCESS;

	/*
	 * Submit initial transfers
	 * The transfers will re-submit themselves when completed
	 */
	qi_info("Starting %i transfers of %i bytes each", total, transz);

	active_transfers = depth;
	transferred_bytes = 0;
	starttime = get_time();

	for (i = 0; i < depth; i++) {
		/* FIXME: Handle alloc failures, don't just crash */
		transfers[i] = libusb_alloc_transfer(0);
		curr_buf = data + transz * i;
		cbds[i].starttime = starttime;
		cbds[i].active_transfers = &active_transfers;
		cbds[i].transferred_bytes = &transferred_bytes;
		cbds[i].total_transfers = total;
		cbds[i].queue_depth = depth;
		cbds[i].transfer_number = i;

		libusb_fill_bulk_transfer(transfers[i], handle, ep, curr_buf,
					  transz, async_cb, (void*)&cbds[i],
					  3000);
		ret = libusb_submit_transfer(transfers[i]);
		if (ret != LIBUSB_SUCCESS) {
			qi_err("Error submitting transfer: %s",
			       libusb_error_name(ret));
			/* FIXME: cleanup, don't just exit */
			return QIPROG_ERR;
		}
	}

	/*
	 * Block until all transfers come back
	 */
	while (active_transfers) {
		ret =  libusb_handle_events(usb_ctx);
		if (ret != LIBUSB_SUCCESS) {
			qi_err("Error: %s\n", libusb_error_name(ret));
			/* FIXME: cleanup, don't just exit */
			return QIPROG_ERR;
		}
	}

	if (transferred_bytes != n) {
		qi_warn("Only transferred %i bytes of %i bytes",
			transferred_bytes, n);
		/* FIXME: cleanup, don't just exit */
		return QIPROG_ERR;
	}

	/* FIXME: cleanup, cleanup, cleanup */

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'read' member
 */
static qiprog_err read(struct qiprog_device *dev, uint32_t where, void *dest,
		       uint32_t n)
{
	int ret, left, len;
	size_t copysz, range;
	struct usb_master_priv *priv;


	if (!dev)
		return QIPROG_ERR_ARG;
	if (!dev->ctx)
		return QIPROG_ERR_ARG;
	if (!dev->ctx->libusb_host_ctx)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/*
	 * Avoid a set_address round-trip if our read pointer is where we want
	 * it to be
	 * FIXME: What if max address is less than where we want to read to, or
	 * if we want to read more than the chip size?
	 */
	if ((dev->addr.pread != where) || (dev->addr.end < (where + n))) {
		/* Can not avoid a round trip */
		ret = set_address(dev, where, where + n);
		if (ret != QIPROG_SUCCESS) {
			qi_err("Could not set address range %i", ret);
			return ret;
		}
	}

	/* See how much the device has left to read */
	range = dev->addr.end + 1 - dev->addr.pread;
	/* Stop if we have been requested to read too much */
	if (n > (range + priv->buflen)) {
		qi_err("I can give you %i bytes, but you asked me to read %i",
		       (int)(range + priv->buflen), (int) n);
		return QIPROG_ERR_ARG;
	}

	/* Write any leftover data from the previous read */
	if ((copysz = MIN(n, priv->buflen)) != 0) {
		memcpy(dest, priv->buf, copysz);
		dest += copysz;
		n -= copysz;
		priv->buflen -= copysz;

		/* Keep any remaining data at the top of the buffer */
		if (priv->buflen) {
			memmove(priv->buf, priv->buf + copysz, priv->buflen);
			/* If there's still data in the buffer, we're done */
			return QIPROG_SUCCESS;
		}
	}

	/* Only read in multiples of the endpoint size */
	range = (n / priv->ep_size_in) * priv->ep_size_in;
	qi_spew("Reading 0x%.8lx -> 0x%.8lx", dev->addr.pread,
		dev->addr.pread + range - 1);

	ret = do_async_bulk_transfers(dev->ctx->libusb_host_ctx, priv->handle,
				      0x81, priv->ep_size_in, dest, range);
	/* Stop here on any error. async handler will print an error message. */
	if (ret != QIPROG_SUCCESS)
		return ret;

	/* Update address range to reflect the previously read bytes */
	dev->addr.pread += range;

	/*
	 * Handle leftover transfers
	 */
	left = n % priv->ep_size_in;
	if (left) {
		qi_spew("Reading leftover packet from 0x%.8lx",
			dev->addr.pread);
		/*
		 * Try to read a whole packet. If the device sends less data
		 * (last packet), we will see that
		 */
		ret = libusb_bulk_transfer(priv->handle, 0x81, priv->buf,
					   priv->ep_size_in, &len, 3000);
		if (ret != LIBUSB_SUCCESS) {
			qi_err("Could not complete transfer: %s",
			       libusb_error_name(ret));
			return QIPROG_ERR;
		}

		/* We should get at least 'left' bytes of data */
		if (len < left) {
			qi_err("Received less data than expected.");
			return QIPROG_ERR;
		}

		/* Update address range to reflect the previously read bytes */
		dev->addr.pread += len;

		/* Move the data to the user-specified memory region */
		memcpy(dest + n - left, priv->buf, left);
		priv->buflen = len - left;
		/* And copy the rest to the start of our internal buffer */
		memmove(priv->buf, priv->buf + left, priv->buflen);
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'write' member
 */
static qiprog_err write(struct qiprog_device *dev, uint32_t where, void *src,
			uint32_t n)
{
	int ret, left, len;
	size_t range;
	struct usb_master_priv *priv;


	if (!dev)
		return QIPROG_ERR_ARG;
	if (!dev->ctx)
		return QIPROG_ERR_ARG;
	if (!dev->ctx->libusb_host_ctx)
		return QIPROG_ERR_ARG;
	if (!(priv = dev->priv))
		return QIPROG_ERR_ARG;

	/*
	 * Avoid a set_address round-trip if our write pointer is where we want
	 * it to be
	 * FIXME: What if max address is less than where we want to write to, or
	 * if we want to write more than the chip size?
	 */
	if (dev->addr.pwrite != where || (dev->addr.end < (where + n))) {
		/* Can not avoid a round trip */
		ret = set_address(dev, where, where + n);
		if (ret != QIPROG_SUCCESS) {
			qi_err("Could not set address range %i", ret);
			return ret;
		}
	}

	/* See how much the device has left to read */
	range = dev->addr.end + 1 - dev->addr.pwrite;
	/* Stop if we have been requested to write too much */
	if (n > range) {
		qi_err("I can write %i bytes, but you asked me to write %i",
		       (int)(range + priv->buflen), (int) n);
		return QIPROG_ERR_ARG;
	}

	/* Only program in multiples of the endpoint size */
	range = (n / priv->ep_size_in) * priv->ep_size_in;
	qi_spew("Programming 0x%.8lx -> 0x%.8lx", dev->addr.pwrite,
		dev->addr.pwrite - 1 + range);

	ret = do_async_bulk_transfers(dev->ctx->libusb_host_ctx, priv->handle,
				      0x01, priv->ep_size_in, src, range);
	/* Stop here on any error. async handler will print an error message. */
	if (ret != QIPROG_SUCCESS)
		return ret;

	/* Update address range to reflect the previously programmed bytes */
	dev->addr.pwrite += range;

	/*
	 * Handle leftover transfers
	 * Unlike bulk reads, we do not need to send endpoint-sized packets, and
	 * thus the last packet could be smaller than the endpoint size.
	 * However, we handle leftover packets separately for now.
	 */
	left = n % priv->ep_size_in;
	if (left) {
		qi_spew("Programming from 0x%.8lx", dev->addr.pwrite);
		/*
		 * Try to read a whole packet. If the device sends less data
		 * (last packet), we will see that
		 */
		ret = libusb_bulk_transfer(priv->handle, 0x01, src + n - left,
					   left, &len, 3000);
		if (ret != LIBUSB_SUCCESS) {
			qi_err("Could not complete transfer: %s",
			       libusb_error_name(ret));
			return QIPROG_ERR;
		}

		/* We should have sent at least 'left' bytes of data */
		if (len < left) {
			qi_err("Sent less data than expected.");
			return QIPROG_ERR;
		}

		/* Update address range */
		dev->addr.pwrite += len;
	}

	return QIPROG_SUCCESS;
}

/**
 * @brief The actual USB host driver structure
 */
struct qiprog_driver qiprog_usb_master_drv = {
	.scan = scan,
	.dev_open = dev_open,
	.set_bus = set_bus,
	.get_capabilities = get_capabilities,
	.read_chip_id = read_chip_id,
	.set_chip_size = set_chip_size,
	.set_erase_size = set_erase_size,
	.set_erase_command = set_erase_command,
	.set_custom_erase_command = set_custom_erase_command,
	.set_write_command = set_write_command,
	.set_custom_write_command = set_custom_write_command,
	.read8 = read8,
	.read16 = read16,
	.read32 = read32,
	.write8 = write8,
	.write16 = write16,
	.write32 = write32,
	.read = read,
	.write = write,
};

/** @} */
