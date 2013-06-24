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
 * @brief
 */
qiprog_err qiprog_open_device(struct qiprog_device *dev)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->dev_open(dev);
}

/**
 * @brief
 */
qiprog_err qiprog_get_capabilities(struct qiprog_device * dev,
				   struct qiprog_capabilities * caps)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->get_capabilities(dev, caps);
}

/** @} */

/**
 * @defgroup commands QiProg device commands
 *
 * @ingroup qiprog_api
 *
 * @author @htmlonly &copy; @endhtmlonly 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * @brief <b>QiProg device commands API</b>
 *
 */
/** @{ */

/**
 * @brief
 */
qiprog_err qiprog_set_bus(struct qiprog_device * dev, enum qiprog_bus bus)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_bus(dev, bus);
}

/**
 * @brief
 */
qiprog_err qiprog_set_clock(struct qiprog_device * dev, uint32_t * clock_khz)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_clock(dev, clock_khz);
}

/**
 * @brief
 */
qiprog_err qiprog_read_chip_id(struct qiprog_device * dev,
			       struct qiprog_chip_id ids[9])
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read_chip_id(dev, ids);
}

/**
 * @brief
 */
qiprog_err qiprog_set_address(struct qiprog_device * dev, uint32_t start,
			      uint32_t end)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);

	/*
	 * Don't set dev->curr_addr_range here. Let the device driver decide if
	 * it should update it or not.
	 */

	return dev->drv->set_address(dev, start, end);
}

/* TODO: qiprog_set_erase_size */
/* TODO: qiprog_set_erase_command */
/* TODO: qiprog_set_write_command */

/**
 * @brief
 */
qiprog_err qiprog_set_spi_timing(struct qiprog_device * dev,
				 uint16_t tpu_read_us, uint32_t tces_ns)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_spi_timing(dev, tpu_read_us, tces_ns);
}

/**
 * @brief
 */
qiprog_err qiprog_read8(struct qiprog_device * dev, uint32_t addr,
			uint8_t * data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read8(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_read16(struct qiprog_device * dev, uint32_t addr,
			 uint16_t * data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read16(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_read32(struct qiprog_device * dev, uint32_t addr,
			 uint32_t * data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->read32(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_write8(struct qiprog_device * dev, uint32_t addr,
			 uint8_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write8(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_write16(struct qiprog_device * dev, uint32_t addr,
			  uint16_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write16(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_write32(struct qiprog_device * dev, uint32_t addr,
			  uint32_t data)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->write32(dev, addr, data);
}

/**
 * @brief
 */
qiprog_err qiprog_set_vdd(struct qiprog_device * dev, uint16_t vdd_mv)
{
	QIPROG_RETURN_ON_BAD_DEV(dev);
	return dev->drv->set_vdd(dev, vdd_mv);
}

/** @} */
