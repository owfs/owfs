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

struct LINK_id {
	char verstring[36];
	char name[30];
	enum adapter_type Adapter;
};

// Steven Bauer added code for the VM links
static struct LINK_id LINK_id_tbl[] = {
	{"1.0", "LINK v1.0", adapter_LINK_10},
	{"1.1", "LINK v1.1", adapter_LINK_11},
	{"1.2", "LINK v1.2", adapter_LINK_12},
	{"VM12a", "LINK OEM v1.2a", adapter_LINK_12},
	{"VM12", "LINK OEM v1.2", adapter_LINK_12},
	{"0", "0", 0}
};


//static void byteprint( const BYTE * b, int size ) ;
static void LINK_set_baud(const struct parsedname *pn) ;
static int LINK_read(BYTE * buf, const size_t size, const struct parsedname *pn);
static int LINK_write(const BYTE * buf, const size_t size, const struct parsedname *pn);
static int LINK_reset(const struct parsedname *pn);
static int LINK_next_both(struct device_search *ds, const struct parsedname *pn);
static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static int LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int LINK_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn);
static int LINK_CR(const struct parsedname *pn);
static void LINK_setroutines(struct connection_in *in);
static int LINK_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);

static void LINK_setroutines(struct connection_in *in)
{
	in->iroutines.detect = LINK_detect;
	in->iroutines.reset = LINK_reset;
	in->iroutines.next_both = LINK_next_both;
	in->iroutines.PowerByte = LINK_PowerByte;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = LINK_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = COM_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_2409path;
	in->bundling_length = LINK_FIFO_SIZE;
}

#define LINK_string(x)  ((BYTE *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
// bus locking done at a higher level
// Looks up the device by comparing the version strings to the ones in the
// LINK_id_table
int LINK_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	LINK_setroutines(in);


	/* Open the com port */
	if (COM_open(in)) {
		return -ENODEV;
	}

	COM_break( in ) ;
	COM_slurp( in->file_descriptor ) ;
	UT_delay(100) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	
	//COM_flush(in);
	if (LINK_reset(&pn) == BUS_RESET_OK && LINK_write(LINK_string(" "), 1, &pn) == 0) {

		BYTE version_read_in[36] = "(none)";
		char *version_pointer = (char *) version_read_in;

		/* read the version string */
		LEVEL_DEBUG("Checking LINK version\n");

		memset(version_read_in, 0, 36);
		LINK_read(version_read_in, 36, &pn);	// ignore return value -- will time out, probably
		Debug_Bytes("Read version from link", version_read_in, 36);

		COM_flush(in);

		/* Now find the dot for the version parsing */
		if (version_pointer) {
			int version_index;
			for (version_index = 0; LINK_id_tbl[version_index].verstring[0] != '0'; version_index++) {
				if (strstr(version_pointer, LINK_id_tbl[version_index].verstring) != NULL) {
					LEVEL_DEBUG("Link version Found %s\n", LINK_id_tbl[version_index].verstring);
					in->Adapter = LINK_id_tbl[version_index].Adapter;
					in->adapter_name = LINK_id_tbl[version_index].name;

					in->baud = Globals.baud ;
					++in->changed_bus_settings ;
					
					return 0;
				}
			}
		}
	}
	LEVEL_DEFAULT("LINK detection error\n");
	return -ENODEV;
}

static void LINK_set_baud(const struct parsedname *pn)
{
	char * speed_code ;
	
	OW_BaudRestrict( &(pn->selected_connection->baud), B9600, B19200, B38400, B57600, 0 ) ;
	
	LEVEL_DEBUG("LINK attempt change baud to %d\n",OW_BaudRate(pn->selected_connection->baud));
	// Find rate parameter
	switch ( pn->selected_connection->baud ) {
		case B9600:
			COM_break(pn->selected_connection) ;
			return ;
		case B19200:
			speed_code = "," ;
			break ;
		case B38400:
			speed_code = "`" ;
			break ;
#ifdef B57600
		/* MacOSX support max 38400 in termios.h ? */
		case B57600:
			speed_code = "^" ;
			break ;
#endif
	}
	
	LEVEL_DEBUG("LINK change baud string <%s>\n",speed_code);
	COM_flush(pn->selected_connection);
	if ( LINK_write(LINK_string(speed_code), 1, pn) ) {
		LEVEL_DEBUG("LINK change baud error -- will return to 9600\n");
		pn->selected_connection->baud = B9600 ;
		++pn->selected_connection->changed_bus_settings ;
		return ;
	}
	
	
	// Send configuration change
	COM_flush(pn->selected_connection);
	
	// Change OS view of rate
	UT_delay(5);
	COM_speed(pn->selected_connection->baud,pn->selected_connection) ;
	UT_delay(5);
	COM_slurp(pn->selected_connection->file_descriptor);
	
	return ;
}


