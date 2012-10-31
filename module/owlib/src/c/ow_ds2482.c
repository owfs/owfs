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

static GOOD_OR_BAD DS2482_detect_bus(enum ds2482_address chip_num, char * i2c_device, struct port_in *pin) ;
static GOOD_OR_BAD DS2482_detect_sys( int any, enum ds2482_address chip_num, struct port_in *pin) ;
static GOOD_OR_BAD DS2482_detect_dir( int any, enum ds2482_address chip_num, struct port_in *pin) ;
static GOOD_OR_BAD DS2482_detect_single(int lowindex, int highindex, char * i2c_device, struct port_in *pin) ;
static enum search_status DS2482_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD DS2482_triple(BYTE * bits, int direction, FILE_DESCRIPTOR_OR_ERROR file_descriptor);
static GOOD_OR_BAD DS2482_send_and_get(FILE_DESCRIPTOR_OR_ERROR file_descriptor, const BYTE wr, BYTE * rd);
static RESET_TYPE DS2482_reset(const struct parsedname *pn);
static GOOD_OR_BAD DS2482_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void DS2482_setroutines(struct connection_in *in);
static GOOD_OR_BAD HeadChannel(struct connection_in *head);
static GOOD_OR_BAD CreateChannels(struct connection_in *head);
static GOOD_OR_BAD DS2482_channel_select(struct connection_in * in);
static GOOD_OR_BAD DS2482_readstatus(BYTE * c, FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long int min_usec, unsigned long int max_usec);
static GOOD_OR_BAD SetConfiguration(BYTE c, struct connection_in *in);
static void DS2482_close(struct connection_in *in);
static GOOD_OR_BAD DS2482_redetect(const struct parsedname *pn);
static GOOD_OR_BAD DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);

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
#define DS2482_STATUS_REGISTER         		 0xF0
#define DS2482_READ_DATA_REGISTER            0xE1
#define DS2482_DEVICE_CONFIGURATION_REGISTER 0xC3
#define DS2482_CHANNEL_SELECTION_REGISTER    0xD2	/* DS2482-800 only */
#define DS2482_PORT_CONFIGURATION_REGISTER   0xB4   /* DS2483 only */

/**
 * Configure Register bit definitions
 * The top 4 bits always read 0.
 * To write, the top nibble must be the 1's compl. of the low nibble.
 */
#define DS2482_REG_CFG_1WS     0x08
#define DS2482_REG_CFG_SPU     0x04
#define DS2482_REG_CFG_PDN     0x02 /* DS2483 only, power down */
#define DS2482_REG_CFG_PPM     0x02 /* non-DS2483, presence pulse masking */
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

/* Defines for making messages more explicit */
#define I2Cformat "I2C bus %s, channel %d/%d"
#define I2Cvar(in)  DEVICENAME(in), (in)->master.i2c.index, (in)->master.i2c.channels

/* Device-specific functions */
static void DS2482_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS2482_detect;
	in->iroutines.reset = DS2482_reset;
	in->iroutines.next_both = DS2482_next_both;
	in->iroutines.PowerByte = DS2482_PowerByte;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = DS2482_sendback_data;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = DS2482_redetect;
	in->iroutines.close = DS2482_close;
	in->iroutines.flags = ADAP_FLAG_overdrive;
	in->bundling_length = I2C_FIFO_SIZE;
}

