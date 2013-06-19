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

/* FIXME: move to QiProg API when appropriate */
#define VULTUREPROG_USB_VID	0x1d50
#define VULTUREPROG_USB_PID	0x6076

struct qiprog_cfg {
	uint16_t pid;
	uint16_t vid;
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
	int opt, ret;
	unsigned int svid = 0, spid = 0;
	bool has_device = false;
	struct qiprog_cfg *config;

	struct option long_options[] = {
		{"copyright", no_argument, 0, 'c'},
		{"device", required_argument, 0, 'd'},
		{0, 0, 0, 0}
	};

	config = (void *)malloc(sizeof(*config));
	memset(config, 0, sizeof(*config));

	/*
	 * Parse arguments
	 */
	while (1) {
		opt = getopt_long(argc, argv, "cd:",
				  long_options, &option_index);

		if (opt == EOF)
			break;

		switch (opt) {
		case 'c':
			print_copyright();
			exit(EXIT_SUCCESS);
			break;
		case 'd':
			/* VIP:PID: see if it makes sense */
			ret = sscanf(optarg, "%x:%x", &svid, &spid);
			if (ret != 2) {
				printf("Invalid argument: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			has_device = true;
			break;
		default:
			break;
		}
	}

	/*
	 * Sanity-check arguments
	 */

	/* Default to VultureProg VID:PID */
	if (!has_device) {
		svid = VULTUREPROG_USB_VID;
		spid = VULTUREPROG_USB_PID;
	}
	if ((svid > 0xffff) || (spid > 0xffff)) {
		printf("Invalid device %.4x:%.4x\n", svid, spid);
		exit(EXIT_FAILURE);
	}

	config->vid = svid;
	config->pid = spid;

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
	if (!bus_master)
		printf("Device does not support any known bus\n");

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
 * Open the first QiProg device to come our way.
 */
int open_device(struct qiprog_cfg *conf)
{
	size_t ndevs;
	qiprog_err ret;
	struct qiprog_context *ctx;
	struct qiprog_device **devs;

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

	return print_device_info(devs[0]);
}
