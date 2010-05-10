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

/* Telnet handling concepts from Jerry Scharf:

You should request the number of bytes you expect. When you scan it,
start looking for FF FA codes. If that shows up, see if the F0 is in the
read buffer. If so, move the char pointer back to where the FF point was
and ask for the rest of the string (expected - FF...F0 bytes.) If the F0
isn't in the read buffer, start reading byte by byte into a separate
variable looking for F0. Once you find that, then do the move the
pointer and read the rest piece as above. Finally, don't forget to scan
the rest of the strings for more FF FA blocks. It's most sloppy when the
FF is the last character of the initial read buffer...

You can also scan and remove the patterns FF F1 - FF F9 as these are 2
byte commands that the transmitter can send at any time. It is also
possible that you could see FF FB xx - FF FE xx 3 byte codes, but this
would be in response to FF FA codes that you would send, so that seems
unlikely. Handling these would be just the same as the FF FA codes above.

*/
struct LINK_id {
	char verstring[36];
	char name[30];
	enum adapter_type Adapter;
};

// Steven Bauer added code for the VM links
struct LINK_id LINKE_id_tbl[] = {
	{"1.0", "LinkHub-E v1.0", adapter_LINK_E},
	{"1.1", "LinkHub-E v1.1", adapter_LINK_E},
	{"0", "0", 0}
};

struct LINK_id LINK_id_tbl[] = {
	{"1.0", "LINK v1.0", adapter_LINK_10},
	{"1.1", "LINK v1.1", adapter_LINK_11},
	{"1.2", "LINK v1.2", adapter_LINK_12},
	{"VM12a", "LINK OEM v1.2a", adapter_LINK_12},
	{"VM12", "LINK OEM v1.2", adapter_LINK_12},
	{"1.3", "LinkUSB V1.3", adapter_LINK_13},
	{"1.4", "LinkUSB V1.4", adapter_LINK_14},
	{"0", "0", 0}
};

#define MAX_LINK_VERSION_LENGTH	36

static GOOD_OR_BAD LINK_serial_detect(struct parsedname * pn_minimal) ;
static GOOD_OR_BAD LINK_net_detect(struct parsedname * pn_minimal) ;

//static void byteprint( const BYTE * b, int size ) ;
static void LINK_set_baud(const struct parsedname *pn) ;
static GOOD_OR_BAD LINK_read(BYTE * buf, const size_t size, int extra_net, const struct parsedname *pn);
static GOOD_OR_BAD LINK_write(const BYTE * buf, const size_t size, const struct parsedname *pn);
static RESET_TYPE LINK_reset(const struct parsedname *pn);
static enum search_status LINK_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void LINK_setroutines(struct connection_in *in);
static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static GOOD_OR_BAD LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static void LINK_close(struct connection_in *in) ;

static GOOD_OR_BAD LinkVersion_knownstring( const char * reported_string, struct LINK_id * tbl, struct connection_in * in ) ;
static GOOD_OR_BAD LinkVersion_unknownstring( const char * reported_string, struct connection_in * in ) ;
static void LINK_flush( struct connection_in * in ) ;

//static void byteprint( const BYTE * b, int size ) ;
static void LINKE_setroutines(struct connection_in *in);

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
	in->iroutines.close = LINK_close;
	in->iroutines.flags = ADAP_FLAG_no2409path;
	in->bundling_length = LINK_FIFO_SIZE;
}

static void LINKE_setroutines(struct connection_in *in)
{
	LINK_setroutines(in) ;
	in->bundling_length = LINKE_FIFO_SIZE;
}

#define LINK_string(x)  ((BYTE *)(x))

static GOOD_OR_BAD LinkVersion_knownstring( const char * reported_string, struct LINK_id * tbl, struct connection_in * in )
{
	int version_index;

	// loop through LINK version string table looking for a match
	for (version_index = 0; tbl[version_index].verstring[0] != '0'; version_index++) {
		if (strstr(reported_string, tbl[version_index].verstring) != NULL) {
			LEVEL_DEBUG("Link version Found %s", tbl[version_index].verstring);
			in->Adapter = tbl[version_index].Adapter;
			in->adapter_name = tbl[version_index].name;
			return gbGOOD;
		}
	}
	return LinkVersion_unknownstring(reported_string,in) ;
}

static GOOD_OR_BAD LinkVersion_unknownstring( const char * reported_string, struct connection_in * in )
{
	const char * version_pointer;
	
	// Apparently strcasestr isn't available by default, will hard code:
	for ( version_pointer = reported_string ; version_pointer != '\0' ; ++version_pointer )  {
		switch ( *version_pointer ) {
			case 'l':
			case 'L':
				if ( strncasecmp( "link", version_pointer, 4 ) == 0 ) {
					LEVEL_DEBUG("Link version is unrecognized: %s (but that's ok).", reported_string);
					in->Adapter = adapter_LINK_other;
					in->adapter_name = "Other LINK";
					return gbGOOD;
				}
				break ;
			default:
				break ;
		}
	}
	return gbBAD;
}

