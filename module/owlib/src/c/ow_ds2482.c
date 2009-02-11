/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
 */
/* i2c support for the DS2482-100 and DS2482-800 1-wire host adapters */
/* Stolen shamelessly from Ben Gardners kernel module */
/* Actually, Dallas datasheet has the information, 
   the module code showed a nice implementation,
   the eventual format is owfs-specific (using similar primatives, data structures)
   Testing by Jan Kandziora and Daniel Höper.
 */
/**
 * ds2482.c - provides i2c to w1-master bridge(s)
 * Copyright (C) 2005  Ben Gardner <bgardner@wabtec.com>
 *
 * The DS2482 is a sensor chip made by Dallas Semiconductor (Maxim).
 * It is a I2C to 1-wire bridge.
 * There are two variations: -100 and -800, which have 1 or 8 1-wire ports.
 * The complete datasheet can be obtained from MAXIM's website at:
 *   http://www.maxim-ic.com/quick_view2.cfm/qv_pk/4382
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * MODULE_AUTHOR("Ben Gardner <bgardner@wabtec.com>");
 * MODULE_DESCRIPTION("DS2482 driver");
 * MODULE_LICENSE("GPL");
 */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

#if OW_I2C
// Header taken from lm-sensors code
// specifically lm-sensors-2.10.0
#include "i2c-dev.h"

enum ds2482_address { 
	ds2482_any=-2, 
	ds2482_all=-1, 
	ds2482_18, ds2482_19, ds2482_1A, ds2482_1B, ds2482_1C, ds2482_1D, ds2482_1E, ds2482_1F, 
	ds2482_too_far 
	} ;

static enum ds2482_address Parse_i2c_address( struct connection_in * in ) ;
static int DS2482_detect_bus(enum ds2482_address chip_num, struct connection_in *in) ;
static int DS2482_detect_sys(enum ds2482_address chip_num, struct connection_in *in) ;
static int DS2482_detect_dir(enum ds2482_address chip_num, struct connection_in *in) ;
static int DS2482_detect_single(int lowindex, int highindex, struct connection_in *in) ;
static int DS2482_next_both(struct device_search *ds, const struct parsedname *pn);
static int DS2482_triple(BYTE * bits, int direction, int file_descriptor);
static int DS2482_send_and_get(int file_descriptor, const BYTE wr, BYTE * rd);
static int DS2482_reset(const struct parsedname *pn);
static int DS2482_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void DS2482_setroutines(struct connection_in *in);
static int HeadChannel(struct connection_in *in);
static int CreateChannels(struct connection_in *in);
static int DS2482_channel_select(const struct parsedname *pn);
static int DS2482_readstatus(BYTE * c, int file_descriptor, unsigned long int min_usec, unsigned long int max_usec);
static int SetConfiguration(BYTE c, struct connection_in *in);
static void DS2482_close(struct connection_in *in);
static int DS2482_redetect(const struct parsedname *pn);
static int DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);

/**
 * The DS2482 registers - there are 3 registers that are addressed by a read
 * pointer. The read pointer is set by the last command executed.
 *
 * To read the data, issue a register read for any address
 */
#define DS2482_CMD_RESET               0xF0	/* No param */
#define DS2482_CMD_SET_READ_PTR        0xE1	/* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_CHANNEL_SELECT      0xC3	/* Param: Channel byte - DS2482-800 only */
#define DS2482_CMD_WRITE_CONFIG        0xD2	/* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET         0xB4	/* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT    0x87	/* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE    0xA5	/* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE     0x96	/* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET       0x78	/* Param: Dir byte (bit7) */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_PTR_CODE_STATUS         0xF0
#define DS2482_PTR_CODE_DATA           0xE1
#define DS2482_PTR_CODE_CHANNEL        0xD2	/* DS2482-800 only */
#define DS2482_PTR_CODE_CONFIG         0xC3

/**
 * Configure Register bit definitions
 * The top 4 bits always read 0.
 * To write, the top nibble must be the 1's compl. of the low nibble.
 */
