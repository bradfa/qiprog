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

#ifndef __QIPROG_USB_DEV_H
#define __QIPROG_USB_DEV_H

#include <qiprog_usb.h>
#include "../src/qiprog_internal.h"

typedef uint16_t (*qiprog_packet_io_cb) (void *data, uint16_t len);

void qiprog_change_device(struct qiprog_device *new_dev);

qiprog_err qiprog_handle_control_request(uint8_t bRequest, uint16_t wValue,
					 uint16_t wIndex, uint16_t wLength,
					 uint8_t **data, uint16_t *len);

qiprog_err qiprog_usb_dev_init(qiprog_packet_io_cb send_packet,
			       qiprog_packet_io_cb recv_packet,
			       uint16_t max_rx_packet, uint16_t max_tx_packet,
			       uint8_t *bulk_buf);
void qiprog_handle_events(void);

#endif				/* __QIPROG_USB_DEV_H */
