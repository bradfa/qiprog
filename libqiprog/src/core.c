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

/** @} */