/* All the rest of the program sees is the DS2482_detect and the entry in iroutines */
/* Open a DS2482 */
/* Top level detect routine */
GOOD_OR_BAD DS2482_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct address_pair ap ;
	GOOD_OR_BAD gbResult ;
	enum ds2482_address chip_num ;
	
	Parse_Address( pin->init_data, &ap ) ;
	
	switch ( ap.second.type ) {
		case address_numeric:
			if ( ap.second.number < ds2482_18 || ap.second.number >= ds2482_too_far ) {
				LEVEL_CALL("DS2482 bus address <%s> invalid. Will search.", ap.second.alpha) ;
				chip_num = ds2482_any ;
			} else {
				chip_num = ap.second.number ;
			}
			break ;
		case address_all:
		case address_asterix:
			chip_num = ds2482_all ;
			break ;
		case address_none:
			chip_num = ds2482_any ;
			break ;
		default:
			LEVEL_CALL("DS2482 bus address <%s> invalid. Will scan.", ap.second.alpha) ;
			chip_num = ds2482_any ;
			break ;
	}

	SAFEFREE( DEVICENAME(in) ) ;

	switch ( ap.first.type ) {
		case address_all:
		case address_asterix:
			// All adapters
			gbResult = DS2482_detect_sys( 0, chip_num, pin ) ;
			break ;
		case address_none:
			// Any adapter -- first one found
			gbResult = DS2482_detect_sys( 1, chip_num, pin ) ;
			break ;
		default:
			// traditional, actual bus specified
			gbResult = DS2482_detect_bus( chip_num, ap.first.alpha, pin ) ;
			break ;
	}
	
	Free_Address( &ap ) ;
	return gbResult ;
}

#define SYSFS_I2C_Path "/sys/class/i2c-adapter"

/* Use sysfs to find i2c adapters */
/* cycle through SYSFS_I2C_Path */
/* return GOOD if any found */
static GOOD_OR_BAD DS2482_detect_sys( int any, enum ds2482_address chip_num, struct port_in *pin_original)
{
	DIR * i2c_list_dir ;
	struct dirent * i2c_bus ;
	int found = 0 ;
	struct port_in * pin_current = pin_original ;

	// We'll look in this directory for available i2c adapters.
	// This may be linux 2.6 specific
	i2c_list_dir = opendir( SYSFS_I2C_Path ) ;
	if ( i2c_list_dir == NULL ) {
		ERROR_CONNECT( "Cannot open %d to find available i2c devices",SYSFS_I2C_Path ) ;
		// Use the cruder approach of trying all possible numbers
		return DS2482_detect_dir( any, chip_num, pin_original ) ;
	}

	/* cycle through entries in /sys/class/i2c-adapter */
	while ( (i2c_bus=readdir(i2c_list_dir)) != NULL ) {
		char dev_name[128] ; // room for /dev/name
		int sn_ret ;

		UCLIBCLOCK ;
		sn_ret = snprintf( dev_name, 128, "/dev/%s", i2c_bus->d_name ) ;
		UCLIBCUNLOCK ;
		if ( sn_ret < 0 ) {
			break ;
		}

		// Now look for the ds2482's
		if ( BAD( DS2482_detect_bus( chip_num, dev_name, pin_current ) ) ) {
			continue ; // none found on this i2c bus
		}

		// at least one found on this i2c bus
		++found ;
		if ( any ) { // found one -- that's enough
			closedir( i2c_list_dir ) ;
			return gbGOOD ;
		}

		// ALL? then set up a new connection_in slot for the next one
		pin_current = NewPort(pin_current) ;
		if ( pin_current == NULL ) {
			break ;
		}
	}

	closedir( i2c_list_dir ) ;
	
	if ( found==0 ) {
		return gbBAD ;
	}

	if ( pin_current != pin_original ) {
		RemovePort( pin_current ) ;
	}
	return gbGOOD ;
}

/* non sysfs method -- try by bus name */
/* cycle through /dev/i2c-n */
/* returns GOOD if any adpters found */
static GOOD_OR_BAD DS2482_detect_dir( int any, enum ds2482_address chip_num, struct port_in *pin_original)
{
	int found = 0 ;
	struct port_in * pin_current = pin_original ;
	int bus = 0 ;
	int sn_ret ;

	for ( bus=0 ; bus<99 ; ++bus ) {
		char dev_name[128] ;

		UCLIBCLOCK ;
		sn_ret = snprintf( dev_name, 128-1, "/dev/i2c-%d", bus ) ;
		UCLIBCUNLOCK ;
		if ( sn_ret < 0 ) {
				break ;
		}

		if ( access(dev_name, F_OK) < 0 ) {
			continue ;
		}

		// Now look for the ds2482's
		if ( BAD( DS2482_detect_bus( chip_num, dev_name, pin_current ) ) ) {
			continue ;
		}

		// at least one found
		++found ;
		if ( any ) {
			return gbGOOD ;
		}

		// ALL? then set up a new connection_in slot
		pin_current = NewPort(pin_current) ;
		if ( pin_current == NULL ) {
			break ;
		}
	}

	if ( found == 0 ) {
		return gbBAD ;
	}
	
	if ( pin_original != pin_current ) {
		RemovePort(pin_current) ;
	}
	return gbGOOD ;
}

