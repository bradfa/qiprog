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
#include <libusb.h>

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

static void fill_string(libusb_device_handle * dev, uint8_t idx,
			unsigned char *str, int len)
{
	int ret;

	/* Start with a null string if anything fails */
	str[0] = '\0';
	/* index 0 is not valid string index */
	if (idx == 0)
		return;

	ret = libusb_get_string_descriptor_ascii(dev, idx, str, len);
	if (ret < LIBUSB_SUCCESS)
		printf("Error getting string descriptor: %s\n",
		       libusb_error_name(ret));
}

static int print_device_info(libusb_device_handle * handle)
{
	int ret;
	unsigned char string[256];
	struct libusb_device_descriptor dscr;
	struct libusb_device *dev;

	dev = libusb_get_device(handle);

	ret = libusb_get_device_descriptor(dev, &dscr);
	if (ret != LIBUSB_SUCCESS) {
		printf("Error getting device descriptor: %s\n",
		       libusb_error_name(ret));
		return EXIT_FAILURE;
	}

	fill_string(handle, dscr.iManufacturer, string, sizeof(string));
	printf("Manufacturer : %s\n", string);

	fill_string(handle, dscr.iProduct, string, sizeof(string));
	printf("Product      : %s\n", string);

	fill_string(handle, dscr.iSerialNumber, string, sizeof(string));
	printf("Serial Number: %s\n", string);

	return EXIT_SUCCESS;
}

int open_device(struct qiprog_cfg *conf)
{
	int ret;
	libusb_context *ctx;
	libusb_device_handle *handle;

	if ((ret = libusb_init(&ctx)) != LIBUSB_SUCCESS) {
		printf("Initialization error: %s\n", libusb_error_name(ret));
		return EXIT_FAILURE;
	}

	handle = libusb_open_device_with_vid_pid(ctx, conf->vid, conf->pid);
	if (handle == NULL) {
		printf("Cannot open device.\n");
		return EXIT_FAILURE;
	}

	if ((ret = libusb_claim_interface(handle, 0)) != LIBUSB_SUCCESS) {
		printf("Cannot claim device: %s\n", libusb_error_name(ret));
		return EXIT_FAILURE;
	}

	return print_device_info(handle);
}
