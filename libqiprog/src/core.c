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

#include "qiprog_internal.h"

/**
 * @ingroup discovery
 */
/** @{ */

/**
 * @brief Open a QiProg device
 *
 * Opens a QiProg device. This function may fail if the device is in use
 * somewhere else (another application or process). When this function succeeds,
 * The device can be used.
 *
 * This function must be called before using the device.
 *
 * @param[in] dev Device to operate on
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_open_device(struct qiprog_device *dev)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->dev_open(dev);
}

/**
 * @brief Query a device for its capabilities
 *
 * Asks the device what functionality it can handle. This includes the supported
 * bus types and programming voltages.
 *
 * It is recommended to query the device first, to see if its capabilities match
 * the need of the application.
 *
 * @param[in] dev Device to operate on
 * @param[out] caps Location where to store the capabilities.
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_get_capabilities(struct qiprog_device *dev,
				   struct qiprog_capabilities *caps)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->get_capabilities(dev, caps);
}

/** @} */

/**
 * @defgroup commands QiProg device commands
 *
 * @ingroup qiprog_public
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg device commands API</b>
 *
 */
/** @{ */

/**
 * @brief Set the bus on which a flash chip is expected to be connected
 *
 * Set the bus type (SPI, LPC, FWH, etc) on which a flash chip is connected. The
 * supported bus types can be queried with @ref qiprog_get_capabilities().
 *
 * Although QiProg devices may automatically set a default bus, they are not
 * required do so. Operating a device before setting a bus has undefined
 * behavior. Therefore, it is recommended to call this function to set a bus
 * type before doing any chip operations.
 *
 * @param[in] dev Device to operate on
 * @param[in] bus @ref qiprog_bus constant. May not logical OR bus types.
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_bus(struct qiprog_device *dev, enum qiprog_bus bus)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_bus(dev, bus);
}

/**
 * @brief Configure the clock speed used to communicate with the flash chip
 *
 * Some QiProg devices may configure the clock speed they use to communicate
 * with flash chips. QiProg devices are required to default to a safe speed,
 * however, choosing the default speed is left ti the device. When the default
 * speed is not appropriate for the task, it may be modified with this function
 * (for example, default speed is too fast for the flash chip, or operations
 * take too much time at the default speed).
 *
 * QiProg devices may implement this function for some supported bus types, but
 * are not required to implement it for all buses. The new clock is applied to
 * the current active bus. An application should call this function only after
 * calling @ref qiprog_set_bus.
 *
 * A failure from this function does not necessarily indicate an error
 * condition. It only means that the device does not implement a controllable
 * clock for the active bus type.
 *
 * @param[in] dev Device to operate on
 * @param[in] clock_khz clock speed in kilohertz.
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_clock(struct qiprog_device *dev, uint32_t *clock_khz)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_clock(dev, clock_khz);
}

/**
 * @brief TODO
 */
qiprog_err qiprog_set_spi_timing(struct qiprog_device *dev,
				 uint16_t tpu_read_us, uint32_t tces_ns)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_spi_timing(dev, tpu_read_us, tces_ns);
}

/**
 * @brief Set the voltage at which to operate the flash chip
 *
 * Set the voltage at which the flash chip is to be operated. The voltage must
 * be one of the voltages supported by the given QiProg device.
 *
 * @param[in] dev Device to operate on
 * @param[in] vdd_mv voltage in millivolts. This must be supported by the
 *		     QiProg device
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_vdd(struct qiprog_device *dev, uint16_t vdd_mv)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_vdd(dev, vdd_mv);
}

/** @} */