/* Try to see if there is a DS2482 device on the specified i2c bus */
/* Includes  a fix from Pascal Baerten */
static GOOD_OR_BAD DS2482_detect_bus(enum ds2482_address chip_num, char * i2c_device, struct port_in * pin_original)
{
	switch (chip_num) {
		case ds2482_any:
			// usual case, find the first adapter
			return DS2482_detect_single( 0, 7, i2c_device, pin_original ) ;
		case ds2482_all:
			// Look through all the possible i2c addresses
			{
				int start_chip = 0 ;
				struct port_in * all_pin = pin_original ;
				do {
					if ( BAD( DS2482_detect_single( start_chip, 7, i2c_device, all_pin ) ) ) {
						if ( pin_original == all_pin ) { //first time
							return gbBAD ;
						}
						LEVEL_DEBUG("Cleaning excess allocated i2c structure");
						RemovePort(all_pin); //Pascal Baerten :this removes the false DS2482-100 provisioned
						return gbGOOD ;
					}
					start_chip = all_pin->first->master.i2c.i2c_index + 1 ;
					if ( start_chip > 7 ) {
						return gbGOOD ;
					}
					all_pin = NewPort(all_pin) ;
					if ( all_pin == NULL ) {
						return gbBAD ;
					}
				} while (1) ;
			}
			break ;
		default:
			// specific i2c address
			return DS2482_detect_single( chip_num, chip_num, i2c_device, pin_original ) ;
	}
}

/* Try to see if there is a DS2482 device on the specified i2c bus */
static GOOD_OR_BAD DS2482_detect_single(int lowindex, int highindex, char * i2c_device, struct port_in *pin)
{
	int test_address[8] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, };	// the last 4 are -800 only
	int i2c_index;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct connection_in * in = pin->first ;

	/* Sanity check */
	if ( lowindex < 0 ) {
		LEVEL_DEBUG("Bad lower bound");
		return gbBAD ;
	}
	if ( highindex >= (int) (sizeof(test_address)/sizeof(int)) ) {
		LEVEL_DEBUG("Bad upper bound");
		return gbBAD ;
	}
	
	/* open the i2c port */
	file_descriptor = open(i2c_device, O_RDWR);
	if ( FILE_DESCRIPTOR_NOT_VALID(file_descriptor) ) {
		ERROR_CONNECT("Could not open i2c device %s", i2c_device);
		return gbBAD;
	}

	/* Set up low-level routines */
	DS2482_setroutines(in);

	for (i2c_index = lowindex; i2c_index <= highindex; ++i2c_index) {
		int trial_address = test_address[i2c_index] ;
		/* set the candidate address */
		if (ioctl(file_descriptor, I2C_SLAVE, trial_address) < 0) {
			ERROR_CONNECT("Cound not set trial i2c address to %.2X", trial_address);
		} else {
			BYTE c;
			LEVEL_CONNECT("Found an i2c device at %s address %.2X", i2c_device, trial_address);
			/* Provisional setup as a DS2482-100 ( 1 channel ) */
			in->pown->file_descriptor = file_descriptor;
			pin->state = cs_deflowered;
			pin->type = ct_i2c ;
			in->master.i2c.i2c_address = trial_address;
			in->master.i2c.i2c_index = i2c_index;
			in->master.i2c.index = 0;
			in->master.i2c.channels = 1;
			in->master.i2c.current = 0;
			in->master.i2c.head = in;
			in->adapter_name = "DS2482-100";
			in->master.i2c.configreg = 0x00 ;	// default configuration setting desired
			if ( Globals.i2c_APU ) {
				in->master.i2c.configreg |= DS2482_REG_CFG_APU ;
			}
			if ( Globals.i2c_PPM ) {
				in->master.i2c.configreg |= DS2482_REG_CFG_PPM ;
			}
			in->Adapter = adapter_DS2482_100;

			/* write the RESET code */
			if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_RESET)	// reset
				|| BAD(DS2482_readstatus(&c, file_descriptor, DS2482_Chip_reset_usec))	// pause .5 usec then read status
				|| (c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST))	// make sure status is properly set
				) {
				LEVEL_CONNECT("i2c device at %s address %.2X cannot be reset. Not a DS2482.", i2c_device, trial_address);
				continue;
			}
			LEVEL_CONNECT("i2c device at %s address %.2X appears to be DS2482-x00", i2c_device, trial_address);
			in->master.i2c.configchip = 0x00;	// default configuration register after RESET
			// Note, only the lower nibble of the device config stored
			
			// Create name
			DEVICENAME(in) = owmalloc( strlen(i2c_device) + 10 ) ;
			if ( DEVICENAME(in) ) {
				UCLIBCLOCK;
				snprintf(DEVICENAME(in), strlen(i2c_device) + 10, "%s:%.2X", i2c_device, trial_address);
				UCLIBCUNLOCK;
			}

			/* Now see if DS2482-100 or DS2482-800 */
			return HeadChannel(in);
		}
	}
	/* fell though, no device found */
	COM_close( in ) ;
	return gbBAD;
}

