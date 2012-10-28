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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
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

/* Simultaneous is a trigger to do a mass conversion on all the devices in the specified path */

/* Added "present" From Jan Kandziora to search for any devices */

#include <config.h>
#include "owfs_config.h"
#include "ow_simultaneous.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
READ_FUNCTION(FS_r_convert);
WRITE_FUNCTION(FS_w_convert_temp);
WRITE_FUNCTION(FS_w_convert_volt);
READ_FUNCTION(FS_r_present);
READ_FUNCTION(FS_r_single);

/* -------- Structures ---------- */
static struct filetype simultaneous[] = {
	{"temperature", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_convert, FS_w_convert_temp, VISIBLE, {i:simul_temp}, },
	{"voltage", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_convert, FS_w_convert_volt, VISIBLE, {i:simul_volt}, },
	{"present", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_present, NO_WRITE_FUNCTION, VISIBLE, {i:_1W_READ_ROM}, },
	{"present_ds2400", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_present, NO_WRITE_FUNCTION, VISIBLE, {i:_1W_OLD_READ_ROM}, },
	{"single", 18, NON_AGGREGATE, ft_ascii, fc_volatile, FS_r_single, NO_WRITE_FUNCTION, VISIBLE, {i:_1W_READ_ROM}, },
	{"single_ds2400", 18, NON_AGGREGATE, ft_ascii, fc_volatile, FS_r_single, NO_WRITE_FUNCTION, VISIBLE, {i:_1W_OLD_READ_ROM}, },
};

DeviceEntry(simultaneous, simultaneous, NO_GENERIC_READ, NO_GENERIC_WRITE);

// in this order: enum sumul_type { simul_temp, simul_volt, } ;
struct internal_prop ipSimul[] = {
	{"temperature", fc_volatile},
	{"voltage", fc_volatile},
};

#define _1W_CONVERT_T             0x44
#define _1W_READ_POWERMODE        0xB4

/* ------- Functions ------------ */
static void OW_single2cache(BYTE * sn, const struct parsedname *pn2);

GOOD_OR_BAD FS_Test_Simultaneous( enum simul_type type, UINT delay, const struct parsedname * pn)
{
	time_t dwell_time ;
	time_t remaining_delay ;

	//LEVEL_DEBUG("TEST Simultaneous valid?");
	if( BAD( Cache_Get_Simul_Time(type, &dwell_time, pn)) ) {
		LEVEL_DEBUG("No simultaneous conversion currently valid");
		return gbBAD ; // No simultaneous valid
	}

	remaining_delay = delay - 1000 * dwell_time ;
	LEVEL_DEBUG("TEST remaining delay=%ld, delay=%ld, 1000*dwelltime=%ld",(long int)remaining_delay,(long int)delay, (long int)(1000*dwell_time));
	if ( remaining_delay > 0 ) {
		LEVEL_DEBUG("Simultaneous conversion requires %d msec delay",(int) remaining_delay);
		UT_delay(remaining_delay) ;
	} else {
		LEVEL_DEBUG("Simultaneous conversion, no delay");
	}
	return gbGOOD ;
}

static ZERO_OR_ERROR FS_w_convert_temp(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct parsedname s_pn_directory;
	struct parsedname * pn_directory = &s_pn_directory ;
	struct connection_in * in = pn->selected_connection ;

	const BYTE cmd_temp[] = { _1W_SKIP_ROM, _1W_CONVERT_T };
	const BYTE cmd_powermode[] = { _1W_READ_POWERMODE, };
	BYTE pow[1] ;
	struct transaction_log tpower[] = {
		TRXN_START,
		TRXN_WRITE1(cmd_powermode),
		TRXN_READ1(pow),
		TRXN_END,
	};
	struct transaction_log t_powered_convert[] = {
		TRXN_START,
		TRXN_WRITE2(cmd_temp),
		TRXN_END,
	};
	struct transaction_log t_unpowered_convert[] = {
		TRXN_START,
		TRXN_WRITE2(cmd_temp),
		TRXN_DELAY(1000),
		TRXN_END,
	};

	if (OWQ_Y(owq) == 0) {
		return 0;				// don't send convert
	}
	
	switch (in->Adapter) {
		case adapter_Bad:
		case adapter_w1_monitor:
		case adapter_browse_monitor:
		case adapter_usb_monitor:
		case adapter_fake:
		case adapter_tester:
		case adapter_mock:
			/* Since writing to /simultaneous/temperature is done recursive to all
			* adapters, we have to fake a successful write even if it's detected
			* as an unsupported adapter. */
			return gbGOOD ;
		default:
			break ;
	}

	FS_LoadDirectoryOnly(pn_directory, pn); // setup up for full directory message

	// Get Power status
	RETURN_BAD_IF_BAD(BUS_transaction(tpower, pn_directory)) ;
	//LEVEL_DEBUG("TEST read power");
	
	Cache_Add_Simul(simul_temp, pn_directory);	// Mark start time
	if ( pow[0] != 0 ) {
		// powered
		// Send the conversion and let the timing work out when the actual
		// temperature reading is requested
		if ( GOOD(BUS_transaction(t_powered_convert, pn_directory) ) ) {
			return 0 ;
		}
	} else {
		// Unpowered prohibits other bus traffic.
		// This is at the port level, so could be all channels of the DS2482-800, etc
		if ( GOOD(BUS_transaction(t_unpowered_convert, pn_directory) )) {
			return 0 ;
		}
	}

	Cache_Del_Simul(simul_temp, pn_directory);	// Clear start time
	LEVEL_DEBUG("Trouble setting simultaneous for %s",pn_directory->path);
	return -EINVAL ;
}

