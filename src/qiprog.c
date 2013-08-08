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

#include "tests.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <qiprog.h>


#define KiB	(1 << 10)
#define MiB	(1 << 20)

enum qi_action {
	NONE,
	ACTION_READ,
	ACTION_WRITE,
	ACTION_VERIFY,
	ACTION_TEST_DEV,
};

struct flash_chip {
	uint16_t vendor_id;
	uint32_t device_id;
	uint32_t size;
	const char *name;
};

/*
 * This is a minimal, example, list. It is not meant to be comprehensive,
 * neither in terms of chips, nor in terms of chip parameters it stores.
 */
const struct flash_chip const chip_list[] = {
	{
		.vendor_id = 0xbf,
		.device_id = 0x4c,
		.size = 2 * MiB,
		.name = "SST49LF160C",
	}, {

		.vendor_id = 0xbf,
		.device_id = 0x5b,
		.size = 1 * MiB,
		.name = "SST49LF080A",
	}, {

		.vendor_id = 0,
		.device_id = 0,
		.size = 0,
		.name = "",
	}
};

struct qiprog_cfg {
	char *filename;
	enum qi_action action;
	uint32_t chip_size;
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

int qiprog_run(struct qiprog_cfg *conf);

int main(int argc, char *argv[])
{
	int option_index = 0;
	int opt, ret;
	bool has_operation = false;
	struct qiprog_cfg *config;

	struct option long_options[] = {
		{"copyright",	no_argument,		0, 'c'},
		{"read",	required_argument,	0, 'r'},
		{"write",	required_argument,	0, 'w'},
		{"verify",	required_argument,	0, 'v'},
		{"test",	no_argument,		0, 't'},
		{0, 0, 0, 0}
	};

	config = (void *)malloc(sizeof(*config));
	memset(config, 0, sizeof(*config));

	/*
	 * Parse arguments
	 */
	while (1) {
		opt = getopt_long(argc, argv, "cr:w:v:st",
				  long_options, &option_index);

		if (opt == EOF)
			break;

		switch (opt) {
		case 'c':
			print_copyright();
			exit(EXIT_SUCCESS);
			break;
		case 'r':
			if (has_operation) {
				printf("More than one operation specified.\n");
				exit(EXIT_FAILURE);
			}
			has_operation = true;
			config->filename = strdup(optarg);
			config->action = ACTION_READ;
			break;
		case 'v':
			if (has_operation) {
				printf("More than one operation specified.\n");
				exit(EXIT_FAILURE);
			}
			has_operation = true;
			config->filename = strdup(optarg);
			config->action = ACTION_VERIFY;
			break;
		case 'w':
			if (has_operation) {
				printf("More than one operation specified.\n");
				exit(EXIT_FAILURE);
			}
			has_operation = true;
			config->filename = strdup(optarg);
			config->action = ACTION_WRITE;
			break;
		case 't':
			if (has_operation) {
				printf("More than one operation specified.\n");
				exit(EXIT_FAILURE);
			}
			has_operation = true;
			config->action = ACTION_TEST_DEV;
			break;
		default:
			/* Invalid option. getopt will have printed something */
			exit(EXIT_FAILURE);
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

	/* Do the deed */
	ret = qiprog_run(config);

	/* Clean up */
	if (config->filename)
		free(config->filename);
	free(config);

	return ret;
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
 * Identify the flash chip's properties based on the chip ID
 */
static int identify_chip(struct qiprog_device *dev, struct qiprog_cfg *conf)
{
	qiprog_err ret;
	struct qiprog_chip_id ids[9];
	const struct flash_chip *chip;

	/* Check if a chip is connected */
	ret = qiprog_read_chip_id(dev, ids);
	if (ret != QIPROG_SUCCESS) {
		printf("Error reading IDs of connected chips\n");
		return EXIT_FAILURE;
	}

	if (ids[0].id_method == 0) {
		printf("No flash chip connected to programmer\n");
		return EXIT_FAILURE;
	}

	/* Only look at the first identified chip */
	printf("Identified chip with ID %x:%x\n",
	       ids[0].vendor_id, ids[0].device_id);

	/* Now check our list of known chips */
	for (chip = chip_list; chip->size != 0; chip++) {
		if(ids[0].vendor_id != chip->vendor_id)
			continue;
		if(ids[0].device_id != chip->device_id)
			continue;
		/* Found it */
		conf->chip_size = chip->size;
		printf("Chip is a %s\n", chip->name);
		break;
	}

	/* If size is 0, then we do not know enough about this chip */
	if (conf->chip_size == 0) {
		printf ("Chip is not supported by this application\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 * Bulk read the flash chip into memory
 */
static int bulk_read(struct qiprog_device *dev, void *buf, size_t size)
{
	/* FIXME: Do not hardcode base address */
	const uint32_t top = 0xffffffff;
	const uint32_t base = top - size + 1;

	if (qiprog_set_address(dev, base, top) != QIPROG_SUCCESS) {
		printf("Failed to set bulk address\n");
		return EXIT_FAILURE;
	}

	/* Bulk read may take a while, so get ready for it */
	printf("Attempting to read flash chip...\n");
	fflush(stdout);

	/* Do the deed */
	if (qiprog_readn(dev, buf, size) != QIPROG_SUCCESS) {
		printf("Failed to bulk read chip\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 * Read chip to a file
 */
static int read_chip(struct qiprog_device *dev, const struct qiprog_cfg *conf)
{
	int ret;
	uint8_t *buf;
	FILE *file = NULL;

	/* Assume the worst */
	ret = EXIT_FAILURE;

	if ((buf = malloc(conf->chip_size)) == NULL) {
		printf("Cannot allocate memory\n");
		goto cleanup;
	}

	if ((file = fopen(conf->filename, "w")) == NULL) {
		printf("Cannot open file \"%s\"\n", conf->filename);
		goto cleanup;
	}

	/* Do the deed */
	if (bulk_read(dev, buf, conf->chip_size) != EXIT_SUCCESS)
		goto cleanup;

	/* Finish the deed */
	fwrite(buf, 1, conf->chip_size, file);

	/* All is good */
	ret = EXIT_SUCCESS;

 cleanup:
	free(buf);
	if (file)
		fclose(file);
	return ret;
}

/*
 * Bulk write the flash chip
 */
static int bulk_write(struct qiprog_device *dev, void *data, size_t size)
{
	/* FIXME: Do not hardcode base address */
	const uint32_t top = 0xffffffff;
	const uint32_t base = top - size + 1;

	if (qiprog_set_address(dev, base, top) != QIPROG_SUCCESS) {
		printf("Failed to set bulk address\n");
		return EXIT_FAILURE;
	}

	/* Bulk read may take a while, so get ready for it */
	printf("Attempting to read flash chip...\n");
	fflush(stdout);

	/* Do the deed */
	if (qiprog_writen(dev, data, size) != QIPROG_SUCCESS) {
		printf("Failed to bulk write chip\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 * Write file contents to chip
 */
static int write_chip(struct qiprog_device *dev, const struct qiprog_cfg *conf)
{
	int ret;
	void *buf = NULL;
	FILE *file;
	size_t size;
	long int file_size;

	/* Assume the worst */
	ret = EXIT_FAILURE;

	if ((file = fopen(conf->filename, "r")) == NULL) {
		printf("Cannot open file \"%s\"\n", conf->filename);
		goto cleanup;
	}

	/* Use the file size to tell how much to read */
	fseek(file, 0L, SEEK_END);
	if ((file_size = ftell(file)) < 0)
		goto cleanup;
	rewind(file);

	size = (size_t) file_size;
	if (size != conf->chip_size) {
		printf("File size of %lu is different than chip size of %lu\n",
		       (unsigned long)size, (unsigned long)conf->chip_size);
		goto cleanup;
	}

	if ((buf = malloc(size)) == NULL) {
		printf("Cannot allocate memory\n");
		goto cleanup;
	}

	/* Get the file contents */
	fread(buf, 1, size, file);

	/* Now the chip contents */
	if (bulk_write(dev, buf, size) != EXIT_SUCCESS)
		goto cleanup;

	/* All is good */
	ret = EXIT_SUCCESS;

 cleanup:
	free(buf);
	if (file)
		fclose(file);
	return ret;
}

/*
 * Verify contents of chip against file
 */
static int verify_chip(struct qiprog_device *dev, const struct qiprog_cfg *conf)
{
	int ret;
	void *buf = NULL, *chip = NULL;
	FILE *file;
	size_t size;
	long int file_size;

	/* Assume the worst */
	ret = EXIT_FAILURE;

	if ((file = fopen(conf->filename, "r")) == NULL) {
		printf("Cannot open file \"%s\"\n", conf->filename);
		goto cleanup;
	}

	/* Use the file size to tell how much to read */
	fseek(file, 0L, SEEK_END);
	if ((file_size = ftell(file)) < 0)
		goto cleanup;
	rewind(file);

	size = (size_t) file_size;
	if (size != conf->chip_size) {
		printf("File size of %lu is different than chip size of %lu\n",
		       (unsigned long)size, (unsigned long)conf->chip_size);
		goto cleanup;
	}

	buf = malloc(size);
	chip = malloc(size);

	if (buf == NULL || chip == NULL) {
		printf("Cannot allocate memory\n");
		goto cleanup;
	}

	/* Get the file contents */
	fread(buf, 1, size, file);

	/* Now the chip contents */
	if (bulk_read(dev, chip, size) != EXIT_SUCCESS)
		goto cleanup;

	if (memcmp(chip, buf, size))
		printf("Verification failed. Contents differ.\n");
	else
		printf("Match!!!\n");

	/* All is good */
	ret = EXIT_SUCCESS;

 cleanup:
	free(buf);
	free(chip);
	if (file)
		fclose(file);
	return ret;
}

/*
 * Open the first QiProg device to come our way.
 */
int qiprog_run(struct qiprog_cfg *conf)
{
	int ret;
	size_t ndevs;
	struct qiprog_context *ctx = NULL;
	struct qiprog_device **devs = NULL;
	struct qiprog_device *dev = NULL;

	/* Debug _everything_ */
	qiprog_set_loglevel(QIPROG_LOG_SPEW);

	if (qiprog_init(&ctx) != QIPROG_SUCCESS) {
		printf("libqiprog initialization failure\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	ndevs = qiprog_get_device_list(ctx, &devs);
	if (!ndevs) {
		printf("No device found\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* Choose the first device for now */
	dev = devs[0];

	if (qiprog_open_device(dev) != QIPROG_SUCCESS) {
		printf("Error opening device\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	if (print_device_info(dev) != EXIT_SUCCESS) {
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* Fail unless told otherwise */
	ret = EXIT_FAILURE;

	if (identify_chip(dev, conf) != EXIT_SUCCESS) {
		/* identify_chip() will print an error message */
		ret = EXIT_FAILURE;
		goto cleanup;
	}
	/*
	 * Dispatch operation
	 */
	switch (conf->action) {
	case ACTION_TEST_DEV:
		ret = run_tests(dev);
		break;
	case ACTION_READ:
		ret = read_chip(dev, conf);
		break;
	case ACTION_WRITE:
		ret = write_chip(dev, conf);
		break;
	case ACTION_VERIFY:
		ret = verify_chip(dev, conf);
		break;
	default:
		/* Do nothing */
		break;
	}

 cleanup:
	if (dev) {
		/* TODO: close device */
	}
	if (devs) {
		/* TODO: clean up device list */
	}
	if (ctx)
		qiprog_exit(ctx);

	return ret;
}
