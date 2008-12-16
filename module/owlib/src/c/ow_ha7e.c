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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

struct HA7E_id {
	char verstring[36];
	char name[30];
	enum adapter_type Adapter;
};

// Steven Bauer added code for the VM links
static struct HA7E_id HA7E_id_tbl[] = {
	{"1.0", "HA7E v1.0", adapter_HA7E_10},
	{"1.1", "HA7E v1.1", adapter_HA7E_11},
	{"1.2", "HA7E v1.2", adapter_HA7E_12},
	{"VM12a", "HA7E OEM v1.2a", adapter_HA7E_12},
	{"VM12", "HA7E OEM v1.2", adapter_HA7E_12},
	{"0", "0", 0}
};


//static void byteprint( const BYTE * b, int size ) ;
static int HA7E_read(BYTE * buf, const size_t size, const struct parsedname *pn);
static int HA7E_write(const BYTE * buf, const size_t size, const struct parsedname *pn);
static int HA7E_reset(const struct parsedname *pn);
static int HA7E_next_both(struct device_search *ds, const struct parsedname *pn);
static int HA7E_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int HA7E_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn);
static int HA7E_CR(const struct parsedname *pn);
static void HA7E_setroutines(struct connection_in *in);
static void HA7E_close(struct connection_in *in);
static int HA7E_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);

static void HA7E_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA7E_detect;
	in->iroutines.reset = HA7E_reset;
	in->iroutines.next_both = HA7E_next_both;
	in->iroutines.PowerByte = HA7E_PowerByte;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = HA7E_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = HA7E_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_2409path;
	in->bundling_length = HA7E_FIFO_SIZE;
}

#define HA7E_string(x)  ((BYTE *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
// bus locking done at a higher level
// Looks up the device by comparing the version strings to the ones in the
// HA7E_id_table
int HA7E_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	HA7E_setroutines(in);

	/* Initialize dir-at-once structures */
	DirblobInit(&(in->connin.link.main));
	DirblobInit(&(in->connin.link.alarm));


	/* Open the com port */
	if (COM_open(in)) {
		return -ENODEV;
	}

	// set the baud rate to 9600. (Already set to 9600 in COM_open())
	COM_speed(B9600, &pn);
	//COM_flush(&pn);
	if (HA7E_reset(&pn) == BUS_RESET_OK && HA7E_write(HA7E_string(" "), 1, &pn) == 0) {

		BYTE version_read_in[36] = "(none)";
		char *version_pointer = (char *) version_read_in;

		/* read the version string */
		LEVEL_DEBUG("Checking HA7E version\n");

		memset(version_read_in, 0, 36);
		HA7E_read(version_read_in, 36, &pn);	// ignore return value -- will time out, probably
		Debug_Bytes("Read version from link", version_read_in, 36);

		COM_flush(&pn);

		/* Now find the dot for the version parsing */
		if (version_pointer) {
			int version_index;
			for (version_index = 0; HA7E_id_tbl[version_index].verstring[0] != '0'; version_index++) {
				if (strstr(version_pointer, HA7E_id_tbl[version_index].verstring) != NULL) {
					LEVEL_DEBUG("Link version Found %s\n", HA7E_id_tbl[version_index].verstring);
					in->Adapter = HA7E_id_tbl[version_index].Adapter;
					in->adapter_name = HA7E_id_tbl[version_index].name;
					return 0;
				}
			}
		}
	}
	LEVEL_DEFAULT("HA7E detection error\n");
	return -ENODEV;
}

static int HA7E_reset(const struct parsedname *pn)
{
	BYTE resp[5];
	int ret;

	COM_flush(pn);
	//if (HA7E_write(HA7E_string("\rr"), 2, pn) || HA7E_read(resp, 4, pn, 1)) {
	//Response is 3 bytes:  1 byte for code + \r\n
	if (HA7E_write(HA7E_string("r"), 1, pn) || HA7E_read(resp, 3, pn)) {
		LEVEL_DEBUG("Error resetting HA7E device\n");
		return -EIO;
	}

	switch (resp[0]) {

	case 'P':
		LEVEL_DEBUG("HA7E reset ok, devices Present\n");
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 1;
		break;

	case 'N':
		LEVEL_DEBUG("HA7E reset ok, devices Not present\n");
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 0;
		break;

	case 'S':
		LEVEL_DEBUG("HA7E reset short, Short circuit on 1-wire bus!\n");
		ret = BUS_RESET_SHORT;
		break;

	default:
		LEVEL_DEBUG("HA7E reset bad, Unknown HA7E response %c\n", resp[0]);
		ret = -EIO;
		break;
	}

	return ret;
}