#define DS2482_REG_CFG_1WS     0x08
#define DS2482_REG_CFG_SPU     0x04
#define DS2482_REG_CFG_PPM     0x02
#define DS2482_REG_CFG_APU     0x01

/**
 * Status Register bit definitions (read only)
 */
#define DS2482_REG_STS_DIR     0x80
#define DS2482_REG_STS_TSB     0x40
#define DS2482_REG_STS_SBR     0x20
#define DS2482_REG_STS_RST     0x10
#define DS2482_REG_STS_LL      0x08
#define DS2482_REG_STS_SD      0x04
#define DS2482_REG_STS_PPD     0x02
#define DS2482_REG_STS_1WB     0x01

/* Time limits for communication
    unsigned long int min_usec, unsigned long int max_usec */
#define DS2482_Chip_reset_usec   1, 2
#define DS2482_1wire_reset_usec   1125, 1250
#define DS2482_1wire_write_usec   530, 585
#define DS2482_1wire_triplet_usec   198, 219

/* Search for a ":" in the name
   Change it to a null,and parse the remaining text as either
   null, a number, or nothing
*/
static enum ds2482_address Parse_i2c_address( struct connection_in * in )
{ 
	enum ds2482_address address ;
	char * colon = strchr( in->name, ':' ) ;
	if ( colon == NULL ) { // not found
		return ds2482_any ;
	}

	colon[0] = 0x00 ; // set as null
	++colon ; // point beyond
	
	if ( strcasecmp(colon,"all")==0 ) {
		return ds2482_all ;
	}

	address = atoi( colon ) ;
	
	if ( address < ds2482_18 || address >= ds2482_too_far ) {
		return ds2482_any ; // bad entry ignored
	}
	
	return address ;
}

/* Device-specific functions */
static void DS2482_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS2482_detect;
	in->iroutines.reset = DS2482_reset;
	in->iroutines.next_both = DS2482_next_both;
	in->iroutines.PowerByte = DS2482_PowerByte;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = DS2482_sendback_data;
	in->iroutines.sendback_bits = NULL;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = DS2482_redetect;
	in->iroutines.close = DS2482_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_overdrive;
	in->bundling_length = I2C_FIFO_SIZE;
}

/* All the rest of the program sees is the DS2482_detect and the entry in iroutines */
/* Open a DS2482 */
/* Try to see if there is a DS2482 device on the specified i2c bus */
int DS2482_detect(struct connection_in *in)
{
	enum ds2482_address chip_num ;
	int any ;
	int all ;

	// find the specific i2c address specified after the ":"
	// Alters the name by changing ':' to NULL
	chip_num = Parse_i2c_address( in ) ;

	any = ( in->name[0] == 0x00 ) ; // no adapter specified, so use any (the first good one )
	all = ( strcasecmp( in->name, "all" ) == 0 ) ;
	if ( !any && !all ) {
		// traditional, actual bus specified
		return DS2482_detect_bus( chip_num, in ) ;
	}

	switch ( DS2482_detect_sys( chip_num, in ) ) {
		case -1:
			return DS2482_detect_dir( chip_num, in ) ? -ENODEV : 0 ;
		case 0:
			return -ENODEV ;
		default:
			return 0 ;
	}
}

/* All the rest of the program sees is the DS2482_detect and the entry in iroutines */
/* Open a DS2482 */
/* cycle through /sys/class/i2c-adapter */
/* returns -1 -- no directory could be opened 
   else found -- number of directories found */
