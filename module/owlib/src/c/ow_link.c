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

static RESET_TYPE LINK_reset(const struct parsedname *pn);
static enum search_status LINK_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static void LINK_close(struct connection_in *in) ;

static void LINK_setroutines(struct connection_in *in);
static void LINKE_setroutines(struct connection_in *in);

static RESET_TYPE LINK_reset_in(struct connection_in * in);
static GOOD_OR_BAD LINK_serial_detect(struct connection_in * in) ;
static GOOD_OR_BAD LINK_net_detect(struct connection_in * in) ;
static char * LINK_version_string(struct connection_in * in) ;

static void LINK_set_baud(struct connection_in * in) ;
static GOOD_OR_BAD LINK_read(BYTE * buf, size_t size, struct connection_in * in);
static GOOD_OR_BAD LINK_write(const BYTE * buf, size_t size, struct connection_in *in);
static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct dirblob *db, struct connection_in * in);
static GOOD_OR_BAD LINK_search_type(struct device_search *ds, struct connection_in * in) ;

static GOOD_OR_BAD LinkVersion_knownstring( const char * reported_string, struct LINK_id * tbl, struct connection_in * in ) ;
static GOOD_OR_BAD LinkVersion_unknownstring( const char * reported_string, struct connection_in * in ) ;
static void LINK_flush( struct connection_in * in ) ;
static void LINK_slurp(struct connection_in *in);

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
	if (in->name == NULL) {
		return gbBAD;
	}

	in->master.serial.tcp.CRLF_size = 2 ;

	switch( in->busmode ) {
		case bus_elink:
			return LINK_net_detect( in ) ;
		case bus_link:
			RETURN_GOOD_IF_GOOD( LINK_serial_detect(in) ) ;
			RETURN_GOOD_IF_GOOD( LINK_serial_detect(in) ) ;
			return LINK_serial_detect(in) ;
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD LINK_serial_detect(struct connection_in * in)
{
	char * version_string ;
	
	/* Set up low-level routines */
	LINK_setroutines(in);
	
	/* Open the com port */
	RETURN_BAD_IF_BAD(COM_open(in)) ;
	
	COM_break( in ) ;
	LINK_slurp( in ) ;
	UT_delay(100) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	
	version_string = LINK_version_string(in) ;
	if ( version_string == NULL ) {
		LEVEL_DEBUG("Cannot get version string");
		LINK_close(in) ;
		return gbBAD;
	}

	/* Now find the dot for the version parsing */
	if ( GOOD( LinkVersion_knownstring( version_string, LINK_id_tbl, in)) ) {
		owfree(version_string);
		in->baud = Globals.baud ;
		++in->changed_bus_settings ;
		LINK_reset_in(in) ; // extra reset
		return gbGOOD;
	}

	owfree(version_string);
	LEVEL_DEFAULT("LINK detection error");
	LINK_close(in) ;
	return gbBAD;
}

static GOOD_OR_BAD LINK_net_detect(struct connection_in * in)
{
	char * version_string ;

	/* Set up low-level routines */
	LINKE_setroutines(in);
	
	/* Open the tcp port */
	RETURN_BAD_IF_BAD(ClientAddr(in->name, DEFAULT_LINK_PORT, in)) ;
	in->file_descriptor = ClientConnect(in) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(in->file_descriptor) ) {
		return gbBAD;
	}
	
	if (1) {
		BYTE data[1] ;
		size_t read_size ;
		struct timeval tvnetfirst = { Globals.timeout_network, 0, };
		tcp_read(in->file_descriptor, data, 1, &tvnetfirst, &read_size ) ;
	}
	LEVEL_DEBUG("Slurp in initial bytes");
	LINK_slurp( in ) ;
	LINK_flush(in);

	version_string = LINK_version_string(in) ;
	if ( version_string != NULL ) {
		/* Now find the dot for the version parsing */
		if ( GOOD( LinkVersion_knownstring(version_string,LINKE_id_tbl,in)) ) {
			owfree(version_string) ;
			LINK_reset_in(in) ; // extra reset
			return gbGOOD;
		}
		owfree(version_string) ;
	}

	LEVEL_DEFAULT("LINK detection error");
	LINK_close(in) ;
	return gbBAD;
}

