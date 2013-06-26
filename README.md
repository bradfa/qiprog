README
======

Under construction, please pardon the dust
------------------------------------------

This repository contains the reference implementation of the QiProg protocol.
QiProg is a specification for communicating with flash chip programmers over
USB. It was originally developed by Peter Stuge, and has since been extended to
a complete API for flash chip programmers. USB is used as a transport medium.

This reference implementation includes the USB host driver, which serializes
QiProg commands over a USB connection, and the USB device driver, which
de-serializes the USB streams into QiProg calls executed on the device.


Building
--------

### Prerequisites ###
* gcc or any working compiler
* cmake
* libusb

### Working with cmake ###

1. Create a build directory and cd into it:
> $ mkdir build; cd build

2. Now tell cmake to create the build system, pointing it to the source
    directory:
> $ cmake ..

3. Compile:
> $ make

### Documentation ###

The code is documented with Doxygen comments. If Doxygen is isntalled, cmake
will create a special 'doc' target for building documentation. To generate
the documentation, use:

> $ make doc



Usage
-----

* -c | --copyright: -- print license and exit
* -d | --device -- device to use, in the form VID:PID (i.e. c03e:b007)
    The default is 1d50:6076



Copyright notices
-----------------

A copyright notice indicating a range of years, must be interpreted as having
had copyrightable material added in each of those years.

For example:

> Copyright (C) 2010-2013 Contributor Name

is to be interpreted as

> Copyright (C) 2010,2011,2012,2013 Contributor Name