/**
 * @defgroup chip_io QiProg flash chip IO
 *
 * @ingroup qiprog_public
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @section io_main IO operations to and from connected flash chips
 *
 * Once a device has been successfully opened, and configured to a specific bus,
 * it is ready to communicate with attached flash chips.
 *
 * There are two mechanism for performing chio IO:
 *
 *
 * @section fine_io Fine-grained mechanism
 *
 * A fine-grained mechanism is provided for performing atomic, or
 * "nearly-atomic" IO operations. Reads to the chip's address space can be
 * performed with @ref qiprog_read8(), @ref qiprog_read16(), and
 * @ref qiprog_read32(), and writes with @ref qiprog_write8(),
 * @ref qiprog_write8(), and @ref qiprog_write32().
 *
 * These functions provide a convenient way to integrate QiProg functionality
 * into existing software, however they are limited by the latencies of
 * communicating with the QiProg device. For reading or writing to a device, it
 * is recommended to use the @ref bulk_io.
 *
 * The fine-grained mechanism is intended for delicate situations, where a
 * specific, non-standard sequence is required to put the chip in the desired
 * mode, or performed the desired operation.
 *
 * For example, to erase a sector on fictional "blackbox" chips:
 * @code{.c}
 *	// The "blackbox" chips have a proprietary erase sequence
 *	qiprog_write8(dev, control_regs_addr, 0x30);
 *	qiprog_write8(dev, control_regs_addr + sector_index, 0xd0);
 * @endcode
 *
 * @warning
 * Avoid using fine-grained IO for bulk operations. Do not try to read or write
 * the chip with this mechanism, as it it slow and inefficient.
 * @code{.c}
 *	// This is inefficient and slow
 *	for (i = start_addr; i < start_addr + chip_size; i++) {
 *		qiprog_read8(dev, i, &val8);
 *		dest[i] = val8;
 *	}
 * @endcode
 *
 *
 * @section bulk_io Bulk mechanism
 *
 * For operations which transport large amounts of data, it is preferable to do
 * so with one or a few bulk operations. Bulk operations are optimized for
 * transferring large amounts of data at a time, and are considerably faster
 * than the "bad" example above. These operations are useful when bulk reading
 * or bulk programming the flash chip.
 *
 * @warning
 * FIXME: set_address has been removed from the API. Update documentation
 * accordingly.
 *
 * The address range for an operation must first be specified with
 * @ref qiprog_set_address(). After that, the bulk operation can be started with
 * @ref qiprog_readn(), or @ref qiprog_writen(). The internal address of the
 * device is automatically incremented with every operation. There is no need
 * to call @ref qiprog_set_address() if performing successive bulk operations
 * on a linear memory space.
 *
 * For example:
 * @code{.c}
 *	// Read the secret number
 *	qiprog_set_address(dev, secret_number_addr, secret_number_addr + 0x100);
 *	qiprog_readn(dev, secret, 64);
 *	// If this is a version 2.0 secret, read the rest of it
 *	if (is_long_secret(secret))
 *		qiprog_readn(dev, secret + 64, 64);
 * @endcode
 *
 * It is also possible to read the entire contents of the chip in one pass:
 * @code{.c}
 *	// Now that we know what chip we are dealing with, read it
 *	qiprog_set_address(dev, start_addr, start_addr + chip_size);
 *	qiprog_readn(dev, &contents, chip_size);
 * @endcode
 */
/** @{ */

