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
 * This section is still in TODO stage. The following mechanism is subject to
 * change.
 *
 * In order to maximize throughput, bulk operations are treated asynchronously
 * by QiProg. This approach requires a closer cooperation between QiProg and the
 * rest of the firmware. Unlike control requests, which can be entirely
 * interrupt driven, bulk operations are polled.
 *
 * <h3> Initializing the QiProg subsystem </h3>
 *
 * You first need to tell QiProg how to communicate with the rest of the
 * firmware. This is achieved with @ref qiprog_usb_dev_init(). The send_packet
 * and recv_packet tell QiProg how to move data over the USB connection. These
 * functions must only operate on the endpoint which handles the read and erase
 * operations, not the instruction set endpoint. This endpoint is the first
 * endpoint in the USB descriptors.
 *
 * Although data returned by QiProg via send_packet can be safely buffered, it
 * must be sent over the bus in-order, and must NOT be coalesced into larger
 * packets. Each send_packet callback must send exactly one packet of thei
 * indicated size. Although not required, it is recommended to only send each
 * packet after the host has sent an IN token to the endpoint. If a packet
 * cannot be transmitted at the moment, this function must not interfere with
 * other transfers, and must return 0. On success, it must return the number of
 * bytes transmitted. If the number of sent bytes does not match the packet size
 * requested, QiProg will treat this as an error.
 *
 * As with send_packet, data sent to QiProg via recv_packet must be sent to
 * QiProg in-order, and must NOT be coalesced into larger chunks. Each
 * recv_packet call must only return one packet to QiProg. If no packet is
 * available, recv_packet must return 0. On success, it must return the number
 * of bytes received. Unlike send_packet, if less bytes are received than
 * requested, QiProg will not treat the situation as an error.
 *
 * The max_rx_packet and max_tx_packet tell QiProg the maximum packet size of an
 * IN packet and OUT packet respectively. These should in the majority of cases,
 * be equal; however, if they are not identiacal, QiProg will respect these
 * sizes when moving packets.
 *
 * Finally, bulk_buf, must be a memory region capable of holding at least four
 * IN or four OUT packets, whichever is greater. This buffer must be available
 * to QiProg indefinitely, and must be left untouched by the firmware.
 *
 * <h3> Handling QiProg events </h3>
 *
 * QiProg events are processed by @ref qiprog_handle_events(). This function
 * should be called continuously from the main firmware loop. This handler is
 * safe to call at any time, even before a call to @ref qiprog_usb_dev_init().
 *
 * @todo
 * The event handler is not safely re-entrant if interrupted by a control
 * request. Fix this.
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
/** @private */
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
	static uint8_t ctrl_buf[64];

	(void)wLength;

	/* Assume we will not be able to handle the request. */
	ret = QIPROG_ERR;

	/* The handler should decide if any data is to be returned */
	*len = 0;

	switch (bRequest) {
	case QIPROG_GET_CAPABILITIES: {
		int i;
		struct qiprog_capabilities caps;

		ret = qiprog_get_capabilities(qi_dev, &caps);
		h_to_le16(caps.instruction_set, ctrl_buf + 0);
		h_to_le32(caps.bus_master, ctrl_buf + 2);
		h_to_le32(caps.max_direct_data, ctrl_buf + 4);
		for (i = 0; i < 10; i++)
			h_to_le16(caps.voltages[i], (ctrl_buf + 10) + (2 * i));
		*data = ctrl_buf;
		*len = 0x20;
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
		int i;
		uint8_t *base;
		struct qiprog_chip_id ids[9];

		ret = qiprog_read_chip_id(qi_dev, ids);
		for (i = 0; i < 9; i++) {
			base = ctrl_buf + (i * 7);
			*(base + 0) = ids[i].id_method;
			h_to_le16(ids[i].vendor_id, base + 1);
			h_to_le32(ids[i].device_id, base + 3);
		}
		*data = ctrl_buf;
		*len = 0x3f;
		break;
	}
	case QIPROG_SET_ADDRESS: {
		uint32_t start = le32_to_h(*data + 0);
		uint32_t end = le32_to_h(*data + 4);

		/* set_address() is not in the core, just the driver */
		ret = qi_dev->drv->set_address(qi_dev, start, end);
		break;
	}
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
		uint8_t reg8 = (*data)[0];
		ret = qiprog_write8(qi_dev, addr, reg8);
		break;
	}
	case QIPROG_WRITE16: {
		uint32_t addr = (wValue << 16) | wIndex;
		uint16_t reg16 = le16_to_h(*data);
		ret = qiprog_write16(qi_dev, addr, reg16);
		break;
	}
	case QIPROG_WRITE32: {
		uint32_t addr = (wValue << 16) | wIndex;
		uint32_t reg32 = le32_to_h(*data);
		ret = qiprog_write32(qi_dev, addr, reg32);
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

/*==============================================================================
 *= Asynchronous task manager
 *----------------------------------------------------------------------------*/
/** @cond private */
struct qiprog_task {
	uint8_t status;
	uint8_t *buf;
	uint16_t len;
};

struct qiprog_task_list {
	struct qiprog_task tasks[4];
	uint8_t task_start;
};

static struct qiprog_task_list task_list = {
	.tasks = {{
		.status = 0,
		.buf = NULL,
		.len = 0,
	}, {
		.status = 0,
		.buf = NULL,
		.len = 0,
	}, {
		.status = 0,
		.buf = NULL,
		.len = 0,
	}, {
		.status = 0,
		.buf = NULL,
		.len = 0,
	}},
	.task_start = 0,
};

enum {
	IDLE,
	READY_SEND,
};
/** @endcond */

/** @private */
static struct qiprog_task *get_free_task(void)
{
	int i, dx;
	struct qiprog_task *task;

	for (i = 0; i < 4; i++) {
		dx = (i + task_list.task_start) & 3;
		task = &(task_list.tasks[dx]);
		if (task->status == IDLE)
			return task;
	}
	return NULL;
}

/** @private */
static struct qiprog_task *get_first_task(void)
{
	return &(task_list.tasks[task_list.task_start]);
}

/** @private */
static void idle_task(struct qiprog_task *task)
{
	task_list.task_start = (task_list.task_start + 1) & 3;
	task->status = IDLE;
	task->len = 0;
}

/*==============================================================================
 *= How to move packets around
 *----------------------------------------------------------------------------*/
/** @cond private */
static qiprog_packet_io_cb qi_read_packet = NULL;
static qiprog_packet_io_cb qi_write_packet = NULL;
static uint16_t qi_max_rx_packet = 0;
static uint16_t qi_max_tx_packet = 0;
static uint8_t *qi_bulk_buf = NULL;
/** @endcond */

/**
 * @brief handle stuff
 */
qiprog_err qiprog_usb_dev_init(qiprog_packet_io_cb send_packet,
			       qiprog_packet_io_cb recv_packet,
			       uint16_t max_rx_packet, uint16_t max_tx_packet,
			       uint8_t *bulk_buf)
{
	int i;
	uint16_t max_packet;

	if ((send_packet == NULL) ||
	    (recv_packet == NULL) ||
	    (max_rx_packet == 0) ||
	    (max_tx_packet == 0) ||
	    (bulk_buf == NULL))
		return QIPROG_ERR_ARG;

	qi_read_packet = recv_packet;
	qi_write_packet = send_packet;
	qi_max_rx_packet = max_rx_packet;
	qi_max_tx_packet = max_tx_packet;
	qi_bulk_buf = bulk_buf;

	max_packet = MAX(max_rx_packet, max_tx_packet);
	for (i = 0; i < 4; i++)
		task_list.tasks[i].buf = bulk_buf + max_packet * i;

	return QIPROG_SUCCESS;
}

/** @private */
static void handle_send(void)
{
	uint16_t txd;
	struct qiprog_task *task;

	/*
	 * See if any packet needs sending
	 */
	task = get_first_task();
	/* Try to send if it wasn't sent last time */
	if (task->status == READY_SEND) {
		txd = qi_write_packet(task->buf, task->len);
		/* Clear task from queue if successfully transmitted */
		if (txd == task->len)
			idle_task(task);
	}
}

/**
 * @brief QiProg event handler
 */
void qiprog_handle_events(void)
{
	uint32_t len, start, end;
	struct qiprog_task *task;

	/* Have we been initialized properly? */
	if (qi_bulk_buf == NULL)
		return;

	handle_send();

	/*
	 * Now see if there is anything we can read
	 */
	start = qi_dev->addr.pread;
	end = qi_dev->addr.end;
	if (start == end)
		return;
	len = end - start + 1;

	if (len == 0)
		return;

	/* Get a free task */
	if ((task = get_free_task()) == NULL)
		return;

	task->len = MIN(len, qi_max_tx_packet);
	qiprog_read(qi_dev, start, task->buf, task->len);
	task->status = READY_SEND;
}

/** @} */
