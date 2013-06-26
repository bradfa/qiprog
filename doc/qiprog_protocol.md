QiProg USB flash chip programming protocol
==========================================

When wValue and/or wIndex are not specified they are not used and must be 0!


Constants
------------------------

#### QIPROG_LANG_ ####

Specify different instruction sets supported on EP 2 OUT.

	#define QIPROG_LANG_SPI		(1 << 0)	/* 1 */

#### QIPROG_BUS_ ####

Specify different bus types supported by QiProg devices.

	#define QIPROG_BUS_ISA		(1 << 0)	/* 1 */
	#define QIPROG_BUS_LPC		(1 << 1)	/* 2 */
	#define QIPROG_BUS_FWH		(1 << 2)	/* 4 */
	#define QIPROG_BUS_SPI		(1 << 3)	/* 8 */
	#define QIPROG_BUS_BDM17	(1 << 4)	/* 16 */
	#define QIPROG_BUS_BDM35	(1 << 5)	/* 32 */
	#define QIPROG_BUS_AUD		(1 << 6)	/* 64 */


Control transfers (all with Type Vendor, Recipient Device)
----------------------------------------------------------

Note that all values transferred over the bus, including those in wValue,
wIndex and wLength fields, are LE coded! (The host side API likely
translates wLength automatically if neccessary, but maybe not wValue and
wIndex, and certainly not the data. Check that your bytes are not swapped.)

##### qiprog_get_capabilities #####

* bRequest=0x00 QIPROG_GET_CAPABILITIES
*  bmRequestType=0xc0 (IN)
*  wValue=0x00
*  wIndex=0x00
*  data: 30 bytes packed struct

	struct qiprog_capabilities {
		/* bitwise OR of supported QIPROG_LANG_ bits */
		uint16_t instruction_set;
		/* bitwise OR of supported QIPROG_BUS_ bits */
		uint32_t bus_master;
		uint32_t max_direct_data;
		uint16_t voltages[10];
	};

Note that a QiProg device may not support any instruction set, in which case
capabilities.instruction_set=0, and in which case the interface with
bInterfaceNumber=0 does not have to include EP 2 OUT and EP 2 IN. Those
endpoints are then free to be used in another interface in the device, not
covered by this protocol description.


The capabilities.voltages array indicates which voltages the QiProg device can
supply, in millivolt (mV) units. the array ends on the first 0-value, or if no
0-value is present then the array contains exactly 10 voltages.

capabilities.max_direct_data contains the maximum number of bytes that can be
stored by a QiProg device using the EP 2 OUT instruction set.

##### qiprog_set_bus #####

* bRequest=0x01 QIPROG_SET_BUS
*  bmRequestType=0x40 (OUT)
*  wValue=most significant 16 bits of the QIPROG_BUS_ constant
*  wIndex=least significant 16 bits of the QIPROG_BUS_ constant
*  wLength=0x00

##### qiprog_set_clock #####

* bRequest=0x02 QIPROG_SET_CLOCK
*  bmRequestType=0xc0 (IN)
*  # Set nearest lower or equal output clock frequency
*  # default to 20MHz for SPI
*  # FIXME decide? : default to 33MHz for LPC
*  # FIXME keep this? : Use wValue=wIndex=0 to only GET current clock speed
*  wValue=most significant 16 bits of the maximum desired frequency in kHz
*  wIndex=least significant 16 bits of the maximum desired frequency in kHz
*  wLength=0x02
*  data: uint32_t actual clock frequency in kHz used by the QiProg device

##### qiprog_read_chip_id #####

* bRequest=0x03 QIPROG_READ_DEVICE_ID
*  bmRequestType=0xc0 (IN)
*  wLength=0x3f
*  # identify the connected flash chip
*  data: array[9] of 7 bytes packed struct

	struct qiprog_chip_id {
		uint8_t id_method;
		uint16_t vendor_id;
		uint32_t device_id;
	}

id_method = 0x01 JEDEC ISA, 0x20 SPI RES, 0x21 SPI REMS, 0x22 SPI TODO?
The array ends on the first 0-value, or if no 0-value is present then
the array contains exactly 9 device ids.


##### qiprog_set_address #####

* bRequest=0x04 QIPROG_SET_ADDRESS
*  bmRequestType=0x40 (OUT)
*  wLength=0x08
*  # EP 1 bulk transfers increase the firmware-internal "current address"
*  # value, this control transfer (re)sets that value so that next EP 1
*  # transfer after this request has succeeded begins at start_address.
*  data: 8 bytes packed

	struct qiprog_address{
		uint32_t start_address;
		uint32_t max_address;
	}

##### qiprog_set_erase_size #####

* bRequest=0x05 QIPROG_SET_ERASE_SIZE
*  bmRequestType=0x40 (OUT)
*  wLength=TODO
*  # specify the erase block size
*  data: TODO! {need efficient representation of eraseblocks in the chip}