/**
 * @brief Try to identify connected flash chips
 *
 * @param[in] dev Device to operate on
 * @param[out] ids Location where to store device ids
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_read_chip_id(struct qiprog_device *dev,
			       struct qiprog_chip_id ids[9])
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read_chip_id(dev, ids);
}

qiprog_err qiprog_read(struct qiprog_device *dev, uint32_t where, void *dest,
		       uint32_t n)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read(dev, where, dest, n);
}

qiprog_err qiprog_write(struct qiprog_device *dev, uint32_t where, void *src,
			uint32_t n)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write(dev, where, src, n);
}

/**
 * @brief Inform the programmer of the erase geometry of the chip
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] types array of erase type enumerators, @ref qiprog_erase_type
 * @param[in] sizes array of sizes corresponding to each erase type in 'types'
 * @param[in] num_sizes the number of elements in the 'types' and 'sizes' arrays
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_erase_size(struct qiprog_device *dev, uint8_t chip_idx,
				 enum qiprog_erase_type *types, uint32_t *sizes,
				 size_t num_sizes)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_erase_size(dev, chip_idx, types, sizes, num_sizes);
}

/**
 * @brief Instruct the programmer to use a predefined erase sequence
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] cmd predefined command sequence to use for erase operations
 * @param[in] subcmd predefined subsequence. Use '0' for default.
 * @param[in] flags TODO @ref TODO
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_erase_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_erase_cmd cmd,
				    enum qiprog_erase_subcmd subcmd,
				    uint16_t flags)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_erase_command(dev, chip_idx, cmd, subcmd, flags);
}

/**
 * @brief Instruct the programmer to use a custom erase sequence
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] addr array of addresses to write data when sending commands
 * @param[in] data array of data to write corresponding to each address in
 *	 	   'addr'
 * @param[in] num_bytes the number of elements in the 'addr' and 'data' arrays
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_custom_erase_command(struct qiprog_device *dev,
					   uint8_t chip_idx,
					   uint32_t *addr, uint8_t *data,
					   size_t num_bytes)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_custom_erase_command(dev, chip_idx, addr, data,
						  num_bytes);
}

/**
 * @brief Instruct the programmer to use a predefined write sequence
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] cmd predefined command sequence to use for write operations
 * @param[in] subcmd predefined subsequence. Use '0' for default.
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_write_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_write_cmd cmd,
				    enum qiprog_write_subcmd subcmd)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_write_command(dev, chip_idx, cmd, subcmd);
}

/**
 * @brief Instruct the programmer to use a custom write sequence
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] addr array of addresses to write data when sending commands
 * @param[in] data array of data to write corresponding to each address in
 *	 	   'addr'
 * @param[in] num_bytes the number of elements in the 'addr' and 'data' arrays
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_custom_write_command(struct qiprog_device *dev,
					   uint8_t chip_idx,
					   uint32_t *addr, uint8_t *data,
					   size_t num_bytes)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_custom_write_command(dev, chip_idx, addr, data,
						  num_bytes);
}

/**
 * @brief Inform the programmer of the size of connected chips
 *
 * @param[in] dev Device to operate on
 * @param[in] chip_idx Index of chip in array returned by @ref read_chip_id
 * @param[in] size Size of chip in bytes
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_set_chip_size(struct qiprog_device *dev, uint8_t chip_idx,
				uint32_t size)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_chip_size(dev, chip_idx, size);
}
/**
 * @brief Read a byte from the flash chip
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address from where to read
 * @param[in] data Location where to store the result
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_read8(struct qiprog_device *dev, uint32_t addr, uint8_t *data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read8(dev, addr, data);
}

/**
 * @brief Read a word from the flash chip
 *
 * The data is read in LE format, and presented to the host in host-endian
 * format.
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address from where to read
 * @param[in] data Location where to store the result
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_read16(struct qiprog_device *dev, uint32_t addr,
			 uint16_t *data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read16(dev, addr, data);
}

/**
 * @brief Read a long word from the flash chip
 *
 * The data is read in LE format, and presented to the host in host-endian
 * format.
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address from where to read
 * @param[in] data Location where to store the result
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_read32(struct qiprog_device *dev, uint32_t addr,
			 uint32_t *data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read32(dev, addr, data);
}

/**
 * @brief Read a byte from the flash chip
 *
 * The must be given in host-endian format, and it will be written to the chip
 * in LE format.
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address where to write
 * @param[in] data Value to write
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_write8(struct qiprog_device *dev, uint32_t addr, uint8_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write8(dev, addr, data);
}

/**
 * @brief Read a word from the flash chip
 *
 * The must be given in host-endian format, and it will be written to the chip
 * in LE format.
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address where to write
 * @param[in] data Value to write
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */

/**
 * @brief
 */
qiprog_err qiprog_write16(struct qiprog_device *dev, uint32_t addr,
			  uint16_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write16(dev, addr, data);
}

/**
 * @brief Read a long word from the flash chip
 *
 * The must be given in host-endian format, and it will be written to the chip
 * in LE format.
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address where to write
 * @param[in] data Value to write
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err qiprog_write32(struct qiprog_device *dev, uint32_t addr,
			  uint32_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write32(dev, addr, data);
}

/** @} */