static int DS2482_detect_sys(enum ds2482_address chip_num, struct connection_in *in)
{
	DIR * i2c_list_dir ;
	struct dirent * i2c_bus ;
	int found = 0 ;
	int any = ( in->name[0] == 0x00 ) ; // no adapter specified, so use any (the first good one )
	struct connection_in * all_in = in ;

	// We'll look in this directory for available i2c adapters.
	// This may be linux 2.6 specific
	i2c_list_dir = opendir( "/sys/class/i2c-adapter" ) ;
	if ( i2c_list_dir == NULL ) {
		ERROR_CONNECT( "Cannot open /sys/class/i2c-adapter to find available i2c devices" ) ;
		return -1 ;
	}

	while ( (i2c_bus=readdir(i2c_list_dir)) != NULL ) {
		char * new_device = malloc( strlen(i2c_bus->d_name) + 7 ) ; // room for /dev/name
		if ( new_device==NULL ) {
			break ; // cannot make space
		}

		// Change name to real i2c bus name
		if ( in->name ) {
			free( in->name ) ;
		}
		in->name = new_device ;
		strcpy( in->name, "/dev/" ) ;
		strcat( in->name, i2c_bus->d_name ) ;

		// Now look for the ds2482's
		if ( DS2482_detect_bus( chip_num, all_in )!=0 ) {
			continue ;
		}

		// at least one found
		++found ;
		if ( any ) {
			break ;
		}

		// ALL? then set up a new connection_in slot
		all_in = NewIn(all_in) ;
		if ( all_in == NULL ) {
			break ;
		}
	}

	closedir( i2c_list_dir ) ;
	return found ;
}

/* All the rest of the program sees is the DS2482_detect and the entry in iroutines */
/* Open a DS2482 */
/* cycle through /dev/i2c-n */
/* returns -1 -- no direcory could be opened 
   else found -- number of directories found */
static int DS2482_detect_dir(enum ds2482_address chip_num, struct connection_in *in)
{
	int found = 0 ;
	int any = ( in->name[0] == 0x00 ) ; // no adapter specified, so use any (the first good one )
	struct connection_in * all_in = in ;
	int bus = 0 ;
	int sn_ret ;

	for ( bus=0 ; bus<99 ; ++bus ) { 
		char dev_name[128] ;
		char * new_device ;

		UCLIBCLOCK ;
		sn_ret = snprintf( dev_name, 128-1, "/dev/i2c-%d", bus ) ;
		UCLIBCUNLOCK ;
		if ( sn_ret < 0 ) {
				break ;
		}

		if ( access(dev_name, F_OK) < 0 ) {
			break ;
		}
		new_device = strdup( dev_name ) ;
		if ( new_device==NULL ) {
			break ; // cannot make space
		}

		// Change name to real i2c bus name
		free( in->name ) ;
		in->name = new_device ;

		// Now look for the ds2482's
		if ( DS2482_detect_bus( chip_num, all_in )!=0 ) {
			continue ;
		}

		// at least one found
		++found ;
		if ( any ) {
			break ;
		}

		// ALL? then set up a new connection_in slot
		all_in = NewIn(all_in) ;
		if ( all_in == NULL ) {
			break ;
		}
	}

	return found ;
}

/* Try to see if there is a DS2482 device on the specified i2c bus */
static int DS2482_detect_bus(enum ds2482_address chip_num, struct connection_in *in)
{
	switch (chip_num) {
		case ds2482_any:
			// usual case, find the first adapter
			return DS2482_detect_single( 0, 7, in ) ;
		case ds2482_all:
			// Look through all the possible i2c addresses
			{
				int start_chip = 0 ;
				struct connection_in * all_in = in ;
				do {
					if ( DS2482_detect_single( start_chip, 7, all_in ) != 0 ) {
						if ( in == all_in ) { //first time
							return -ENODEV ;
						}
						return 0 ;
					}
					start_chip = all_in->connin.i2c.i2c_index + 1 ;
					if ( start_chip > 7 ) {
						return 0 ;
					}
					all_in = NewIn(all_in) ;
					if ( all_in == NULL ) {
						return -ENOMEM ;
					}
				} while (1) ;
			}
			break ;
		default:
			// specific i2c address
			return DS2482_detect_single( chip_num, chip_num, in ) ;
	}
}