static char * LINK_version_string(struct connection_in * in)
{
	char * version_string = owmalloc(MAX_LINK_VERSION_LENGTH) ;
	int version_index ;
	int index_0D = 0 ;

	in->master.serial.tcp.default_discard = 0 ;
	in->master.serial.tcp.CRLF_size = 0 ;
	in->master.link.tmode = e_link_t_unknown ;

	if ( version_string == NULL ) {
		LEVEL_DEBUG( "cannot allocate version string" );
		return NULL ;
	}
	memset(version_string, 0, MAX_LINK_VERSION_LENGTH);

	if ( BAD( LINK_write(LINK_string(" "), 1, in) ) ) {
		LEVEL_DEFAULT("LINK version string cannot be requested");
		owfree(version_string);
		return NULL ;
	}
		
	/* read the version string */
	LEVEL_DEBUG("Checking LINK version");

	// need to read 1 char at a time to get a short string
	for ( version_index=0 ; version_index<MAX_LINK_VERSION_LENGTH ; ++version_index ) {
		if ( BAD( LINK_read( (BYTE *) &(version_string[version_index]), 1, in)) ) {
			LEVEL_DEBUG("Cannot read a full CRLF version string");
			owfree(version_string);
			return NULL ;
		}
		switch( version_string[version_index] ) {
			case 0x0D:
				index_0D = version_index ;
				break ;
			case 0x0A:
				in->master.serial.tcp.CRLF_size = version_index - index_0D + 1 ;
				LEVEL_DEBUG("CRLF string length = %d",in->master.serial.tcp.CRLF_size) ;
				return version_string ;
			default:
				break ;
		}
	}
	owfree(version_string) ;
	LEVEL_DEFAULT("LINK version string too long");
	return NULL ;
}

static void LINK_set_baud(struct connection_in * in)
{
	char * speed_code ;

	if ( in->busmode == bus_elink ) {
		return ;
	}

	COM_BaudRestrict( &(in->baud), B9600, B19200, B38400, B57600, 0 ) ;

	LEVEL_DEBUG("to %d",COM_BaudRate(in->baud));
	// Find rate parameter
	switch ( in->baud ) {
		case B9600:
			COM_break(in) ;
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
	LINK_flush(in);
	if ( BAD( LINK_write(LINK_string(speed_code), 1, in) ) ) {
		LEVEL_DEBUG("LINK change baud error -- will return to 9600");
		in->baud = B9600 ;
		++in->changed_bus_settings ;
		return ;
	}


	// Send configuration change
	LINK_flush(in);

	// Change OS view of rate
	UT_delay(5);
	COM_speed(in->baud,in) ;
	UT_delay(5);
	LINK_slurp(in);

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
	return LINK_reset_in(pn->selected_connection);
}

static RESET_TYPE LINK_reset_in(struct connection_in * in)
{
	BYTE resp[1+in->master.serial.tcp.CRLF_size];

	if (in->changed_bus_settings > 0) {
		--in->changed_bus_settings ;
		LINK_set_baud(in);	// reset paramters
	} else {
		LINK_flush(in);
	}

	//Response is 3 bytes:  1 byte for code + \r\n
	if ( BAD(LINK_write(LINK_string("r"), 1, in) || BAD( LINK_read(resp, 1, in))) ) {
		LEVEL_DEBUG("Error resetting LINK device");
		LINK_slurp(in);
		return BUS_RESET_ERROR;
	}

	switch (resp[0]) {

	case 'P':
		in->AnyDevices = anydevices_yes;
		return BUS_RESET_OK;
	case 'N':
		in->AnyDevices = anydevices_no;
		return BUS_RESET_OK;
	case 'S':
		return BUS_RESET_SHORT;
	default:
		LEVEL_DEBUG("bad, Unknown LINK response %c", resp[0]);
		return BUS_RESET_ERROR;
	}
}

static enum search_status LINK_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(in->alarm) : &(in->main);

	//Special case for DS2409 hub, use low-level code
	if ( pn->pathlength>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(LINK_directory(ds, db, in)) ) {
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

static void LINK_slurp(struct connection_in *in)
{
	switch ( in->busmode ) {
		case bus_elink:
			// Only need to add an extra byte sometimes
			TCP_slurp(in->file_descriptor);
		default:
			COM_slurp(in->file_descriptor);
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
static GOOD_OR_BAD LINK_read(BYTE * buf, size_t size, struct connection_in *in)
{
	switch ( in->busmode ) {
		case bus_elink:
			// Only need to add an extra byte sometimes
			return telnet_read( buf, size+in->master.serial.tcp.CRLF_size, in ) ;
		default:
			return COM_read( buf, size+in->master.serial.tcp.CRLF_size, in ) ;
	}
}

// Write a string to the serial port
// return 0=good,
//          -EIO = error
//Special processing for the remote hub (add 0x0A)
static GOOD_OR_BAD LINK_write(const BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_write( buf, size, in ) ;
}

static GOOD_OR_BAD LINK_search_type(struct device_search *ds, struct connection_in * in)
{
	char resp[3+in->master.serial.tcp.CRLF_size];
	int response_length ;
	
	switch ( in->master.link.tmode ) {
		case e_link_t_unknown:
			RETURN_BAD_IF_BAD( LINK_write(LINK_string("tF0"), 3, in));
			RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), 2, in));
			switch ( resp[0] ) {
				case 'F':
					in->master.link.tmode = e_link_t_none ;
					response_length = 2 ;
					break;
				default:
					in->master.link.tmode = e_link_t_comma ;
					response_length = 3 ;
					LINK_slurp(in);
					break;
			}
			break ;	
		case e_link_t_comma:
			response_length = 3 ;
			break ;
		case e_link_t_none:
			response_length = 2 ;
			break ;
	}

	//Depending on the search type, the LINK search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching
	

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		RETURN_BAD_IF_BAD(LINK_write(LINK_string("tEC"), 3, in)) ;
		RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), response_length, in)) ;
		if (strstr(resp, "EC") == NULL) {
			LEVEL_DEBUG("Did not change to conditional search");
			return gbBAD;
		}
		LEVEL_DEBUG("LINK set for conditional search");
	} else {
		RETURN_BAD_IF_BAD( LINK_write(LINK_string("tF0"), 3, in));
		RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), response_length, in));
		if (strstr(resp, "F0") == NULL) {
			LEVEL_DEBUG("Did not change to normal search");
			return gbBAD;
		}
		LEVEL_DEBUG("LINK set for normal search");
	}
	return gbGOOD ;
}

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
#define DEVICE_LENGTH  16
#define COMMA_LENGTH 1
#define PLUS_LENGTH 1

