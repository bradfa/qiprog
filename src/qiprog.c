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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <qiprog.h>


struct qiprog_cfg {
};

const char license[] =
    "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
    "of this software and associated documentation files (the \"Software\"), to deal\n"
    "in the Software without restriction, including without limitation the rights\n"
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
    "copies of the Software, and to permit persons to whom the Software is\n"
    "furnished to do so, subject to the following conditions:\n"
    "\n"
    "The above copyright notice and this permission notice shall be included in\n"
    "all copies or substantial portions of the Software.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
    "THE SOFTWARE.\n";

static void print_copyright(void)
{
	printf("\n");
	printf(" qiprog - Reference implementation of the QiProg protocol\n\n");
	printf(" Copyright (C) 2013 Alexandru Gagniuc\n\n");
	printf("%s", license);
}

int open_device(struct qiprog_cfg *conf);

int main(int argc, char *argv[])
{
	int option_index = 0;
	int opt;
	struct qiprog_cfg *config;

	struct option long_options[] = {
		{"copyright", no_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	config = (void *)malloc(sizeof(*config));
	memset(config, 0, sizeof(*config));

	/*
	 * Parse arguments
	 */
	while (1) {
		opt = getopt_long(argc, argv, "c",
				  long_options, &option_index);

		if (opt == EOF)
			break;

		switch (opt) {
		case 'c':
			print_copyright();
			exit(EXIT_SUCCESS);
			break;
		default:
			break;
		}
	}

	/*
	 * Sanity-check arguments
	 */

	/*
	 * At this point, the arguments are sane.
	 */
	printf("  qiprog  Copyright (C) 2013 Alexandru Gagniuc\n\n"
	       "This program comes with ABSOLUTELY NO WARRANTY;\n"
	       "This is free software, and you are welcome to redistribute it\n"
	       "under certain conditions; invoke with `-c' for details.\n");

	return open_device(config);
}

/*
 * Print buses supported by the device.
 */
static void print_buses(uint32_t bus_master)
{
	if (!bus_master) {
		printf("Device does not support any known bus\n");
		return;
	}

	printf("Device supports");
	if (bus_master & QIPROG_BUS_ISA)
		printf(" ISA");
	if (bus_master & QIPROG_BUS_LPC)
		printf(" LPC");
	if (bus_master & QIPROG_BUS_FWH)
		printf(" FWH");
	if (bus_master & QIPROG_BUS_SPI)
		printf(" SPI");
	if (bus_master & QIPROG_BUS_AUD)
		printf(" AUD");
	if (bus_master & QIPROG_BUS_BDM17)
		printf(" BDM17");
	if (bus_master & QIPROG_BUS_BDM35)
		printf(" BDM35");

	printf("\n");
}

/*
 * Query and print the capabilities of the device
 */
static int print_device_info(struct qiprog_device *dev)
{
	int i;
	qiprog_err ret;
	struct qiprog_capabilities caps;

	ret = qiprog_get_capabilities(dev, &caps);
	if (ret != QIPROG_SUCCESS) {
		printf("Error querying device capabilities\n");
		return EXIT_FAILURE;
	}

	print_buses(caps.bus_master);
	for (i = 0; i < 10; i++) {
		if (caps.voltages[i] == 0)
			break;
		printf("Supported voltage: %imV\n", caps.voltages[i]);
	}
	/* Ignore caps.instruction_set and caps.max_direct_data */

	return EXIT_SUCCESS;
}

/*
 * Call different functions on the device and see if they fail or not
 */
static int stress_test_device(struct qiprog_device *dev)
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

/*
 * Open the first QiProg device to come our way.
 */
int open_device(struct qiprog_cfg *conf)
{
	size_t ndevs;
	qiprog_err ret;
	struct qiprog_context *ctx;
	struct qiprog_device **devs;

	(void)conf;

	/* Debug _everything_ */
	qiprog_set_loglevel(QIPROG_LOG_SPEW);

	if ((ret = qiprog_init(&ctx)) != QIPROG_SUCCESS) {
		printf("libqiprog initialization failure\n");
		return EXIT_FAILURE;
	}

	ndevs = qiprog_get_device_list(ctx, &devs);
	if (!ndevs) {
		printf("No device found\n");
		return EXIT_FAILURE;
	}

	ret = qiprog_open_device(devs[0]);
	if (ret != QIPROG_SUCCESS) {
		printf("Error opening device\n");
		return EXIT_FAILURE;
	}

	if (print_device_info(devs[0]) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (stress_test_device(devs[0]) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