/* All the rest of the program sees is the DS2482_detect and the entry in iroutines */
/* Open a DS2482 */
/* Try to see if there is a DS2482 device on the specified i2c bus */
static int DS2482_detect_single(int lowindex, int highindex, struct connection_in *in)
{
	int test_address[8] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, };	// the last 4 are -800 only
	int i;
	int file_descriptor;

	/* open the i2c port */
	file_descriptor = open(in->name, O_RDWR);
	if (file_descriptor < 0) {
		ERROR_CONNECT("Could not open i2c device %s\n", in->name);
		return -ENODEV;
	}

	/* Set up low-level routines */
	DS2482_setroutines(in);

	for (i = lowindex; i <= highindex; ++i) {
		/* set the candidate address */
		if (ioctl(file_descriptor, I2C_SLAVE, test_address[i]) < 0) {
			ERROR_CONNECT("Cound not set trial i2c address to %.2X\n", test_address[i]);
		} else {
			BYTE c;
			LEVEL_CONNECT("Found an i2c device at %s address %d\n", in->name, test_address[i]);
			/* Provisional setup as a DS2482-100 ( 1 channel ) */
			in->file_descriptor = file_descriptor;
			in->connin.i2c.index = 0;
			in->connin.i2c.channels = 1;
			in->connin.i2c.current = 0;
			in->connin.i2c.head = in;
			in->adapter_name = "DS2482-100";
			in->connin.i2c.i2c_address = test_address[i];
			in->connin.i2c.i2c_index = i;
			in->connin.i2c.configreg = 0x00 ;	// default configuration setting desired
			if ( Globals.i2c_APU ) {
				in->connin.i2c.configreg |= DS2482_REG_CFG_APU ;
			}
			if ( Globals.i2c_PPM ) {
				in->connin.i2c.configreg |= DS2482_REG_CFG_PPM ;
			}
			#if OW_MT
			my_pthread_mutex_init(&(in->connin.i2c.i2c_mutex), Mutex.pmattr);
#endif							/* OW_MT */
			in->busmode = bus_i2c;
			in->Adapter = adapter_DS2482_100;

			/* write the RESET code */
			if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_RESET)	// reset
				|| DS2482_readstatus(&c, file_descriptor, DS2482_Chip_reset_usec)	// pause .5 usec then read status
				|| (c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST))	// make sure status is properly set
				) {
				LEVEL_CONNECT("i2c device at %s address %d cannot be reset. Not a DS2482.\n", in->name, test_address[i]);
				continue;;
			}
			LEVEL_CONNECT("i2c device at %s address %d appears to be DS2482-x00\n", in->name, test_address[i]);
			in->connin.i2c.configchip = 0x00;	// default configuration register after RESET
			// Note, only the lower nibble of the device config stored

			/* Now see if DS2482-100 or DS2482-800 */
			return HeadChannel(in);
		}
	}
	/* fellthough, no device found */
	Test_and_Close( &(in->file_descriptor) ) ;
	return -ENODEV;
}

/* Re-open a DS2482 */
static int DS2482_redetect(const struct parsedname *pn)
{
	struct connection_in *head = pn->selected_connection->connin.i2c.head;
	int address = head->connin.i2c.i2c_address;
	int file_descriptor;

	/* open the i2c port */
	file_descriptor = open(head->name, O_RDWR);
	if (file_descriptor < 0) {
		ERROR_CONNECT("Could not open i2c device %s\n", head->name);
		return -ENODEV;
	}

	/* address is known */
	if (ioctl(file_descriptor, I2C_SLAVE, head->connin.i2c.i2c_address) < 0) {
		ERROR_CONNECT("Cound not set i2c address to %.2X\n", address);
	} else {
		BYTE c;
		/* write the RESET code */
		if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_RESET)	// reset
			|| DS2482_readstatus(&c, file_descriptor, DS2482_Chip_reset_usec)	// pause .5 usec then read status
			|| (c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST))	// make sure status is properly set
			) {
			LEVEL_CONNECT("i2c device at %s address %d cannot be reset. Not a DS2482.\n", head->name, address);
		} else {
			head->connin.i2c.current = 0;
			head->file_descriptor = file_descriptor;
			head->connin.i2c.configchip = 0x00;	// default configuration register after RESET
			LEVEL_CONNECT("i2c device at %s address %d reset successfully\n", head->name, address);
			if (head->connin.i2c.channels > 1) {	// need to reset other 8 channels?
				/* loop through devices, matching those that have the same "head" */
				/* BUSLOCK also locks the sister channels for this */
				struct connection_in *in;
				for (in = Inbound_Control.head; in; in = in->next) {
					if (in == head) {
						in->reconnect_state = reconnect_ok;
					}
				}
			}
			return 0;
		}
	}
	/* fellthough, no device found */
	close(file_descriptor);
	return -ENODEV;
}