// bus locking done at a higher level
GOOD_OR_BAD LINK_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	if (in->name == NULL) {
		return gbBAD;
	}

	switch( in->busmode ) {
		case bus_elink:
			return LINK_net_detect( &pn ) ;
		case bus_link:
			return LINK_serial_detect(&pn) ;
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD LINK_serial_detect(struct parsedname * pn_minimal)
{
	struct connection_in * in =  pn_minimal->selected_connection ;
	
	/* Set up low-level routines */
	LINK_setroutines(in);
	
	/* Open the com port */
	RETURN_BAD_IF_BAD(COM_open(in)) ;
	
	COM_break( in ) ;
	COM_slurp( in->file_descriptor ) ;
	UT_delay(100) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	
	//COM_flush(in);
	if (LINK_reset(pn_minimal) == BUS_RESET_OK && GOOD( LINK_write(LINK_string(" "), 1, pn_minimal) ) ) {
		char version_read_in[MAX_LINK_VERSION_LENGTH] ;
		memset(version_read_in, 0, MAX_LINK_VERSION_LENGTH);
		
		/* read the version string */
		LEVEL_DEBUG("Checking LINK version");
		
		LINK_read((BYTE *)version_read_in, MAX_LINK_VERSION_LENGTH, 0, pn_minimal);	// ignore return value -- will time out, probably
		Debug_Bytes("Read version from link", (BYTE*)version_read_in, MAX_LINK_VERSION_LENGTH);
		
		LINK_flush(in);
		
		/* Now find the dot for the version parsing */
		if ( version_read_in!=NULL && GOOD( LinkVersion_knownstring(version_read_in,LINK_id_tbl,in)) ) {
			in->baud = Globals.baud ;
			++in->changed_bus_settings ;
			BUS_reset(pn_minimal) ; // extra reset
			return gbGOOD;
		}
	}
	LEVEL_DEFAULT("LINK detection error");
	return gbBAD;
}

static GOOD_OR_BAD LINK_net_detect(struct parsedname * pn_minimal)
{
	struct connection_in * in =  pn_minimal->selected_connection ;
	
	LINKE_setroutines(in);
	
	RETURN_BAD_IF_BAD(ClientAddr(in->name, in)) ;

	in->file_descriptor = ClientConnect(in) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(in->file_descriptor) ) {
		return gbBAD;
	}
	
	in->default_discard = 0 ;

	if (1) {
		BYTE data[1] ;
		size_t read_size ;
		struct timeval tvnetfirst = { Globals.timeout_network, 0, };
		tcp_read(in->file_descriptor, data, 1, &tvnetfirst, &read_size ) ;
	}
	LEVEL_DEBUG("Slurp in initial bytes");
	TCP_slurp( in->file_descriptor ) ;
	LINK_flush(in);

	if ( GOOD( LINK_write(LINK_string(" "), 1, pn_minimal) ) ) {
		char version_read_in[MAX_LINK_VERSION_LENGTH] ;
		int version_index ;
		memset(version_read_in, 0, MAX_LINK_VERSION_LENGTH);
		
		/* read the version string */
		LEVEL_DEBUG("Checking LINK version");

		// need to read 1 char at a time to get a short string
		for ( version_index=0 ; version_index<MAX_LINK_VERSION_LENGTH ; ++version_index ) {
			if ( BAD( LINK_read((BYTE *)&version_read_in[version_index], 1, 0, pn_minimal)) ) {
				break ;	// ignore return value -- will time out, probably
			}
		}
		Debug_Bytes("Read version from link", (BYTE*)version_read_in, MAX_LINK_VERSION_LENGTH);
		/* Now find the dot for the version parsing */
		if ( version_read_in!=NULL && GOOD( LinkVersion_knownstring(version_read_in,LINKE_id_tbl,in)) ) {
			BUS_reset(pn_minimal) ; // extra reset
			return gbGOOD;
		}
	}
	LEVEL_DEFAULT("LINK detection error");
	return gbBAD;
}

