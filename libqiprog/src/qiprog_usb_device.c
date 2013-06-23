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
 * @defgroup usb_device_file QiProg USB device driver
 *
 * @ingroup qiprog_drivers
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg USB device driver</b>
 *
 * This file contains the device-side driver for communicating with USB QiProg
 * devices. The driver deserializes USB messages to QiProg calls.
 *
 * The USB device driver, is not a driver in the normal sense. Instead, it is
 * a special translation unit that interprets QiProg USB commands and passes
 * them to one or more QiProg drivers internal to the device. QiProg commands
 * are sent to a device after it has been enumerated, discovered, and a
 * qiprog_device is available. Hence, the USB device driver only implements the
 * part of the API that requires a pointer to a qiprog_device. This part of the
 * API is also known as the QiProg core.
 *
 * @note
 * In the following paragraphs, "driver", "device driver", and "internal driver"
 * refer to QiProg drivers running on the QiProg device, not host QiProg
 * drivers. These internal drivers are entirely transparent to the host.
 *
 * An internal qiprog_device must be available as soon as the USB
 * SetConfiguration command is completed, and before receiving any USB control
 * request with type = VENDOR. The 'drv' field in the device must be a valid
 * qiprog_driver.
 *
 * A minimal driver and device declaration could look like:
 * @code{.c}
 *	static struct qiprog_driver my_driver = {
 *		.scan = NULL,	// scan is not used for internal drivers
 *		.dev_open = my_driver_init,
 *		.get_capabilities = my_driver_get_capabilities,
 *		...
 *	};
 *
 *	struct qiprog_device my_device = {
 *		 .drv = &my_driver,
 *	};
 * @endcode
 *
 * Before the USB device translator can accept control requests, it must be told
 * which device to use. This is accomplished with @ref qiprog_change_device().
 *
 * For example:
 * @code{.c}
 *	extern struct qiprog_device my_device;
 *	...
 *	qiprog_change_device(&my_device);
 * @endcode
 *
 * It is recommended to set a device before the USB SetConfiguration request is
 * completed.
 *
 * @ref qiprog_change_device() does more than just update an internal pointer.
 * If a device was already selected, that device is closed with .dev_close()
 * before updating the internal device pointer. This allows the driver to
 * restore the state of the hardware to its power-on defaults via its
 * .dev_close() member. The new driver will thus not have to worry about the
 * state or states in which a previous driver may have left the hardware.
 * .dev_open of the new driver is called before returning control. This allows
 * the new driver to configure the hardware to a state suited for its task.
 *
 * @ref qiprog_change_device() is very useful during @ref qiprog_set_bus()
 * commands, where each bus has a separate driver.
 */

#include <qiprog_usb_dev.h>

/*
 * We need a qiprog_device to run the QiProg API.
 * The bad news is we don't know what device to run until we are being told to
 * do so.
 * The good news is that we have a device, so we don't have to worry about
 * enumeration and discovery. We can also implement a different driver for
 * each buses we support, and we can change the driver when we receive a
 * qiprog_set_bus().
 * This process is a little different than "full-blown" QiProg, but it is best
 * suited for embedded devices.
 */
static struct qiprog_device *qi_dev = NULL;

void qiprog_change_device(struct qiprog_device *new_dev)
{
	/* FIXME: close old driver */

	qi_dev = new_dev;

	/* Initialize the new driver */
	qi_dev->drv->dev_open(qi_dev);
}

static uint8_t ctrl_buf[64];

qiprog_err qiprog_handle_control_request(uint8_t bRequest, uint16_t wValue,
					 uint16_t wIndex, uint16_t wLength,
					 uint8_t ** data, uint16_t * len)
{
	qiprog_err ret;

	/* Assume we will not be able to handle the request. */
	ret = QIPROG_ERR;

	switch (bRequest) {
	case QIPROG_GET_CAPABILITIES: {
		struct qiprog_capabilities *caps = (void*) ctrl_buf;
		ret = qiprog_get_capabilities(qi_dev, caps);
		*data = (void *)caps;
		*len = sizeof(*caps);
		break;
	}
	case QIPROG_SET_BUS: {
		uint32_t bus;
		bus = (wValue << 16) | wIndex;
		ret = qiprog_set_bus(qi_dev, bus);
		break;
	}
	case QIPROG_SET_CLOCK:
		/* Not Handled */
		break;
	case QIPROG_READ_DEVICE_ID: {
		struct qiprog_chip_id *ids = (void*) ctrl_buf;
		ret = qiprog_read_chip_id(qi_dev, ids);
		*data = ids;
		*len = sizeof(*ids) * 9;
		break;
	}
	case QIPROG_SET_ADDRESS:
		/* Not Handled */
		break;
	case QIPROG_SET_ERASE_SIZE:
		/* Not Handled */
		break;
	case QIPROG_SET_ERASE_COMMAND:
		/* Not Handled */
		break;
	case QIPROG_SET_WRITE_COMMAND:
		/* Not Handled */
		break;
	case QIPROG_SET_SPI_TIMING:
		/* Not Handled */
		break;
	case QIPROG_READ8: {
		uint32_t addr = (wValue << 16) | wIndex;
		ret = qiprog_read8(qi_dev, addr, (void*)ctrl_buf);
		*data = ctrl_buf;
		*len = sizeof(uint8_t);
		break;
	}
	case QIPROG_READ16: {
		uint32_t addr = (wValue << 16) | wIndex;
		ret = qiprog_read16(qi_dev, addr, (void*)ctrl_buf);
		*data = ctrl_buf;
		*len = sizeof(uint16_t);
		break;
	}
	case QIPROG_READ32: {
		uint32_t addr = (wValue << 16) | wIndex;
		ret = qiprog_read32(qi_dev, addr, (void*)ctrl_buf);
		*data = ctrl_buf;
		*len = sizeof(uint32_t);
		break;
	}
	case QIPROG_WRITE8: {
		uint32_t addr = (wValue << 16) | wIndex;
		uint8_t *reg8 = (void*)(*data);
		ret = qiprog_write8(qi_dev, addr, *reg8);
		break;
	}
	case QIPROG_WRITE16: {
		uint32_t addr = (wValue << 16) | wIndex;
		uint16_t *reg16 = (void*)(*data);
		ret = qiprog_write16(qi_dev, addr, *reg16);
		break;
	}
	case QIPROG_WRITE32: {
		uint32_t addr = (wValue << 16) | wIndex;
		uint32_t *reg32 = (void*)(*data);
		ret = qiprog_write32(qi_dev, addr, *reg32);
		break;
	}
	case QIPROG_SET_VDD:
		/* Not Handled */
		break;
	default:
		/* Nothing to handle */
		break;
	}

	return ret;
}