/* read status register */
/* should already be set to read from there */
/* will read at min time, avg time, max time, and another 50% */
/* returns 0 good, 1 bad */
/* tests to make sure bus not busy */
static int DS2482_readstatus(BYTE * c, int file_descriptor, unsigned long int min_usec, unsigned long int max_usec)
{
	unsigned long int delta_usec = (max_usec - min_usec + 1) / 2;
	int i = 0;
	UT_delay_us(min_usec);		// at least get minimum out of the way
	do {
		int ret = i2c_smbus_read_byte(file_descriptor);
		if (ret < 0) {
			LEVEL_DEBUG("Reading DS2482 status problem min=%lu max=%lu i=%d ret=%d\n", min_usec, max_usec, i, ret);
			return 1;
		}
		if ((ret & DS2482_REG_STS_1WB) == 0x00) {
			c[0] = (BYTE) ret;
			LEVEL_DEBUG("DS2482 read status ok\n");
			return 0;
		}
		if (i++ == 3) {
			LEVEL_DEBUG("Reading DS2482 status still busy min=%lu max=%lu i=%d ret=%d\n", min_usec, max_usec, i, ret);
			return 1;
		}
		UT_delay_us(delta_usec);	// increment up to three times
	} while (1);
}

/* uses the "Triple" primative for faster search */
static int DS2482_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int search_direction = 0;	/* initialization just to forestall incorrect compiler warning */
	int bit_number;
	int last_zero = -1;
	int file_descriptor = pn->selected_connection->connin.i2c.head->file_descriptor;
	BYTE bits[3];
	int ret;

	// initialize for search
	// if the last call was not the last one
	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

	/* Make sure we're using the correct channel */
	/* Appropriate search command */
	if ((ret = BUS_send_data(&(ds->search), 1, pn))) {
		return ret;
	}
	// loop to do the search
	for (bit_number = 0; bit_number < 64; ++bit_number) {
		LEVEL_DEBUG("DS2482 search bit number %d\n", bit_number);
		/* Set the direction bit */
		if (bit_number < ds->LastDiscrepancy) {
			search_direction = UT_getbit(ds->sn, bit_number);
		} else {
			search_direction = (bit_number == ds->LastDiscrepancy) ? 1 : 0;
		}
		/* Appropriate search command */
		if ((ret = DS2482_triple(bits, search_direction, file_descriptor))) {
			return ret;
		}
		if (bits[0] || bits[1] || bits[2]) {
			if (bits[0] && bits[1]) {	/* 1,1 */
				/* No devices respond */
				ds->LastDevice = 1;
				return -ENODEV;
			}
		} else {				/* 0,0,0 */
			last_zero = bit_number;
		}
		UT_setbit(ds->sn, bit_number, bits[2]);
	}							// loop until through serial number bits

	if (CRC8(ds->sn, 8) || (bit_number < 64) || (ds->sn[0] == 0)) {
		/* Unsuccessful search or error -- possibly a device suddenly added */
		return -EIO;
	}
	if ((ds->sn[0] & 0x7F) == 0x04) {
		/* We found a DS1994/DS2404 which require longer delays */
		pn->selected_connection->ds2404_compliance = 1;
	}
	// if the search was successful then

	ds->LastDiscrepancy = last_zero;
	ds->LastDevice = (last_zero < 0);
	LEVEL_DEBUG("DS2482_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return 0;
}