static void LINK_set_baud(const struct parsedname *pn)
{
	char * speed_code ; // 

	if ( pn->selected_connection->busmode == bus_elink ) {
		return ;
	}

	COM_BaudRestrict( &(pn->selected_connection->baud), B9600, B19200, B38400, B57600, 0 ) ;

	LEVEL_DEBUG("to %d",COM_BaudRate(pn->selected_connection->baud));
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
		default:
			LEVEL_DEBUG("Unrecognized baud rate");
			return ;
	}

	LEVEL_DEBUG("LINK change baud string <%s>",speed_code);
	LINK_flush(pn->selected_connection);
	if ( BAD( LINK_write(LINK_string(speed_code), 1, pn) ) ) {
		LEVEL_DEBUG("LINK change baud error -- will return to 9600");
		pn->selected_connection->baud = B9600 ;
		++pn->selected_connection->changed_bus_settings ;
		return ;
	}


	// Send configuration change
	LINK_flush(pn->selected_connection);

	// Change OS view of rate
	UT_delay(5);
	COM_speed(pn->selected_connection->baud,pn->selected_connection) ;
	UT_delay(5);
	COM_slurp(pn->selected_connection->file_descriptor);

	return ;
}

static void LINK_flush( struct connection_in * in )
{
	switch( in->busmode ) {
		case bus_elink:
			tcp_read_flush(in->file_descriptor) ;
			break ;
		default:
			COM_flush(in) ;
			break ;
	}
}

static RESET_TYPE LINK_reset(const struct parsedname *pn)
{
	BYTE resp[3+1];

	if (pn->selected_connection->changed_bus_settings > 0) {
		--pn->selected_connection->changed_bus_settings ;
		LINK_set_baud(pn);	// reset paramters
	} else {
		LINK_flush(pn->selected_connection);
	}

	//Response is 3 bytes:  1 byte for code + \r\n
	if ( BAD(LINK_write(LINK_string("r"), 1, pn) || BAD( LINK_read(resp, 3, 1, pn))) ) {
		LEVEL_DEBUG("Error resetting LINK device");
		return -EIO;
	}

	switch (resp[0]) {

	case 'P':
		LEVEL_DEBUG("ok, devices Present");
		pn->selected_connection->AnyDevices = anydevices_yes;
		return BUS_RESET_OK;
	case 'N':
		LEVEL_DEBUG("ok, devices Not present");
		pn->selected_connection->AnyDevices = anydevices_no;
		return BUS_RESET_OK;
	case 'S':
		LEVEL_DEBUG("short, Short circuit on 1-wire bus!");
		return BUS_RESET_SHORT;
	default:
		LEVEL_DEBUG("bad, Unknown LINK response %c", resp[0]);
		return -EIO;
	}
}

static enum search_status LINK_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->alarm) : &(pn->selected_connection->main);

	//Special case for DS2409 hub, use low-level code
	if ( pn->pathlength>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	LINK_flush(pn->selected_connection);

	if (ds->index == -1) {
		if ( BAD(LINK_directory(ds, db, pn)) ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, db) ) {
		case 0:
			if ((ds->sn[0] & 0x7F) == 0x04) {
				/* We found a DS1994/DS2404 which require longer delays */
				pn->selected_connection->ds2404_compliance = 1;
			}
			LEVEL_DEBUG("SN found: " SNformat "", SNvar(ds->sn));
			return search_good;
		case -ENODEV:
		default:
			ds->LastDevice = 1;
			LEVEL_DEBUG("SN finished");
			return search_done;
	}
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
static GOOD_OR_BAD LINK_read(BYTE * buf, const size_t size, int extra_net_byte, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	switch ( in->busmode ) {
		case bus_elink:
			// Only need to add an extra byte sometimes
			return telnet_read( buf, size+extra_net_byte, pn ) ;
		default:
			return COM_read( buf, size, in )? gbBAD : gbGOOD ;
	}
}

// Write a string to the serial port
// return 0=good,
//          -EIO = error
//Special processing for the remote hub (add 0x0A)
static GOOD_OR_BAD LINK_write(const BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_write( buf, size, pn->selected_connection ) ? gbBAD : gbGOOD ;
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) { printf( "%.2X ",b[i] ) ; }
    if ( size ) { printf("\n") ; }
}
*/