/* Re-open a DS2482 */
static GOOD_OR_BAD DS2482_redetect(const struct parsedname *pn)
{
	struct connection_in *head = pn->selected_connection->master.i2c.head;
	int address = head->master.i2c.i2c_address;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct address_pair ap ; // to get device name from device:address

	/* open the i2c port */
	Parse_Address( DEVICENAME(head), &ap ) ;
	file_descriptor = open(ap.first.alpha, O_RDWR );
	Free_Address( &ap ) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(file_descriptor) ) {
		ERROR_CONNECT("Could not open i2c device %s", DEVICENAME(head));
		return gbBAD;
	}
	
	/* address is known */
	if (ioctl(file_descriptor, I2C_SLAVE, head->master.i2c.i2c_address) < 0) {
		ERROR_CONNECT("Cound not set i2c address to %.2X", address);
	} else {
		BYTE c;
		/* write the RESET code */
		if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_RESET)	// reset
			|| BAD(DS2482_readstatus(&c, file_descriptor, DS2482_Chip_reset_usec))	// pause .5 usec then read status
			|| (c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST))	// make sure status is properly set
			) {
			LEVEL_CONNECT("i2c device at %s address %d cannot be reset. Not a DS2482.", DEVICENAME(head), address);
		} else {
			struct connection_in * next ;
			head->master.i2c.current = 0;
			head->pown->file_descriptor = file_descriptor;
			head->pown->state = cs_deflowered ;
			head->pown->type = ct_i2c ;
			head->master.i2c.configchip = 0x00;	// default configuration register after RESET	
			LEVEL_CONNECT("i2c device at %s address %d reset successfully", DEVICENAME(head), address);
			for ( next = head->pown->first; next; next = next->next ) {
				/* loop through devices, matching those that have the same "head" */
				/* BUSLOCK also locks the sister channels for this */
				next->reconnect_state = reconnect_ok;
			}
			return gbGOOD;
		}
	}
	/* fellthough, no device found */
	close(file_descriptor);
	return gbBAD;
}

/* read status register */
/* should already be set to read from there */
/* will read at min time, avg time, max time, and another 50% */
/* returns 0 good, 1 bad */
/* tests to make sure bus not busy */
static GOOD_OR_BAD DS2482_readstatus(BYTE * c, FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long int min_usec, unsigned long int max_usec)
{
	unsigned long int delta_usec = (max_usec - min_usec + 1) / 2;
	int i = 0;
	UT_delay_us(min_usec);		// at least get minimum out of the way
	do {
		int ret = i2c_smbus_read_byte(file_descriptor);
		if (ret < 0) {
			LEVEL_DEBUG("problem min=%lu max=%lu i=%d ret=%d", min_usec, max_usec, i, ret);
			return gbBAD;
		}
		if ((ret & DS2482_REG_STS_1WB) == 0x00) {
			c[0] = (BYTE) ret;
			LEVEL_DEBUG("ok");
			return gbGOOD;
		}
		if (i++ == 3) {
			LEVEL_DEBUG("still busy min=%lu max=%lu i=%d ret=%d", min_usec, max_usec, i, ret);
			return gbBAD;
		}
		UT_delay_us(delta_usec);	// increment up to three times
	} while (1);
}