/* DS2482 Reset -- A little different from DS2480B */
// return 1 shorted, 0 ok, <0 error
static int DS2482_reset(const struct parsedname *pn)
{
	BYTE c;
	int file_descriptor = pn->selected_connection->connin.i2c.head->file_descriptor;

	/* Make sure we're using the correct channel */
	if (DS2482_channel_select(pn)) {
		return -EIO;
	}

	/* write the RESET code */
	if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_1WIRE_RESET)) {
		return -EIO;
	}

	/* wait */
	// rstl+rsth+.25 usec

	/* read status */
	if (DS2482_readstatus(&c, file_descriptor, DS2482_1wire_reset_usec)) {
		return -EIO;			// 8 * Tslot
	}

	pn->selected_connection->AnyDevices = (c & DS2482_REG_STS_PPD) != 0;
	LEVEL_DEBUG("DS2482 Reset\n");
	return (c & DS2482_REG_STS_SD) ? BUS_RESET_SHORT : BUS_RESET_OK;
}

static int DS2482_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	int file_descriptor = pn->selected_connection->connin.i2c.head->file_descriptor;
	size_t i;

	/* Make sure we're using the correct channel */
	if (DS2482_channel_select(pn)) {
		return -1;
	}

	for (i = 0; i < len; ++i) {
		if (DS2482_send_and_get(file_descriptor, data[i], &resp[i])) {
			return 1;
		}
	}
	return 0;
}

/* Single byte -- assumes channel selection already done */
static int DS2482_send_and_get(int file_descriptor, const BYTE wr, BYTE * rd)
{
	int read_back;
	BYTE c;

	/* Write data byte */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_1WIRE_WRITE_BYTE, wr) < 0) {
		return 1;
	}

	/* read status for done */
	if (DS2482_readstatus(&c, file_descriptor, DS2482_1wire_write_usec)) {
		return -1;
	}

	/* Select the data register */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_SET_READ_PTR, DS2482_PTR_CODE_DATA) < 0) {
		return 1;
	}

	/* Read the data byte */
	read_back = i2c_smbus_read_byte(file_descriptor);

	if (read_back < 0) {
		return 1;
	}
	rd[0] = (BYTE) read_back;

	return 0;
}

/* It's a DS2482 -- whether 1 channel or 8 channel not yet determined */
/* All general stored data will be assigned to this "head" channel */
static int HeadChannel(struct connection_in *in)
{
	struct parsedname pn;

	/* Intentionally put the wrong index */
	in->connin.i2c.index = 1;
	pn.selected_connection = in;
	if (DS2482_channel_select(&pn)) {	/* Couldn't switch */
		in->connin.i2c.index = 0;	/* restore correct value */
		LEVEL_CONNECT("DS2482-100 (Single channel).");
		return 0;				/* happy as DS2482-100 */
	}
	LEVEL_CONNECT("DS2482-800 (Eight channels).");
	/* Must be a DS2482-800 */
	in->connin.i2c.channels = 8;
	in->Adapter = adapter_DS2482_800;
	
	return CreateChannels(in);
}

/* create more channels,
   inserts in connection_in chain
   "in" points to  first (head) channel
   called only for DS12482-800
   NOTE: coded assuming num = 1 or 8 only
 */
static int CreateChannels(struct connection_in *in)
{
	int i;
	char *name[] = { "DS2482-800(0)", "DS2482-800(1)", "DS2482-800(2)",
		"DS2482-800(3)", "DS2482-800(4)", "DS2482-800(5)", "DS2482-800(6)",
		"DS2482-800(7)",
	};
	in->connin.i2c.index = 0;
	in->adapter_name = name[0];
	for (i = 1; i < 8; ++i) {
		struct connection_in *added ;
		added = NewIn(in);
		if (added == NULL) {
			return -ENOMEM;
		}
		added->connin.i2c.index = i;
		added->adapter_name = name[i];
	}
	return 0;
}

static int DS2482_triple(BYTE * bits, int direction, int file_descriptor)
{
	/* 3 bits in bits */
	BYTE c;

	LEVEL_DEBUG("-> TRIPLET attempt direction %d\n", direction);
	/* Write TRIPLE command */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_1WIRE_TRIPLET, direction ? 0xFF : 0) < 0) {
		return 1;
	}

	/* read status */
	if (DS2482_readstatus(&c, file_descriptor, DS2482_1wire_triplet_usec)) {
		return -1;				// 250usec = 3*Tslot
	}

	bits[0] = (c & DS2482_REG_STS_SBR) != 0;
	bits[1] = (c & DS2482_REG_STS_TSB) != 0;
	bits[2] = (c & DS2482_REG_STS_DIR) != 0;
	LEVEL_DEBUG("<- TRIPLET %d %d %d\n", bits[0], bits[1], bits[2]);
	return 0;
}