##### qiprog_set_erase_command #####

* bRequest=0x06 QIPROG_SET_ERASE_COMMAND
*  bmRequestType=0x40 (OUT)
*  wLength=TODO
*  # how to erase one block
*  data: TODO! {what bytes to send and what to wait for using instruction set}

##### qiprog_set_write_command #####

* bRequest=0x07 QIPROG_SET_WRITE_COMMAND
*  bmRequestType=0x40 (OUT)
*  wLength=TODO
*  # how to write one block
*  data: TODO! {what bytes to send and what to wait for using instruction set,
    including prelude and postlude!}

A _SET_WRITE_SIZE request is not neccessary, since the write command specified
by the data here includes information about how many data bytes the command can
write.

##### qiprog_set_spi_timing #####

* bRequest=0x20 QIPROG_SET_SPI_TIMING
*  bmRequestType=0x40 (OUT)
*  # Set SPI flash chip TPU-READ and TCES (Power-Up timing and
*  # Chip Enable Setup time, see datasheet)
*  # the time to wait for the chip to come live after powering on VDD and
*  # after selecting the chip
*  wValue=TPU-READ in µs (default:50)
*  wIndex=TCES in ns (default:100)
*  wLength=0x00

##### qiprog_read8 #####

* bRequest=0x30 QIPROG_READ8
*  bmRequestType=0xc0 (IN)
*  wValue=most significant 16 bits of the memory address to read from
*  wIndex=least significant 16 bits of the memory address to read from
*  wLength=number of uint8_t to read from given memory address (0 < n <= 64)
*  data=memory contents at given address (may be fewer uint8_t than requested)

##### qiprog_read16 #####

* bRequest=0x31 QIPROG_READ16
*  bmRequestType=0xc0 (IN)
*  wValue=most significant 16 bits of the memory address to read from
*  wIndex=least significant 16 bits of the memory address to read from
*  wLength=number of uint16_t to read from given memory address (0 < n <= 32)
*  data=memory contents at given address (may be fewer uint16_t than requested)

##### qiprog_read32 #####

* bRequest=0x32 QIPROG_READ32
*  bmRequestType=0xc0 (IN)
*  wValue=most significant 16 bits of the memory address to read from
*  wIndex=least significant 16 bits of the memory address to read from
*  wLength=number of uint32_t to read from given memory address (0 < n <= 16)
*  data=memory contents at given address (may be fewer uint16_t than requested)

##### qiprog_write8 #####

* bRequest=0x33 QIPROG_WRITE8
*  bmRequestType=0x40 (OUT)
*  wValue=most significant 16 bits of the memory address to write to
*  wIndex=least significant 16 bits of the memory address to write to
*  wLength=number of uint8_t to read from given memory address (0 < n <= 64)
*  data=new memory contents to write

##### qiprog_write16 #####

* bRequest=0x34 QIPROG_WRITE16
*  bmRequestType=0x40 (OUT)
*  wValue=most significant 16 bits of the memory address to write to
*  wIndex=least significant 16 bits of the memory address to write to
*  wLength=number of uint16_t to read from given memory address (0 < n <= 32)
*  data=new memory contents to write

##### qiprog_write32 #####

* bRequest=0x35 QIPROG_WRITE32
*  bmRequestType=0x40 (OUT)
*  wValue=most significant 16 bits of the memory address to write to
*  wIndex=least significant 16 bits of the memory address to write to
*  wLength=number of uint32_t to read from given memory address (0 < n <= 16)
*  data=new memory contents to write

##### qiprog_set_voltage #####

* bRequest=0xf0 QIPROG_SET_VDD
*  bmRequestType=0x40 (OUT)
*  wValue=voltage in mV, must exist in capabilities.voltages[] and be > 0
*  wIndex=0:supply off  1:supply on
*  wLength=0x00


Bulk transfers (bInterfaceNumber 0, bInterfaceClass 0xff)
---------------------------------------------------------

* EP 1 OUT  Erase and write flash chip

  Writes bytes to the flash chip and increases the current address counter.
  The firmware will know when to add command bytes before sending out this
  data on SPI and other buses, and will know when to poll for a success/fail
  status after writing a full program unit of data.

* EP 1 IN   Read flash chip

  Reads bytes from the flash chip and increases the current address counter.

* EP 2 OUT

  This endpoint understands an "instruction set" for performing arbitrary
  bus operations. Any generated data is buffered by firmware (up to
  capabilities.max_direct_data bytes) and on success the data can be read
  from EP 2 IN. TODO! Define the instruction set!

* EP 2 IN

  Return data (up to capabilities.max_direct_data bytes) received during the
  previous high-level bus operation issued on EP 2 OUT.


Instruction set
----------------
TODO