/* uses the "Triple" primative for faster search */
static enum search_status DS2482_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int search_direction = 0;	/* initialization just to forestall incorrect compiler warning */
	int bit_number;
	int last_zero = -1;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = pn->selected_connection->pown->file_descriptor;
	BYTE bits[3];

	// initialize for search
	// if the last call was not the last one
	if (ds->LastDevice) {
		return search_done;
	}

	if ( BAD( BUS_select(pn) ) ) {
		return search_error ;
	}

	// need the reset done in BUS-select to set AnyDevices
	if ( pn->selected_connection->AnyDevices == anydevices_no ) {
		ds->LastDevice = 1;
		return search_done;
	}

	/* Make sure we're using the correct channel */
	/* Appropriate search command */
	if ( BAD(BUS_send_data(&(ds->search), 1, pn)) ) {
		return search_error;
	}
	// loop to do the search
	for (bit_number = 0; bit_number < 64; ++bit_number) {
		LEVEL_DEBUG("bit number %d", bit_number);
		/* Set the direction bit */
		if (bit_number < ds->LastDiscrepancy) {
			search_direction = UT_getbit(ds->sn, bit_number);
		} else {
			search_direction = (bit_number == ds->LastDiscrepancy) ? 1 : 0;
		}
		/* Appropriate search command */
		if ( BAD( DS2482_triple(bits, search_direction, file_descriptor) ) )  {
			return search_error;
		}
		if (bits[0] || bits[1] || bits[2]) {
			if (bits[0] && bits[1]) {	/* 1,1 */
				/* No devices respond */
				ds->LastDevice = 1;
				return search_done;
			}
		} else {				/* 0,0,0 */
			last_zero = bit_number;
		}
		UT_setbit(ds->sn, bit_number, bits[2]);
	}							// loop until through serial number bits

	if (CRC8(ds->sn, SERIAL_NUMBER_SIZE) || (bit_number < 64) || (ds->sn[0] == 0)) {
		/* Unsuccessful search or error -- possibly a device suddenly added */
		return search_error;
	}
	// if the search was successful then
	ds->LastDiscrepancy = last_zero;
	ds->LastDevice = (last_zero < 0);
	LEVEL_DEBUG("SN found: " SNformat "", SNvar(ds->sn));
	return search_good;
}

/* DS2482 Reset -- A little different from DS2480B */
// return 1 shorted, 0 ok, <0 error
static RESET_TYPE DS2482_reset(const struct parsedname *pn)
{
	BYTE status_byte;
	struct connection_in * in = pn->selected_connection ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->pown->file_descriptor;

	/* Make sure we're using the correct channel */
	if ( BAD(DS2482_channel_select(in)) ) {
		return BUS_RESET_ERROR;
	}

	/* write the RESET code */
	if (i2c_smbus_write_byte(file_descriptor, DS2482_CMD_1WIRE_RESET)) {
		return BUS_RESET_ERROR;
	}

	/* wait */
	// rstl+rsth+.25 usec

	/* read status */
	if ( BAD( DS2482_readstatus(&status_byte, file_descriptor, DS2482_1wire_reset_usec) ) ) {
		return BUS_RESET_ERROR;			// 8 * Tslot
	}

	in->AnyDevices = (status_byte & DS2482_REG_STS_PPD) ? anydevices_yes : anydevices_no ;
	LEVEL_DEBUG("DS2482 "I2Cformat" Any devices found on reset? %s",I2Cvar(in),in->AnyDevices==anydevices_yes?"Yes":"No");
	return (status_byte & DS2482_REG_STS_SD) ? BUS_RESET_SHORT : BUS_RESET_OK;
}