static int DS2482_channel_select(const struct parsedname *pn)
{
	struct connection_in *head = pn->selected_connection->connin.i2c.head;
	int chan = pn->selected_connection->connin.i2c.index;
	int file_descriptor = head->file_descriptor;
	BYTE config = pn->selected_connection->connin.i2c.configreg;
	/**
     * Write and verify codes for the CHANNEL_SELECT command (DS2482-800 only).
     * To set the channel, write the value at the index of the channel.
     * Read and compare against the corresponding value to verify the change.
     */
	static const BYTE W_chan[8] = { 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 };
	static const BYTE R_chan[8] = { 0xB8, 0xB1, 0xAA, 0xA3, 0x9C, 0x95, 0x8E, 0x87 };

	if (file_descriptor < 0) {
		LEVEL_CONNECT("Calling a closed i2c channel (%d)\n", chan);
		return -EINVAL;
	}

	/* Already properly selected? */
	/* All `100 (1 channel) will be caught here */
	if (chan == head->connin.i2c.current) {
		int read_back;

		/* Select command */
		if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_CHANNEL_SELECT, W_chan[chan]) < 0) {
			return -EIO;
		}

		/* Read back and confirm */
		read_back = i2c_smbus_read_byte(file_descriptor);
		if (read_back < 0) {
			return -ENODEV;
		}
		if (((BYTE) read_back) != R_chan[chan]) {
			return -ENODEV;
		}

		/* Set the channel in head */
		head->connin.i2c.current = pn->selected_connection->connin.i2c.index;
	}

	/* Now check the configuration register */
	/* This is since configuration is per chip, not just channel */
	if (pn->selected_connection->connin.i2c.configreg != head->connin.i2c.configchip) {
		return SetConfiguration(pn->selected_connection->connin.i2c.configreg, pn->selected_connection);
	}

	return 0;
}

/* Set the configuration register, both for this channel, and for head global data */
/* Note, config is stored as only the lower nibble */
static int SetConfiguration(BYTE c, struct connection_in *in)
{
	struct connection_in *head = in->connin.i2c.head;
	int file_descriptor = head->file_descriptor;
	int read_back;

	/* Write, readback, and compare configuration register */
	/* Logic error fix from Uli Raich */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_WRITE_CONFIG, c | ((~c) << 4))
		|| (read_back = i2c_smbus_read_byte(file_descriptor)) < 0 || ((BYTE) read_back != c)
		) {
		head->connin.i2c.configchip = 0xFF;	// bad value to trigger retry
		LEVEL_CONNECT("Trouble changing DS2482 configuration register\n");
		return -EINVAL;
	}
	/* Clear the strong pull-up power bit(register is automatically cleared by reset) */
	in->connin.i2c.configreg = head->connin.i2c.configchip = c & ~DS2482_REG_CFG_SPU;
	return 0;
}

static int DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	/* Make sure we're using the correct channel */
	if (DS2482_channel_select(pn)) {
		return -1;
	}

	/* Set the power (bit is automatically cleared by reset) */
	if (SetConfiguration(pn->selected_connection->connin.i2c.configreg | DS2482_REG_CFG_SPU, pn->selected_connection)) {
		return -1;
	}

	/* send and get byte (and trigger strong pull-up */
	if (DS2482_send_and_get(pn->selected_connection->connin.i2c.head->file_descriptor, byte, resp)) {
		return -1;
	}

	UT_delay(delay);

	return 0;
}

static void DS2482_close(struct connection_in *in)
{
	if (in == NULL) {
		return;
	}
	Test_and_Close( & (in->connin.i2c.head->file_descriptor) ) ;
}
#endif							/* OW_I2C */
