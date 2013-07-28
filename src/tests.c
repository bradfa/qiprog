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

#include <qiprog.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Call different functions on the device and see if they fail or not
 */
int stress_test_device(struct qiprog_device *dev)
{
	int i;
	uint8_t reg8;
	uint16_t reg16;
	uint32_t reg32;

	qiprog_err ret;
	struct qiprog_chip_id ids[9];

	/* Make sure the device can ACK a set_bus command */
	ret = qiprog_set_bus(dev, QIPROG_BUS_LPC);
	if (ret != QIPROG_SUCCESS) {
		printf("Error setting device to LPC bus\n");
		return EXIT_FAILURE;
	}

	/* Now check if a chip is connected */
	ret = qiprog_read_chip_id(dev, ids);
	if (ret != QIPROG_SUCCESS) {
		printf("Error setting reading IDs of connected chips\n");
		return EXIT_FAILURE;
	}

	for (i = 0; i < 9; i++) {
		if (ids[i].id_method == 0)
			break;
		printf("Identified chip with [manufacturer:product] ID %x:%x\n",
		       ids[i].vendor_id, ids[i].device_id);
	}

	/*
	 * Play around with reading and writing 8/16/32-bit data.
	 *
	 * LPC chips like to respond to address 0xffbc0000 with their IDs, so
	 * use this address for testing purposes.
	 */
	ret = qiprog_read8(dev, 0xffbc0000, &reg8);
	if (ret != QIPROG_SUCCESS) {
		printf("read8 failure\n");
		return EXIT_FAILURE;
	}
	printf("read8: %.2x\n", reg8);
	ret = qiprog_read16(dev, 0xffbc0000, &reg16);
	if (ret != QIPROG_SUCCESS) {
		printf("read16 failure\n");
		return EXIT_FAILURE;
	}
	printf("read16: %.4x\n", reg16);
	ret = qiprog_read32(dev, 0xffbc0000, &reg32);
	if (ret != QIPROG_SUCCESS) {
		printf("read32 failure\n");
		return EXIT_FAILURE;
	}
	printf("read32: %.8x\n", reg32);

	/*
	 * Writing all 1s to the end of the address space should be safe. We
	 * only care if the chip responds to our write requests.
	 */
	ret = qiprog_write8(dev, 0xfffffff0, 0xdb);
	if (ret != QIPROG_SUCCESS) {
		printf("write8 failure\n");
		return EXIT_FAILURE;
	}
	printf("write8 worked\n");
	ret = qiprog_write16(dev, 0xfffffff0, 0xd0b1);
	if (ret != QIPROG_SUCCESS) {
		printf("write16 failure\n");
		return EXIT_FAILURE;
	}
	printf("write16 worked\n");
	ret = qiprog_write32(dev, 0xfffffff0, 0x00c0ffee);
	if (ret != QIPROG_SUCCESS) {
		printf("write32 failure\n");
		return EXIT_FAILURE;
	}
	printf("write32 worked\n");
	return EXIT_SUCCESS;
}

int run_tests(struct qiprog_device *dev)
{
	return stress_test_device(dev);
}