static int HA7E_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->connin.link.alarm) : &(pn->selected_connection->connin.link.main);

	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

	COM_flush(pn);

	if (ds->index == -1) {
		if (HA7E_directory(ds, db, pn)) {
			return -EIO;
		}
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d\n", ds->index);

	ret = DirblobGet(ds->index, ds->sn, db);
	LEVEL_DEBUG("DirblobGet %d\n", ret);
	switch (ret) {
	case 0:
		if ((ds->sn[0] & 0x7F) == 0x04) {
			/* We found a DS1994/DS2404 which require longer delays */
			pn->selected_connection->ds2404_compliance = 1;
		}
		break;
	case -ENODEV:
		ds->LastDevice = 1;
		break;
	}

	LEVEL_DEBUG("HA7E_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return ret;
}

/* Assymetric */
/* Read from HA7E with timeout on each character */
// NOTE: from PDkit, reead 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for HA7E
// returns 0=good 1=bad
/* return 0=good,
          -errno = read error
          -EINTR = timeout
 */
static int HA7E_read(BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_read( buf, size, pn ) ;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
   Special processing for the remote hub (add 0x0A)
 */
static int HA7E_write(const BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_write( buf, size, pn ) ;
}

static int HA7E_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{

	if (HA7E_write(HA7E_string("p"), 1, pn)
		|| HA7E_byte_bounce(&data, resp, pn)) {
		return -EIO;			// send just the <CR>
	}
	// delay
	UT_delay(delay);

	// flush the buffers
	return HA7E_CR(pn);			// send just the <CR>
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t i;
	size_t left;
	BYTE *buf = pn->selected_connection->combuffer;

	if (size == 0) {
		return 0;
	}
	if (HA7E_write(HA7E_string("b"), 1, pn)) {
		LEVEL_DEBUG("HA7E_sendback_data error sending b\n");
		return -EIO;
	}
//    for ( i=0; ret==0 && i<size ; ++i ) ret = HA7E_byte_bounce( &data[i], &resp[i], pn ) ;
	for (left = size; left;) {
		i = (left > 16) ? 16 : left;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) buf, &data[size - left], i);
		if (HA7E_write(buf, i << 1, pn) || HA7E_read(buf, i << 1, pn)) {
			return -EIO;
		}
		string2bytes((char *) buf, &resp[size - left], i);
		left -= i;
	}
	return HA7E_CR(pn);
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) { printf( "%.2X ",b[i] ) ; }
    if ( size ) { printf("\n") ; }
}
*/

static int HA7E_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn)
{
	BYTE data[2];

	num2string((char *) data, out[0]);
	if (HA7E_write(data, 2, pn) || HA7E_read(data, 2, pn)) {
		return -EIO;
	}
	in[0] = string2num((char *) data);
	return 0;
}

static int HA7E_CR(const struct parsedname *pn)
{
	BYTE data[3];
	if (HA7E_write(HA7E_string("\r"), 1, pn) || HA7E_read(data, 2, pn)) {
		return -EIO;
	}
	LEVEL_DEBUG("HA7E_CR return 0\n");
	return 0;
}

/********************************************************/
/* HA7E_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void HA7E_close(struct connection_in *in)
{
	DirblobClear(&(in->connin.link.main));
	DirblobClear(&(in->connin.link.alarm));
	COM_close(in);

}

/************************************************************************/
/*									                                     */
/*	HA7E_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/*									                                     */
/************************************************************************/
static int HA7E_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	char resp[21];
	BYTE sn[8];
	int ret;

	DirblobClear(db);

	//Depending on the search type, the HA7E search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		if ((ret = HA7E_write(HA7E_string("tEC"), 3, pn))) {
			return ret;
		}
		if ((ret = (HA7E_read(HA7E_string(resp), 5, pn)))) {
			return ret;
		}
		if (strncmp(resp, ",EC", 3) != 0) {
			LEVEL_DEBUG("Did not change to conditional search");
			return -EIO;
		}
		LEVEL_DEBUG("HA7E set for conditional search\n");
	} else {
		if ((ret = HA7E_write(HA7E_string("tF0"), 3, pn))) {
			return ret;
		}
		if ((ret = (HA7E_read(HA7E_string(resp), 5, pn)))) {
			return ret;
		}
		if (strncmp(resp, ",F0", 3) != 0) {
			LEVEL_DEBUG("Did not change to normal search");
			return -EIO;
		}
		LEVEL_DEBUG("HA7E set for normal search\n");
	}

	if ((ret = HA7E_write(HA7E_string("f"), 1, pn))) {
		return ret;
	}
	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the HA7E
	//device

	if ((ret = HA7E_read(HA7E_string(resp), 1, pn))) {
		return ret;
	}

	if (resp[0] == 'E') {
		if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
			LEVEL_DEBUG("HA7E returned E: No devices in alarm\n");
			return 0;
		}
	}

	if ((ret = HA7E_read(HA7E_string((resp + 1)), 19, pn))) {
		return ret;
	}

	// Check if we should start scanning
	switch (resp[0]) {
	case '-':
	case '+':
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			pn->selected_connection->AnyDevices = 1;
		}
		break;
	case 'N':
		LEVEL_DEBUG("HA7E returned N: Empty bus\n");
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			pn->selected_connection->AnyDevices = 0;
		}
	case 'X':
	default:
		LEVEL_DEBUG("HA7E_search default case\n");
		return -EIO;
	}

	/* Join the loop after the first query -- subsequent handled differently */
	while ((resp[0] == '+') || (resp[0] == '-')) {
		sn[7] = string2num(&resp[2]);
		sn[6] = string2num(&resp[4]);
		sn[5] = string2num(&resp[6]);
		sn[4] = string2num(&resp[8]);
		sn[3] = string2num(&resp[10]);
		sn[2] = string2num(&resp[12]);
		sn[1] = string2num(&resp[14]);
		sn[0] = string2num(&resp[16]);
		LEVEL_DEBUG("HA7E_directory SN found: " SNformat "\n", SNvar(sn));

		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s\n", sn);
			return -EIO;
		}

		DirblobAdd(sn, db);

		switch (resp[0]) {
		case '+':
			// get next element
			if ((ret = HA7E_write(HA7E_string("n"), 1, pn))) {
				return ret;
			}
			if ((ret = HA7E_read(HA7E_string((resp)), 20, pn))) {
				return ret;
			}
			break;
		case '-':
			return 0;
		default:
			break;
		}
	}
	return 0;
}