static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct dirblob *db, struct connection_in * in)
{
	char resp[DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH+in->master.serial.tcp.CRLF_size];

	DirblobClear(db);

	// Send the configuration command and check response
	RETURN_BAD_IF_BAD( LINK_search_type( ds, in )) ;

	// send the first search
	RETURN_BAD_IF_BAD(LINK_write(LINK_string("f"), 1, in)) ;

	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the LINK
	//device

	RETURN_BAD_IF_BAD(LINK_read(LINK_string(resp), 1, in)) ;
	
	switch (resp[0]) {
		case 'E':
			LEVEL_DEBUG("LINK returned E: No devices in alarm");
			// pass through
		case 'N':
			// remove extra 2 bytes
			LEVEL_DEBUG("LINK returned E or N: Empty bus");
			if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
				in->AnyDevices = anydevices_no;
			}
			return gbGOOD ;
		default:
			break ;
	}
	
	if ( BAD(LINK_read(LINK_string(&resp[1+in->master.serial.tcp.CRLF_size]), DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH-1-in->master.serial.tcp.CRLF_size, in)) ) {
		return gbBAD;
	}

	// Check if we should start scanning
	switch (resp[0]) {
	case '-':
	case '+':
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			in->AnyDevices = anydevices_yes;
		}
		break;
	default:
		LEVEL_DEBUG("LINK_search unrecognized case");
		return gbBAD;
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
			LEVEL_DEBUG("sn = %s -- BAD family or CRC8", sn);
			return gbBAD;
		}

		DirblobAdd(sn, db);

		switch (resp[0]) {
		case '+':
			// get next element
			if ( BAD(LINK_write(LINK_string("n"), 1, in))) {
				return gbBAD;
			}
			if ( BAD(LINK_read(LINK_string((resp)), DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH, in)) ) {
				return gbBAD;
			}
			break;
		case '-':
			return gbGOOD;
		default:
			break;
		}
	}

	return gbGOOD;
}

static void LINK_close(struct connection_in *in)
{
	switch( in->busmode ) {
		case bus_elink:
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
	struct connection_in * in = pn->selected_connection ;
	ASCII buf[3] = "pxx";
	BYTE respond[2+in->master.serial.tcp.CRLF_size] ;
	
	num2string(&buf[1], data);
	
	RETURN_BAD_IF_BAD(LINK_write(LINK_string(buf), 3, in) ) ;
	
	// delay
	UT_delay(delay);
	
	// flush the buffers
	RETURN_BAD_IF_BAD(LINK_write(LINK_string("\r"), 1, in) ) ;
	
	RETURN_BAD_IF_BAD( LINK_read(LINK_string(respond), 2, in) ) ;
	
	resp[0] = string2num((const ASCII *) respond);
	return gbGOOD ;
}

//  _sendback_data
//  Send data and return response block
//  return 0=good
// Assume buffer length (combuffer) is 1 + 32*2 + 1
#define LINK_SEND_SIZE  32
static GOOD_OR_BAD LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t left = size;
	size_t location = 0 ;
	BYTE buf[1+LINK_SEND_SIZE*2+1+in->master.serial.tcp.CRLF_size] ;
	
	if (size == 0) {
		return gbGOOD;
	}
	
	//Debug_Bytes( "ELINK sendback send", data, size) ;
	while (left > 0) {
		size_t this_length = (left > LINK_SEND_SIZE) ? LINK_SEND_SIZE : left;
		buf[0] = 'b';			//put in byte mode
		//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) &buf[1], &data[location], this_length);
		buf[1+this_length*2] = '\r';	// take out of byte mode
		RETURN_BAD_IF_BAD(LINK_write(buf, 1+this_length*2+1, in) ) ;
		RETURN_BAD_IF_BAD( LINK_read(buf, this_length*2, in) ) ;
		string2bytes((char *) buf, &resp[size - left], this_length);
		left -= this_length;
		location += this_length ;
	}
	//Debug_Bytes( "ELINK sendback get", resp, size) ;
	return gbGOOD;
}
