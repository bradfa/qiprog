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
 *
 * @section ctrl_req Handling control requests
 *
 * Control requests are the easiest to handle, since they are handled
 * synchronously. As soon as the device's USB stack receives a control packet on
 * the QiProg interface, it should forward it to
 * @ref qiprog_handle_control_request(). This function will handle both IN and
 * OUT requests equally, and there is no need for extra logic to distinguish
 * between the two. Only call @ref qiprog_handle_control_request() for transfers
 * with type VENDOR and recipient DEVICE (bmRequestType = 0xc0 or 0x40).
 *
 * <h3> Communicating the status to the host </h3>
 *
 * A return value of QIPROG_SUCCESS indicates that the control transaction was
 * handled successfully, and that an ACK handshake may be transmitted. Any other
 * return value indicates a failure, and a STALL handshake should be transmitted
 * to indicate the failure to the host.
 *
 * <h3> Sending data back to the host (IN transactions) </h3>
 *
 * If data needs to be returned to the host, the *len parameter will be non-zero
 * to indicate the length of data to send back. The **data parameter will point
 * to the buffer containing packet to be sent. When *len is non-zero, data must
 * be returned to the host for the transaction to complete successfully.
 *
 * <h3> Example control transaction handler </h3>
 *
 * @note
 * Depending on the capabilities of your USB stack, the handling of the control
 * requests may differ. Some stacks handle ACKs and STALLs automatically, in
 * which case some of the steps below would not be needed. Check the
 * documentation of your USB stack to see which steps are and which are not
 * needed. The following example assumes a very basic stack is used:
 *
 * @code{.c}
 *	uint8_t *data;
 *	uint16_t len;
 *
 *	// If there is a data stage, read the packet from the control endpoint
 *	if (req->wLength > 0)
 *		len = control_endpoint_read(data);
 *	else
 *		len = 0;
 *
 *	// Let QiProg handle the details
 *	ret = qiprog_handle_control_request(req->bRequest, req->wValue,
 *					    req->wIndex, req->wLength,
 *					    &data, &len);
 *
 *	if (ret != QIPROG_SUCCESS) {
 *		// There was an error. STALL the endpoint
 *		stall_control_endpoint();
 *		return;
 *	}
 *
 *	if (len != 0) {
 *		// We need to send data back to the host
 *		control_endpoint_write(data, len);
 *	} else {
 *		// No data to return, but we can ACK
 *		ack_control_endpoint();
 *	}
 * @endcode
 *
 * If you are using the libopencm3 USB stack the following is all that is
 * needed in the vendor control request handler:
 *
 * @code{.c}
 *	ret = qiprog_handle_control_request(req->bRequest, req->wValue,
 *					    req->wIndex, req->wLength,
 *					    buf, len);
 *
 *	// ACK or STALL based on whether QiProg handles the request
 *	return (ret == QIPROG_SUCCESS);
 * @endcode
 *
 *
 * @section bulk_req Handling bulk transactions
 *
 * @warning
 * This section is still in TODO stage.
 */
/** @{ */

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

/**
 * @brief Change the QiProg device to operate on
 *
 * Changes the internal QiProg device to operate on. If an active device already
 * exists, it is closed by calling its .dev_close() member. This allows you to
 * use .dev_close() to restore the hardware (GPIO and peripherals) to their
 * state before opening the device.
 * \n
 * The .dev_open() member of the new device is called. .dev_open() allows you to
 * configure the hardware used by the device to a known state before performing
 * operations with the device.
 *
 * @param[in] new_dev A new internal QiProg device to operate on
 */
void qiprog_change_device(struct qiprog_device *new_dev)
{
	/* FIXME: close old driver */

	qi_dev = new_dev;

	/* Initialize the new driver */
	qi_dev->drv->dev_open(qi_dev);
}

static uint8_t ctrl_buf[64];

/**
 * @brief Handle USB control requests
 *
 * @param[in] bRequest the bRequest field of the control transaction
 * @param[in] wValue the wValue field of the control transaction
 * @param[in] wIndex the wIndex field of the control transaction
 * @param[in] wLength the wLength field of the control transaction
 * @param[in] data pointer to a buffer containing the data of the transaction
 * @param[out] data pointer to a buffer with data to retutn to the host. Only
 * 		    valid if len is non-zero.
 * @param[out] len The size of packet in data, which needs to be sent back to
 * 		   the host
 * 		   - 0 if no data needs to be returned (OUT transaction)
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise. The
 * 	   control endpoint should be STALL'ed on errors.
 */
qiprog_err qiprog_handle_control_request(uint8_t bRequest, uint16_t wValue,
					 uint16_t wIndex, uint16_t wLength,
					 uint8_t **data, uint16_t *len)
{
	qiprog_err ret;

	/* Assume we will not be able to handle the request. */
	ret = QIPROG_ERR;

	/* The handler should decide if any data is to be returned */
	*len = 0;

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

/** @} */