static GOOD_OR_BAD DS2482_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->pown->file_descriptor;
	size_t i;

	/* Make sure we're using the correct channel */
	RETURN_BAD_IF_BAD(DS2482_channel_select(in)) ;

	TrafficOut( "write", data, len, in ) ;
	for (i = 0; i < len; ++i) {
		RETURN_BAD_IF_BAD(DS2482_send_and_get(file_descriptor, data[i], &resp[i])) ;
	}
	TrafficOut( "response", resp, len, in ) ;
	return gbGOOD;
}

/* Single byte -- assumes channel selection already done */
static GOOD_OR_BAD DS2482_send_and_get(FILE_DESCRIPTOR_OR_ERROR file_descriptor, const BYTE wr, BYTE * rd)
{
	int read_back;
	BYTE c;

	/* Write data byte */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_1WIRE_WRITE_BYTE, wr) < 0) {
		return gbBAD;
	}

	/* read status for done */
	RETURN_BAD_IF_BAD( DS2482_readstatus(&c, file_descriptor, DS2482_1wire_write_usec) ) ;

	/* Select the data register */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_SET_READ_PTR, DS2482_READ_DATA_REGISTER) < 0) {
		return gbBAD;
	}

	/* Read the data byte */
	read_back = i2c_smbus_read_byte(file_descriptor);

	if (read_back < 0) {
		return gbBAD;
	}
	rd[0] = (BYTE) read_back;

	return gbGOOD;
}

/* It's a DS2482 -- whether 1 channel or 8 channel not yet determined */
/* All general stored data will be assigned to this "head" channel */
static GOOD_OR_BAD HeadChannel(struct connection_in *head)
{
	/* Intentionally put the wrong index */
	head->master.i2c.index = 1;

	if ( BAD(DS2482_channel_select(head)) ) {	/* Couldn't switch */
		head->master.i2c.index = 0;	/* restore correct value */
		LEVEL_CONNECT("DS2482-100 (Single channel)");
		return gbGOOD;				/* happy as DS2482-100 */
	}

	// It's a DS2482-800 (8 channels) to set up other 7 with this one as "head"
	LEVEL_CONNECT("DS2482-800 (Eight channels)");
	/* Must be a DS2482-800 */
	head->master.i2c.channels = 8;
	head->Adapter = adapter_DS2482_800;

	return CreateChannels(head);
}

/* create more channels,
   inserts in connection_in chain
   "in" points to  first (head) channel
   called only for DS12482-800
   NOTE: coded assuming num = 1 or 8 only
 */
static GOOD_OR_BAD CreateChannels(struct connection_in *head)
{
	int i;
	char *name[] = { "DS2482-800(0)", "DS2482-800(1)", "DS2482-800(2)",
		"DS2482-800(3)", "DS2482-800(4)", "DS2482-800(5)", "DS2482-800(6)",
		"DS2482-800(7)",
	};
	head->master.i2c.index = 0;
	head->adapter_name = name[0];
	for (i = 1; i < 8; ++i) {
		struct connection_in * added = AddtoPort(head->pown);
		if (added == NO_CONNECTION) {
			return gbBAD;
		}
		added->master.i2c.index = i;
		added->adapter_name = name[i];
	}
	return gbGOOD;
}

static GOOD_OR_BAD DS2482_triple(BYTE * bits, int direction, FILE_DESCRIPTOR_OR_ERROR file_descriptor)
{
	/* 3 bits in bits */
	BYTE c;

	LEVEL_DEBUG("-> TRIPLET attempt direction %d", direction);
	/* Write TRIPLE command */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_1WIRE_TRIPLET, direction ? 0xFF : 0) < 0) {
		return gbBAD;
	}

	/* read status */
	RETURN_BAD_IF_BAD(DS2482_readstatus(&c, file_descriptor, DS2482_1wire_triplet_usec)) ;

	bits[0] = (c & DS2482_REG_STS_SBR) != 0;
	bits[1] = (c & DS2482_REG_STS_TSB) != 0;
	bits[2] = (c & DS2482_REG_STS_DIR) != 0;
	LEVEL_DEBUG("<- TRIPLET %d %d %d", bits[0], bits[1], bits[2]);
	return gbGOOD;
}