static ZERO_OR_ERROR FS_w_convert_volt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct parsedname pn_directory;
	
	if (OWQ_Y(owq) == 0) {
		return 0;				// don't send convert
	}
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	Cache_Del_Internal(&ipSimul[simul_volt], &pn_directory);	// remove existing entry
	
	/* Since writing to /simultaneous/temperature is done recursive to all
	* adapters, we have to fake a successful write even if it's detected
	* as a bad adapter. */
	if (pn->selected_connection->Adapter != adapter_Bad) {
		BYTE cmd_volt[] = { _1W_SKIP_ROM, 0x3C, 0x0F, 0x00, 0xFF, 0xFF };
		struct transaction_log t[] = {
			TRXN_START,
			TRXN_WR_CRC16(cmd_volt, 4, 0),
			TRXN_DELAY(5),
			TRXN_END,
		};
		if ( GOOD(BUS_transaction(t, &pn_directory)) ) {
			Cache_Add_SlaveSpecific(NULL, 0, &ipSimul[simul_volt], &pn_directory);
		}
	}
	return 0;
}

static ZERO_OR_ERROR FS_r_convert(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct parsedname pn_directory;

	FS_LoadDirectoryOnly(&pn_directory, pn);
	OWQ_Y(owq) = GOOD (Cache_Get_SlaveSpecific(NULL, 0, &ipSimul[pn->selected_filetype->data.i], &pn_directory) );
	return 0;
}

static ZERO_OR_ERROR FS_r_present(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	switch (pn->selected_connection->Adapter) {
		case adapter_fake:
		case adapter_tester:
			// fake adapter -- simple memory look
			OWQ_Y(owq) = (DirblobElements(&(pn->selected_connection->master.fake.main)) > 0);
		default:
		{
			struct parsedname pn_directory;
			BYTE read_ROM[1] ;
			BYTE resp[SERIAL_NUMBER_SIZE];
			BYTE collisions[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
			BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
			struct transaction_log t[] = {
				TRXN_START,
				TRXN_WRITE1(read_ROM),
				TRXN_READ(resp, 8),
				TRXN_END,
			};

			/* check if DS2400 compatibility is needed */
			read_ROM[0] = _1W_READ_ROM;

			FS_LoadDirectoryOnly(&pn_directory, pn);
			RETURN_ERROR_IF_BAD(BUS_transaction(t, &pn_directory)) ;
			if (memcmp(resp, collisions, SERIAL_NUMBER_SIZE)) {	// all devices
				OWQ_Y(owq) = 1;		// YES present
			} else if (memcmp(resp, match, SERIAL_NUMBER_SIZE)) {	// some device(s) complained
					OWQ_Y(owq) = 1;		// YES present
				if (CRC8(resp, SERIAL_NUMBER_SIZE)) {
					return 0;		// crc8 error -- more than one device
				}
				OW_single2cache(resp, &pn_directory);
			} else {				// no devices
				OWQ_Y(owq) = 0;
			}
		}
			break ;
	}
	return 0;
}

static ZERO_OR_ERROR FS_r_single(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	ASCII ad[30] = { 0x00, };	// long enough -- default "blank"
	BYTE resp[SERIAL_NUMBER_SIZE];

	switch (pn->selected_connection->Adapter) {
		case adapter_fake:
		case adapter_tester:
		case adapter_mock:
			if (DirblobElements(&(pn->selected_connection->master.fake.main)) == 1) {
				DirblobGet(0, resp, &(pn->selected_connection->master.fake.main));
				FS_devicename(ad, sizeof(ad), resp, pn);
			}
			break ;
		default:
		{
			struct parsedname pn_directory;
			BYTE read_ROM[1] ;
			BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
			BYTE collisions[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
			struct transaction_log t[] = {
				TRXN_START,
				TRXN_WRITE1(read_ROM),
				TRXN_READ(resp, SERIAL_NUMBER_SIZE),
				TRXN_END,
			};

			/* check if DS2400 compatibility is needed */
			read_ROM[0] = pn->selected_filetype->data.i;

			FS_LoadDirectoryOnly(&pn_directory, pn);
			RETURN_ERROR_IF_BAD(BUS_transaction(t, &pn_directory)) ;
			LEVEL_DEBUG("dat=" SNformat " crc8=%02x", SNvar(resp), CRC8(resp, 7));
			if ((memcmp(resp, collisions, SERIAL_NUMBER_SIZE) != 0) && (memcmp(resp, match, SERIAL_NUMBER_SIZE) != 0) && (CRC8(resp, SERIAL_NUMBER_SIZE) == 0)) {	// non-empty, and no CRC error
				OW_single2cache(resp, &pn_directory);
				/* Return device id. */
				FS_devicename(ad, sizeof(ad), resp, pn);
			} else {
				/* Jan Kandziora:
				I think it's not really an error if no iButton is connected to a lock. As
				EINVAL is given if something went wrong with the host adapter (e.g. pulled
				from the host), too, those errors are then obscured by catching the "no key
				connected" EINVAL.
				*/
				ad[0] = '\0' ;
			}
		}
			break ;

	}
	return OWQ_format_output_offset_and_size_z(ad, owq);
}

// Do cache for single item
static void OW_single2cache(BYTE * sn, const struct parsedname *pn)
{
	struct dirblob db;
	DirblobInit(&db);
	DirblobAdd(sn, &db);
	if (DirblobPure(&db)) {
		Cache_Add_Dir(&db, pn);	// Directory cache
	}
	Cache_Add_Device(pn->selected_connection->index, sn);	// Device cache
}