static int LINK_reset(const struct parsedname *pn)
{
	BYTE resp[5];
	int ret;

	if (pn->selected_connection->changed_bus_settings > 0) {
		--pn->selected_connection->changed_bus_settings ;
		LINK_set_baud(pn);	// reset paramters
	} else {
		COM_flush(pn->selected_connection);
	}
	
	//Response is 3 bytes:  1 byte for code + \r\n
	if (LINK_write(LINK_string("r"), 1, pn) || LINK_read(resp, 3, pn)) {
		LEVEL_DEBUG("Error resetting LINK device\n");
		return -EIO;
	}

	switch (resp[0]) {

	case 'P':
		LEVEL_DEBUG("LINK reset ok, devices Present\n");
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 1;
		break;

	case 'N':
		LEVEL_DEBUG("LINK reset ok, devices Not present\n");
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 0;
		break;

	case 'S':
		LEVEL_DEBUG("LINK reset short, Short circuit on 1-wire bus!\n");
		ret = BUS_RESET_SHORT;
		break;

	default:
		LEVEL_DEBUG("LINK reset bad, Unknown LINK response %c\n", resp[0]);
		ret = -EIO;
		break;
	}

	return ret;
}

static int LINK_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->alarm) : &(pn->selected_connection->main);

	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

	COM_flush(pn->selected_connection);

	if (ds->index == -1) {
		if (LINK_directory(ds, db, pn)) {
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

	LEVEL_DEBUG("LINK_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return ret;
}

/* Assymetric */
/* Read from LINK with timeout on each character */
// NOTE: from PDkit, reead 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK
// returns 0=good 1=bad
/* return 0=good,
          -errno = read error
          -EINTR = timeout
 */
static int LINK_read(BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_read( buf, size, pn ) ;	
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
   Special processing for the remote hub (add 0x0A)
 */
static int LINK_write(const BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_write( buf, size, pn ) ;	
}

static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{

	if (LINK_write(LINK_string("p"), 1, pn)
		|| LINK_byte_bounce(&data, resp, pn)) {
		return -EIO;			// send just the <CR>
	}
	// delay
	UT_delay(delay);

	// flush the buffers
	return LINK_CR(pn);			// send just the <CR>
}

// LINK_sendback_data
//  Send data and return response block
/* return 0=good
 */
static int LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t i;
	size_t left;
	BYTE *buf = pn->selected_connection->combuffer;

	if (size == 0) {
		return 0;
	}
	if (LINK_write(LINK_string("b"), 1, pn)) {
		LEVEL_DEBUG("LINK_sendback_data error sending b\n");
		return -EIO;
	}
//    for ( i=0; ret==0 && i<size ; ++i ) ret = LINK_byte_bounce( &data[i], &resp[i], pn ) ;
	for (left = size; left;) {
		i = (left > 16) ? 16 : left;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) buf, &data[size - left], i);
		if (LINK_write(buf, i << 1, pn) || LINK_read(buf, i << 1, pn)) {
			return -EIO;
		}
		string2bytes((char *) buf, &resp[size - left], i);
		left -= i;
	}
	return LINK_CR(pn);
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) { printf( "%.2X ",b[i] ) ; }
    if ( size ) { printf("\n") ; }
}
*/

static int LINK_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn)
{
	BYTE data[2];

	num2string((char *) data, out[0]);
	if (LINK_write(data, 2, pn) || LINK_read(data, 2, pn)) {
		return -EIO;
	}
	in[0] = string2num((char *) data);
	return 0;
}

static int LINK_CR(const struct parsedname *pn)
{
	BYTE data[3];
	if (LINK_write(LINK_string("\r"), 1, pn) || LINK_read(data, 2, pn)) {
		return -EIO;
	}
	LEVEL_DEBUG("LINK_CR return 0\n");
	return 0;
}

/************************************************************************/
/*									                                     */
/*	LINK_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/*									                                     */
/************************************************************************/
static int LINK_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	char resp[21];
	BYTE sn[8];
	int ret;

	DirblobClear(db);

	//Depending on the search type, the LINK search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		if ((ret = LINK_write(LINK_string("tEC"), 3, pn))) {
			return ret;
		}
		if ((ret = (LINK_read(LINK_string(resp), 5, pn)))) {
			return ret;
		}
		if (strncmp(resp, ",EC", 3) != 0) {
			LEVEL_DEBUG("Did not change to conditional search");
			return -EIO;
		}
		LEVEL_DEBUG("LINK set for conditional search\n");
	} else {
		if ((ret = LINK_write(LINK_string("tF0"), 3, pn))) {
			return ret;
		}
		if ((ret = (LINK_read(LINK_string(resp), 5, pn)))) {
			return ret;
		}
		if (strncmp(resp, ",F0", 3) != 0) {
			LEVEL_DEBUG("Did not change to normal search");
			return -EIO;
		}
		LEVEL_DEBUG("LINK set for normal search\n");
	}

	if ((ret = LINK_write(LINK_string("f"), 1, pn))) {
		return ret;
	}
	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick 
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the LINK
	//device

	if ((ret = LINK_read(LINK_string(resp), 1, pn))) {
		return ret;
	}

	if (resp[0] == 'E') {
		if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
			LEVEL_DEBUG("LINK returned E: No devices in alarm\n");
			return 0;
		}
	}

	if ((ret = LINK_read(LINK_string((resp + 1)), 19, pn))) {
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
		LEVEL_DEBUG("LINK returned N: Empty bus\n");
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			pn->selected_connection->AnyDevices = 0;
		}
	case 'X':
	default:
		LEVEL_DEBUG("LINK_search default case\n");
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
		LEVEL_DEBUG("LINK_directory SN found: " SNformat "\n", SNvar(sn));

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
			if ((ret = LINK_write(LINK_string("n"), 1, pn))) {
				return ret;
			}
			if ((ret = LINK_read(LINK_string((resp)), 20, pn))) {
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
