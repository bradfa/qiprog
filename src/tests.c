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
		printf("Error reading IDs of connected chips\n");
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

/*
 * Make sure misaligned reads return correct data
 */
int test_alignment(struct qiprog_device *dev)
{
	int ret;
	uint8_t *align = NULL, *unalign = NULL;
	size_t i;
	const size_t size = 1024;
	const uint32_t top = 0xffffffff;

	/* Assume the worst */
	ret = EXIT_FAILURE;

	if ((align = malloc(size)) == NULL)
		goto cleanup;
	if ((unalign = malloc(size)) == NULL)
		goto cleanup;

	/* Read top 1 KiB in one pass */
	if (qiprog_set_address(dev, top - size + 1, top) != QIPROG_SUCCESS)
		goto cleanup;
	if (qiprog_readn(dev, align, size))
		goto cleanup;

	/* Now that we have a reference, write its inverse in the test buffer */
	for (i = 0; i < size; i++)
		unalign[i] = ~align[i];

	/*
	 * Alignment test #1:
	 * Make sure buffer is not overwritten on incomplete reads. This test
	 * ensures that the granularity of a device is properly handled and the
	 * buffer is not written past its end. qiprog_readn must assume that the
	 * buffer is exactly n bytes long. If it writes past that, the test
	 * fails.
	 */

	printf("Checking for buffer overflows\n");
	/* Trick the device into thinking we want to read the top 1 KiB */
	if (qiprog_set_address(dev, top - size + 1, top) != QIPROG_SUCCESS)
		goto cleanup;
	/* But only read 15 bytes */
	if (qiprog_readn(dev, unalign, 15) != QIPROG_SUCCESS)
		goto cleanup;

	/* Now make sure that the bytes match */
	for (i = 0 ; i < 15; i++) {
		if (unalign[i] != align[i]) {
			printf("Failed to re-read (byte %i)\n", (int)i);
			goto cleanup;
		}
	}
	/* Make sure that extra bytes were not overwritten */
	for (i = 15; i < size; i++) {
		if (unalign[i] != (~align[i] & 0xff)) {
			printf("Buffer overflow (byte %i)\n", (int)i);
			goto cleanup;
		}
	}

	/*
	 * Alignment test #2:
	 * Make sure read can continue where it left off. This test ensures that
	 * reads of different sizes are done in order, and if needed, the device
	 * driver properly buffers any extra data.
	 */

	printf("Checking if bulk reads are 1-byte granular\n");
	/* Read half of the remaining bytes */
	if (qiprog_readn(dev, unalign + 15, size/2) != QIPROG_SUCCESS)
		goto cleanup;
	for (i = 15; i < size/2; i++) {
		if (unalign[i] != align[i]) {
			printf("Read resumed incorrectly (byte %i)\n", (int)i);
			goto cleanup;
		}
	}

	/*
	 * Alignment test #3:
	 * Make sure the device discards any data it may have read and buffered
	 * after a readn() did not access the entire range specified in
	 * set_address(). In the previous test, we asked the device for 1024
	 * bytes, but we only recovered 512.
	 */

	printf("Checking if device properly discards obsolete buffers\n");
	/* Make a new set_address request */
	if (qiprog_set_address(dev, top - size + 1, top) != QIPROG_SUCCESS)
		goto cleanup;
	/* And read the entire range */
	if (qiprog_readn(dev, unalign, size) != QIPROG_SUCCESS)
		goto cleanup;

	for (i = 0; i < size; i++) {
		if (unalign[i] != align[i]) {
			printf("Buffer was not discarded (byte %i)\n", (int)i);
			goto cleanup;
		}
	}

	/* Tests passed */
	ret = EXIT_SUCCESS;

 cleanup:
	free(align);
	free(unalign);

	return ret;
}

int run_tests(struct qiprog_device *dev)
{
	if (stress_test_device(dev) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (test_alignment(dev) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
