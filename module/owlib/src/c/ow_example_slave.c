/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* Example slave -- 1-wire slave example to show developers how to write support for a new 1-wire device */

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
	( or a closely related family of chips )

	The connection to the larger program is through the "device" data structure,
	  which must be declared in the acompanying header file.

	The device structure holds the
	  family code,
	  name,
	  number of properties,
	  list of property structures, called "filetype".

	Each filetype structure holds the
	  name,
	  estimated length (in bytes),
	  aggregate structure pointer,
	  data format,
	  read function,
	  write funtion,
	  generic data pointer

	The aggregate structure, is present for properties that several members
	(e.g. pages of memory or entries in a temperature log. It holds:
	  number of elements
	  whether the members are lettered or numbered
	  whether the elements are stored together and split, or separately and joined
*/

// Example_slave

/* Notes for a new slave device -- what files to change

A. Property functions:
  1. Create this file with READ_FUNCTION and WRITE_FUNCTION for the different properties
  2. Create the filetype struct for each property
  3. Create the DeviceEntry or DeviceEntryExtended struct
  4. Write all the appropriate property functions
  5. Add any visibility functions if needed
  6. Add this file and the header file to the CVS

B. Header
  1. Create a similarly named header file in ../include
  2. Protect inclusion with #ifdef / #define / #endif
  3. Link the DeviceHeader* to the include file

C. Makefile and include
  1. Add this file to Makefile.am
  2. Add the header to ../include/Makefile.am
  3. Add the header file to ../include/devices.h

D. Device tree
  1. Add the appropriate entry to the device tree in
  * ow_tree.c
  *
  SUMMARY:
  * create the slave_support file (this one) and the (simple) header file
  * add references to the slave in ow_tree.c ow_devices.h and both Makefile.am
  * Add the files the the CVS repository

*/


#include <config.h>
#include "owfs_config.h"
#include "ow_example_slave.h"

/* ------- Prototypes ----------- */

/* Here are the functions that perform the tasks of the listed properties
 * Unually there is on function for each read and write task, but there is
 * a data field that can be used to distinguish similar tasks and pool the code
 *
 * The functions get the onw_wire_query structure that contains buffers for data or results
 * and the parsedname structure that contains information on the name, serial number,
 * extra data and extension
 *
 * sometimes it makes sense to have an internal (hidden) field that corresponds better
 * to the devices registers, and different "linked" properties that access the register
 * for presentation
 */

/* Example slave */
READ_FUNCTION(FS_r_read_number);
READ_FUNCTION(FS_r_read_bits);
READ_FUNCTION(FS_r_is_index_prime);
READ_FUNCTION(FS_r_extension_characters);

/* ------- Structures ----------- */

/* Here are the "aggregate" structures
 * For properties that have an extension
 * e.g. PIO.A PIO.B
 * one set of functions can be used to manage the data
 *
 * There are several aggregate types:
 * 1. aggregate -- a single internal value is access on the slave, but it's also addressable separately
 *              And example would be an 8-bit register that corresponds to 8 pio pins. Read set and cleared
 *              together by the chip, but owfs can use then separately or as a single entity
 *              e.g. DS2408 PIO pins
 * 2. separate -- a set of values that have similar structure, but are treated separately internally
 *              owfs can pretend they are individual or unified
 *              e.g. DS2450 voltage readings
 * 3. sparse   -- the "extension" the value after the dot, isn't really an index, but rather a field that can
 *              be used in the property processing. Perhaps a memory location or a parameter setting
 *              An example could be a baud rate or parity setting.
 *              Clearly there is no "ALL" since the range of the extension is essentially unknown and unlimitted.
 */
static struct aggregate AExample_bits = { 8, ag_numbers, ag_aggregate, };
static struct aggregate AExample_prime = { 0, ag_numbers, ag_sparse, };
static struct aggregate AExample_chars = { 0, ag_letters, ag_sparse, };
static struct filetype Example_slave[] = {
	F_STANDARD,
	{"read_number", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_read_number, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"bits", PROPERTY_LENGTH_BITFIELD, &AExample_bits, ft_bitfield, fc_stable, FS_r_read_bits, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"is_index_prime", PROPERTY_LENGTH_YESNO, &AExample_prime, ft_yesno, fc_volatile, FS_r_is_index_prime, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"extension_characters", PROPERTY_LENGTH_UNSIGNED, &AExample_chars, ft_unsigned, fc_volatile, FS_r_extension_characters, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(DD, Example_slave, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _EXAMPLE_SLAVE_DEFAULT_VALUE 42

/* ------- Functions ------------ */

/* Simple number read */
static ZERO_OR_ERROR FS_r_read_number(struct one_wire_query *owq)
{
	// Simple function that returns an integer.
	// More usual case would be to send instructions to the slave over the 1-wire bus
	// and return the (interpreted) result.

	OWQ_I(owq) = _EXAMPLE_SLAVE_DEFAULT_VALUE ;
	// OWQ structure holds the result. the format (integer in this case) comes from a
	// property field (ft_integer in this case)

	return 0; // no error
}

/* Simple bitfield */
static ZERO_OR_ERROR FS_r_read_bits(struct one_wire_query *owq)
{
	// Simple function that returns a bitfield.
	// This means it's an array of bits and can be accessed as
	// bit list: bits.ALL
	// unsigned number: bits.BYTE
	// or individual bits: bits.0, ... bits.7

	// Note that all this handling is done at a higher level
	// All we need to do here is return a single unsigned number

	OWQ_U(owq) = _EXAMPLE_SLAVE_DEFAULT_VALUE ;
	// OWQ structure holds the result. the format (integer in this case) comes from a
	// property field (ft_integer in this case)

	return 0; // no error
}

/* Sparse numeric indexes */
static ZERO_OR_ERROR FS_r_is_index_prime(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ; // Get the parsename reference
	int array_index ; // array index

	// Here we'll use the index as a number (integer) and test whether the number is prime
	// The index is not an array index.
	// There is no pre-defined number of elements
	// There is no ".ALL" function available

	// The index string was parsed and converted to a number, but
	// no bounds checking was done (unlike a normal array).

	array_index = pn->extension ;

	// Prime number test
	// pick off negatives, 0, 1, 2, evens
	// Then test by dividing by odds up to square root
	if ( array_index < 1 ) {
		return -EINVAL ;
	} else if ( array_index == 1 ) {
		OWQ_Y(owq) = 1 ; // 1 is prime
	} else if ( array_index == 2 ) {
		OWQ_Y(owq) = 1 ; // 2 is prime
	} else if ( array_index % 2 == 0 ) {
		OWQ_Y(owq) = 0 ; // even is not prime
	} else {
		int idiv = 1 ; // test devisor

		OWQ_Y(owq) = 1 ; // assume prime

		do {
			idiv += 2 ; // still odd
			if ( array_index % idiv == 0 ) {
				OWQ_Y(owq) = 0 ; // non-prime
				break ;
			}
		} while ( idiv < (array_index/idiv) ) ;
	}

	return 0;
}

// Number of characters in extension index
static ZERO_OR_ERROR FS_r_extension_characters(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ; // Get the parsename reference

	// This is another sparse array -- no ALL or max element
	// Even more, it is really a hash array -- the index is text.
	// We can use the text however we want.

	// Unlike normal arrays, any parsing and bounds checking is up to us.
	if ( pn->sparse_name == NULL ) {
		return -EINVAL ;
	}

	// In this case, just return the string length
	OWQ_U(owq) = strlen( pn->sparse_name ) ;
	return 0;
}