static GOOD_OR_BAD DS2482_channel_select(struct connection_in * in)
{
	struct connection_in *head = in->master.i2c.head;
	int chan = in->master.i2c.index;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->pown->file_descriptor;

	/*
		Write and verify codes for the CHANNEL_SELECT command (DS2482-800 only).
		To set the channel, write the value at the index of the channel.
		Read and compare against the corresponding value to verify the change.
	*/
	static const BYTE W_chan[8] = { 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 };
	static const BYTE R_chan[8] = { 0xB8, 0xB1, 0xAA, 0xA3, 0x9C, 0x95, 0x8E, 0x87 };

	if ( FILE_DESCRIPTOR_NOT_VALID(file_descriptor) ) {
		LEVEL_CONNECT("Calling a closed i2c channel (%d) "I2Cformat" ", chan,I2Cvar(in));
		return gbBAD;
	}

	/* Already properly selected? */
	/* All `100 (1 channel) will be caught here */
	if (chan != head->master.i2c.current) {
		int read_back;

		/* Select command */
		if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_CHANNEL_SELECT, W_chan[chan]) < 0) {
			return gbBAD;
		}

		/* Read back and confirm */
		read_back = i2c_smbus_read_byte(file_descriptor);
		if (read_back < 0) {
			return gbBAD; // flag for DS2482-100 vs -800 detection
		}
		if (((BYTE) read_back) != R_chan[chan]) {
			return gbBAD; // flag for DS2482-100 vs -800 detection
		}

		/* Set the channel in head */
		head->master.i2c.current = in->master.i2c.index;
	}

	/* Now check the configuration register */
	/* This is since configuration is per chip, not just channel */
	if (in->master.i2c.configreg != head->master.i2c.configchip) {
		return SetConfiguration(in->master.i2c.configreg, in);
	}

	return gbGOOD;
}

/* Set the configuration register, both for this channel, and for head global data */
/* Note, config is stored as only the lower nibble */
static GOOD_OR_BAD SetConfiguration(BYTE c, struct connection_in *in)
{
	struct connection_in *head = in->master.i2c.head;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->pown->file_descriptor;
	int read_back;

	/* Write, readback, and compare configuration register */
	/* Logic error fix from Uli Raich */
	if (i2c_smbus_write_byte_data(file_descriptor, DS2482_CMD_WRITE_CONFIG, c | ((~c) << 4))
		|| (read_back = i2c_smbus_read_byte(file_descriptor)) < 0 || ((BYTE) read_back != c)
		) {
		head->master.i2c.configchip = 0xFF;	// bad value to trigger retry
		LEVEL_CONNECT("Trouble changing DS2482 configuration register "I2Cformat" ",I2Cvar(in));
		return gbBAD;
	}
	/* Clear the strong pull-up power bit(register is automatically cleared by reset) */
	in->master.i2c.configreg = head->master.i2c.configchip = c & ~DS2482_REG_CFG_SPU;
	return gbGOOD;
}

static GOOD_OR_BAD DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	
	/* Make sure we're using the correct channel */
	RETURN_BAD_IF_BAD(DS2482_channel_select(in)) ;

	/* Set the power (bit is automatically cleared by reset) */
	TrafficOut("power write", &byte, 1, in ) ;
	RETURN_BAD_IF_BAD(SetConfiguration(  in->master.i2c.configreg | DS2482_REG_CFG_SPU, in)) ;

	/* send and get byte (and trigger strong pull-up */
	RETURN_BAD_IF_BAD(DS2482_send_and_get( in->pown->file_descriptor, byte, resp)) ;
	TrafficOut("power response", resp, 1, in ) ;

	UT_delay(delay);

	return gbGOOD;
}

static void DS2482_close(struct connection_in *in)
{
	if (in == NO_CONNECTION) {
		return;
	}
}
#endif							/* OW_I2C */
