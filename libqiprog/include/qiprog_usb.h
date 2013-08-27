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

#ifndef __QIPROG_USB_H
#define __QIPROG_USB_H

#include <qiprog.h>

#define USB_VID_OPENMOKO		0x1d50
#define USB_PID_OPENMOKO_VULTUREPROG	0x6076

/**
 * @brief QiProg USB control request types for Control transfers
 *
 * @note
 * Note that all values transferred over the bus, including those in wValue,
 * wIndex and wLength fields, are LE coded! (The host side API likely translates
 * wLength automatically if necessary, but maybe not wValue and wIndex, and
 * certainly not the data. Check that your bytes are not swapped.)
 */
enum qiprog_usb_ctrl_req_type {
	QIPROG_GET_CAPABILITIES = 0x00,
	QIPROG_SET_BUS = 0x01,
	QIPROG_SET_CLOCK = 0x02,
	QIPROG_READ_DEVICE_ID = 0x03,
	QIPROG_SET_ADDRESS = 0x04,
	QIPROG_SET_ERASE_SIZE = 0x05,
	QIPROG_SET_ERASE_COMMAND = 0x06,
	QIPROG_SET_WRITE_COMMAND = 0x07,
	QIPROG_SET_CHIP_SIZE = 0x08,
	QIPROG_SET_SPI_TIMING = 0x20,
	QIPROG_READ8 = 0x30,
	QIPROG_READ16 = 0x31,
	QIPROG_READ32 = 0x32,
	QIPROG_WRITE8 = 0x33,
	QIPROG_WRITE16 = 0x34,
	QIPROG_WRITE32 = 0x35,
	QIPROG_SET_VDD = 0xf0,
};

#endif				/* __QIPROG_USB_H */