/************************************************************************/
/*									*/
/*	LINK_directory: searches the Directory stores it in a dirblob	*/
/*			& stores in in a dirblob object depending if it */
/*			Supports conditional searches of the bus for 	*/
/*			/alarm branch					*/
/*                                                                      */
/* Only called for the first element, everything else comes from dirblob*/
/* returns 0 even if no elements, errors only on communication errors   */
/*									*/
/************************************************************************/
static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	char resp[21];

	DirblobClear(db);

	//Depending on the search type, the LINK search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		RETURN_BAD_IF_BAD(LINK_write(LINK_string("tEC"), 3, pn)) ;
		RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), 5, 1, pn)) ;
		if (strstr(resp, "EC") == NULL) {
			LEVEL_DEBUG("Did not change to conditional search");
			return gbBAD;
		}
		LEVEL_DEBUG("LINK set for conditional search");
	} else {
		RETURN_BAD_IF_BAD( LINK_write(LINK_string("tF0"), 3, pn));
		RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), 5, 1, pn));
		if (strstr(resp, "F0") == NULL) {
			LEVEL_DEBUG("Did not change to normal search");
			return gbBAD;
		}
		LEVEL_DEBUG("LINK set for normal search");
	}

	RETURN_BAD_IF_BAD(LINK_write(LINK_string("f"), 1, pn)) ;

	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the LINK
	//device

	if ( BAD(LINK_read(LINK_string(resp), 1, 0, pn)) ) {
		return -EIO;
	}
	
	switch (resp[0]) {
		case 'E':
			LEVEL_DEBUG("LINK returned E: No devices in alarm");
			// pass through
		case 'N':
			// remove extra 2 bytes
			LEVEL_DEBUG("LINK returned E or N: Empty bus");
			if ( BAD(LINK_read(LINK_string(&resp[1]), 2, 1, pn)) ) {
				return -EIO;
			}
			if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
				pn->selected_connection->AnyDevices = anydevices_no;
			}
			return 0 ;
		default:
			break ;
	}
	
	if ( BAD(LINK_read(LINK_string(&resp[1]), 19, 1, pn)) ) {
		return -EIO;
	}

	// Check if we should start scanning
	switch (resp[0]) {
	case '-':
	case '+':
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			pn->selected_connection->AnyDevices = anydevices_yes;
		}
		break;
	default:
		LEVEL_DEBUG("LINK_search default case");
		return -EIO;
	}

	/* Join the loop after the first query -- subsequent handled differently */
	while ((resp[0] == '+') || (resp[0] == '-')) {
		BYTE sn[SERIAL_NUMBER_SIZE];

		sn[7] = string2num(&resp[2]);
		sn[6] = string2num(&resp[4]);
		sn[5] = string2num(&resp[6]);
		sn[4] = string2num(&resp[8]);
		sn[3] = string2num(&resp[10]);
		sn[2] = string2num(&resp[12]);
		sn[1] = string2num(&resp[14]);
		sn[0] = string2num(&resp[16]);
		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));

		// CRC check
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s", sn);
			return -EIO;
		}

		DirblobAdd(sn, db);

		switch (resp[0]) {
		case '+':
			// get next element
			if ( BAD(LINK_write(LINK_string("n"), 1, pn))) {
				return -EIO;
			}
			if ( BAD(LINK_read(LINK_string((resp)), 20, 1, pn)) ) {
				return -EIO;
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

static void LINK_close(struct connection_in *in)
{
	switch( in->busmode ) {
		case bus_elink:
			Test_and_Close( &(in->file_descriptor) ) ;
			FreeClientAddr(in);
			break ;
		case bus_link:
			COM_close(in) ;
			break ;
		default:
			break ;
	}
}

static GOOD_OR_BAD LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	ASCII buf[3] = "pxx";
	BYTE discard[3] ;
	
	num2string(&buf[1], data);
	
	RETURN_BAD_IF_BAD(LINK_write(LINK_string(buf), 3, pn) ) ;
	RETURN_BAD_IF_BAD( LINK_read(LINK_string(buf), 2, 0, pn) ) ;
	
	resp[0] = string2num(buf);
	
	// delay
	UT_delay(delay);
	
	// flush the buffers
	RETURN_BAD_IF_BAD(LINK_write(LINK_string("\r"), 1, pn) ) ;
	return LINK_read(discard, 2, 1, pn) ;
}

//  _sendback_data
//  Send data and return response block
//  return 0=good
// Assume buffer length (combuffer) is 1 + 32*2 + 1
#define LINK_SEND_SIZE  32
static GOOD_OR_BAD LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t left = size;
	BYTE buf[1+LINK_SEND_SIZE*2+1] ;
	
	if (size == 0) {
		return gbGOOD;
	}
	
	Debug_Bytes( "ELINK sendback send", data, size) ;
	while (left > 0) {
		size_t this_length = (left > LINK_SEND_SIZE) ? LINK_SEND_SIZE : left;
		size_t total_length = 2 * this_length + 2;
		buf[0] = 'b';			//put in byte mode
		//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) &buf[1], &data[size - left], this_length);
		buf[total_length - 1] = '\r';	// take out of byte mode
		RETURN_BAD_IF_BAD(LINK_write(buf, total_length, pn) ) ;
		RETURN_BAD_IF_BAD( LINK_read(buf, total_length, 1, pn) ) ;
		string2bytes((char *) buf, &resp[size - left], this_length);
		left -= this_length;
	}
	return gbGOOD;
}
