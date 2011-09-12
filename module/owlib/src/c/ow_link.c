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
struct LINK_id LINK_id_tbl[] = {
	{"1.0", "LinkHub-E v1.0", adapter_LINK_E},
	{"1.1", "LinkHub-E v1.1", adapter_LINK_E},

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
static GOOD_OR_BAD LINK_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t size, const struct parsedname *pn);
static GOOD_OR_BAD LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD LINK_PowerBit(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static void LINK_close(struct connection_in *in) ;

static GOOD_OR_BAD LinkHubE_Control( struct connection_in * in_link ) ;
static GOOD_OR_BAD Control_com( struct connection_in * in_control ) ;


static void LINK_setroutines(struct connection_in *in);
static void LINKE_setroutines(struct connection_in *in);

static RESET_TYPE LINK_reset_in(struct connection_in * in);
static GOOD_OR_BAD LINK_detect_serial(struct connection_in * in) ;
static GOOD_OR_BAD LINK_detect_net(struct connection_in * in) ;
static GOOD_OR_BAD LINK_version(struct connection_in * in) ;

static void LINK_set_baud(struct connection_in * in) ;
static GOOD_OR_BAD LINK_read(BYTE * buf, size_t size, struct connection_in * in);
static GOOD_OR_BAD LINK_read_true_length(BYTE * buf, size_t size, struct connection_in *in) ;
static GOOD_OR_BAD LINK_write(const BYTE * buf, size_t size, struct connection_in *in);
static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct connection_in * in);
static GOOD_OR_BAD LINK_search_type(struct device_search *ds, struct connection_in * in) ;

static GOOD_OR_BAD LINK_readback_data( BYTE * resp, const size_t size, struct connection_in * in);

static GOOD_OR_BAD LinkVersion_knownstring( const char * reported_string, struct connection_in * in ) ;
static GOOD_OR_BAD LinkVersion_unknownstring( const char * reported_string, struct connection_in * in ) ;
static void LINK_flush( struct connection_in * in ) ;
static void LINK_slurp(struct connection_in *in);

static void LINK_setroutines(struct connection_in *in)
{
	in->iroutines.detect = LINK_detect;
	in->iroutines.reset = LINK_reset;
	in->iroutines.next_both = LINK_next_both;
	in->iroutines.PowerByte = LINK_PowerByte;
	in->iroutines.PowerBit = LINK_PowerBit;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = LINK_sendback_data;
    in->iroutines.sendback_bits = LINK_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = LINK_close;
	in->iroutines.flags = ADAP_FLAG_no2409path | ADAP_FLAG_no2404delay ;
	in->bundling_length = LINK_FIFO_SIZE;
}

static void LINKE_setroutines(struct connection_in *in)
{
	LINK_setroutines(in) ;
	in->bundling_length = LINKE_FIFO_SIZE;
}

#define LINK_string(x)  ((BYTE *)(x))

static GOOD_OR_BAD LinkVersion_knownstring( const char * reported_string, struct connection_in * in )
{
	int version_index;

	// bad string
	if ( reported_string == NULL || reported_string[0] == '\0' ) {
		return LinkVersion_unknownstring(reported_string,in) ;
	}

	// loop through LINK version string table looking for a match
	for (version_index = 0; LINK_id_tbl[version_index].verstring[0] != '0'; version_index++) {
		if (strstr(reported_string, LINK_id_tbl[version_index].verstring) != NULL) {
			LEVEL_DEBUG("Link version Found %s", LINK_id_tbl[version_index].verstring);
			in->Adapter = LINK_id_tbl[version_index].Adapter;
			in->adapter_name = LINK_id_tbl[version_index].name;
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
	if (SOC(in)->devicename == NULL) {
		return gbBAD;
	}

	COM_set_standard( in ) ; // standard COM port settings

	switch( SOC(in)->type ) {
		case ct_telnet:
			// LinkHub-E
			SOC(in)->baud = B115200 ;
			LEVEL_DEBUG("Attempt connection to networked LINK at 115200 baud");
			RETURN_GOOD_IF_GOOD(  LINK_detect_net( in )  );

			// Xport or ser2net
			SOC(in)->baud = B9600 ;
			LEVEL_DEBUG("Attempt connection to networked LINK at 9600 baud");
			RETURN_GOOD_IF_GOOD(  LINK_detect_net( in )  );

			// Now try control port reset
			RETURN_BAD_IF_BAD( LinkHubE_Control( in ) ) ;
			SOC(in)->baud = B115200 ;
			LEVEL_DEBUG("Reattempt LINK connection") ;
			UT_delay( 5000 ) ;
			RETURN_GOOD_IF_GOOD(  LINK_detect_net( in )  );
			
			break ;

			

		case ct_serial:
			SOC(in)->baud = B9600 ;

			SOC(in)->flow = flow_first ;
			RETURN_GOOD_IF_GOOD( LINK_detect_serial(in) ) ;

			LEVEL_DEBUG("Second attempt at serial LINK setup");
			SOC(in)->flow = flow_second ;
			RETURN_GOOD_IF_GOOD( LINK_detect_serial(in) ) ;

			LEVEL_DEBUG("Third attempt at serial LINK setup");
			SOC(in)->flow = flow_first ;
			RETURN_GOOD_IF_GOOD( LINK_detect_serial(in) ) ;

			LEVEL_DEBUG("Fourth attempt at serial LINK setup");
			SOC(in)->flow = flow_second ;
			RETURN_GOOD_IF_GOOD( LINK_detect_serial(in) ) ;
			break ;

		default:
			return gbBAD ;
	}
	return gbBAD ;
}

static GOOD_OR_BAD LINK_detect_serial(struct connection_in * in)
{
	/* Set up low-level routines */
	LINK_setroutines(in);
	SOC(in)->timeout.tv_sec = Globals.timeout_serial ;
	SOC(in)->timeout.tv_usec = 0 ;

	/* Open the com port */
	RETURN_BAD_IF_BAD(COM_open(in)) ;
	
	//COM_break( in ) ;
	LEVEL_DEBUG("Slurp in initial bytes");
	LINK_slurp( in ) ;
	UT_delay(100) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	LINK_slurp( in ) ;
	
	RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;
	LEVEL_DEFAULT("LINK detection error");
	COM_close(in) ;
	return gbBAD;
}

static GOOD_OR_BAD LINK_detect_net(struct connection_in * in)
{
	/* Set up low-level routines */
	LINKE_setroutines(in);
	SOC(in)->timeout.tv_sec = 0 ;
	SOC(in)->timeout.tv_usec = 300000 ;

	/* Open the tcp port */
	RETURN_BAD_IF_BAD( COM_open(in) ) ;
	
	LEVEL_DEBUG("Slurp in initial bytes");
//	LINK_slurp( in ) ;
	UT_delay(1000) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	LINK_slurp( in ) ;
//	LINK_flush(in);

	SOC(in)->dev.telnet.telnet_negotiated = needs_negotiation ;
	RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;

	// second try -- send a break and line settings
	LEVEL_DEBUG("Second try -- send BREAK");
	COM_flush(in) ;
	COM_break(in);
	telnet_change(in);
//	LINK_slurp( in ) ;
	RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;

	LEVEL_DEFAULT("LINK detection error");
	COM_close(in) ;
	return gbBAD;
}

static GOOD_OR_BAD LINK_version(struct connection_in * in)
{
	char version_string[MAX_LINK_VERSION_LENGTH+1] ;
	enum { lvs_string, lvs_0d, } lvs = lvs_string ; // read state machine
	int version_index ;

	in->master.link.tmode = e_link_t_unknown ;
	in->master.link.qmode = e_link_t_unknown ;

	// clear out the buffer
	memset(version_string, 0, MAX_LINK_VERSION_LENGTH+1);

	if ( BAD( LINK_write(LINK_string(" "), 1, in) ) ) {
		LEVEL_DEFAULT("LINK version string cannot be requested");
		return gbBAD ;
	}
		
	/* read the version string */
	LEVEL_DEBUG("Checking LINK version");

	// need to read 1 char at a time to get a short string
	for ( version_index=0 ; version_index<MAX_LINK_VERSION_LENGTH ; ++version_index ) {
		if ( BAD( LINK_read_true_length( (BYTE *) &(version_string[version_index]), 1, in)) ) {
			LEVEL_DEBUG("Cannot read a full CRLF version string");
			switch ( lvs ) {
				case lvs_string:
					LEVEL_DEBUG("Found string <%s> but no 0x0D 0x0A",version_string) ;
					return gbBAD ;
				case lvs_0d:
					LEVEL_DEBUG("Found string <%s> but no 0x0A",version_string) ;
					return gbBAD ;
				default:
					return gbBAD ;
			}
		}
		
		switch( version_string[version_index] ) {
			case 0x0D:
				switch ( lvs ) {
					case lvs_string:
						lvs = lvs_0d ;
						// end of text
						version_string[version_index] = '\0' ;
						break ;
					case lvs_0d:
						LEVEL_DEBUG("Extra CR char in <%s>",version_string);
						return gbBAD ;
				}
				break ;
			case 0x0A:
				switch ( lvs ) {
					case lvs_string:
						LEVEL_DEBUG("No CR before LF char in <%s>",version_string);
						return gbBAD ;
					case lvs_0d:
						return LinkVersion_knownstring( version_string, in ) ;
				}
			default:
				switch ( lvs ) {
					case lvs_string:
						// add to string
						break ; 
					case lvs_0d:
						++in->master.serial.tcp.CRLF_size;
						break ;
				}
				break ;
		}
	}
	LEVEL_DEFAULT("LINK version string too long. <%s> greater than %d chars",version_string,MAX_LINK_VERSION_LENGTH);
	return gbBAD ;
}

static void LINK_set_baud(struct connection_in * in)
{
	char * speed_code ;

	if ( SOC(in)->type == ct_telnet ) {
		// telnet pinned at 115200
		return ;
	}

	COM_BaudRestrict( &(SOC(in)->baud), B9600, B19200, B38400, B57600, 0 ) ;

	LEVEL_DEBUG("to %d",COM_BaudRate(SOC(in)->baud));
	// Find rate parameter
	switch ( SOC(in)->baud ) {
		case B9600:
			COM_break(in) ;
			LINK_flush(in);
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
		SOC(in)->baud = B9600 ;
		++in->changed_bus_settings ;
		return ;
	}


	// Send configuration change
	LINK_flush(in);

	// Change OS view of rate
	UT_delay(5);
	COM_change(in) ;
	UT_delay(5);
	LINK_slurp(in);

	return ;
}

static void LINK_flush( struct connection_in * in )
{
	COM_flush(in) ;
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
		LINK_slurp(in);
		return BUS_RESET_ERROR;
	}
}

static enum search_status LINK_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;

	//Special case for DS2409 hub, use low-level code
	if ( pn->pathlength>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(LINK_directory(ds, in)) ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp) ) ) {
		case 0:
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
	COM_slurp(in);
}

static GOOD_OR_BAD LINK_read(BYTE * buf, size_t size, struct connection_in *in)
{
	return LINK_read_true_length( buf, size+in->master.serial.tcp.CRLF_size, in ) ;
}

static GOOD_OR_BAD LINK_read_true_length(BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_read( buf, size, in ) ;
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

	LEVEL_DEBUG("Test to see if LINK supports the tF0 command");
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
					in->master.link.tmode = e_link_t_extra ;
					response_length = 3 ;
					LINK_slurp(in);
					break;
			}
			break ;	
		case e_link_t_extra:
			response_length = 3 ;
			break ;
		case e_link_t_none:
		default:
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

static GOOD_OR_BAD LINK_directory(struct device_search *ds, struct connection_in * in)
{
	char resp[DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH+in->master.serial.tcp.CRLF_size];

	DirblobClear( &(ds->gulp) );

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
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0x00)) {
			LEVEL_DEBUG("BAD family or CRC8");
			return gbBAD;
		}

		DirblobAdd(sn,  &(ds->gulp) );

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
	// the standard COM_free routine cleans up the connection
	(void) in ;
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
	
	RETURN_BAD_IF_BAD( LINK_readback_data( LINK_string(respond), 2, in) ) ;
	
	resp[0] = string2num((const ASCII *) respond);
	return gbGOOD ;
}

//  _sendback_bits
//  Send data and return response block
//  return 0=good
static GOOD_OR_BAD LINK_PowerBit(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	BYTE buf[2+1+1+in->master.serial.tcp.CRLF_size] ;
	
	buf[0] = '~';			//put in power bit mode
	buf[1] = data ? '1' : '0' ;
	
	// send to LINK (wait for final CR)
	RETURN_BAD_IF_BAD(LINK_write(buf, 2, in) ) ;

	// delay
	UT_delay(delay);
	
	// // take out of power bit mode
	RETURN_BAD_IF_BAD(LINK_write(LINK_string("\r"), 1, in) ) ;
		
	// read back
	RETURN_BAD_IF_BAD( LINK_readback_data(buf, 1, in) ) ;
	
	// place data (converted back to hex) in resp
	resp[0] = (buf[0]=='0') ? 0x00 : 0xFF ;
	return gbGOOD;
}


//  _sendback_data
//  Send data and return response block
//  return 0=good
#define LINK_SEND_SIZE  32
static GOOD_OR_BAD LINK_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t left = size;
	size_t location = 0 ;
	BYTE buf[1+LINK_SEND_SIZE*2+1+1+in->master.serial.tcp.CRLF_size] ;
	
	if (size == 0) {
		return gbGOOD;
	}
	
	//Debug_Bytes( "ELINK sendback send", data, size) ;
	while (left > 0) {
		// Loop through taking only 32 bytes at a time
		size_t this_length = (left > LINK_SEND_SIZE) ? LINK_SEND_SIZE : left;
		size_t this_length2 = 2 * this_length ; // doubled for switch from hex to ascii
		
		buf[0] = 'b';			//put in byte mode
		bytes2string((char *) &buf[1], &data[location], this_length); // load in data as ascii data
		buf[1+this_length2] = '\r';	// take out of byte mode
		
		// send to LINK
		RETURN_BAD_IF_BAD(LINK_write(buf, 1+this_length2+1, in) ) ;
		
		// read back
		RETURN_BAD_IF_BAD( LINK_readback_data(buf, this_length2, in) ) ;
		
		// place data (converted back to hex) in resp
		string2bytes((char *) buf, &resp[location], this_length);
		left -= this_length;
		location += this_length ;
	}
	//Debug_Bytes( "ELINK sendback get", resp, size) ;
	return gbGOOD;
}

//  _sendback_bits
//  Send data and return response block
//  return 0=good
static GOOD_OR_BAD LINK_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t left = size;
	size_t location = 0 ;
	BYTE buf[1+LINK_SEND_SIZE+1+1+in->master.serial.tcp.CRLF_size] ;
	
	if (size == 0) {
		return gbGOOD;
	}
	
	Debug_Bytes( "ELINK sendback bits send", databits, size) ;
	while (left > 0) {
		// Loop through taking only 32 bytes at a time
		size_t this_length = (left > LINK_SEND_SIZE) ? LINK_SEND_SIZE : left;
		size_t i ;
		
		buf[0] = 'j';			//put in bit mode
		for ( i=0 ; i<this_length ; ++i ) {
			buf[i+1] = databits[i+location] ? '1' : '0' ;
		}
		buf[1+this_length] = '\r';	// take out of bit mode
		
		// send to LINK
		RETURN_BAD_IF_BAD(LINK_write(buf, 1+this_length+1, in) ) ;
		
		// read back
		RETURN_BAD_IF_BAD( LINK_readback_data(buf, this_length, in) ) ;
		
		// place data (converted back to hex) in resp
		for ( i=0 ; i<this_length ; ++i ) {
			respbits[i+location] = (buf[i]=='0') ? 0x00 : 0xFF ;
		}
		left -= this_length;
		location += this_length ;
	}
	Debug_Bytes( "ELINK sendback bits get", respbits, size) ;
	return gbGOOD;
}

static GOOD_OR_BAD LINK_readback_data( BYTE * buf, const size_t size, struct connection_in * in)
{
	int qmode_extra ;
	
	switch ( in->master.link.qmode ) {
		case e_link_t_extra:
			qmode_extra = 1 ;
			break ;
		case e_link_t_unknown:
		case e_link_t_none:
		default:
			qmode_extra = 0 ;
			break ;
	}
			
	// read back
	RETURN_BAD_IF_BAD( LINK_read(buf, size+qmode_extra, in) ) ;
	
	// see if we've yet tested the extra '?' "feature"
	if ( in->master.link.qmode == e_link_t_unknown ) {
		if ( buf[size] != 0x0D ) {
			in->master.link.qmode = e_link_t_extra ;
			LINK_slurp(in) ;
		} else {
			in->master.link.qmode = e_link_t_none ;
		}
	}
		
	return gbGOOD;
}

// use the Xport control port to reset
static GOOD_OR_BAD LinkHubE_Control( struct connection_in * in_link )
{
	struct connection_in * in_control = AllocIn( NO_CONNECTION ) ;

	if ( SOC(in_link)->dev.tcp.host == NULL ) {
		LEVEL_DEBUG("No Xport control on local machine");
		return gbBAD ;
	}

	if ( in_control != NO_CONNECTION ) {
		GOOD_OR_BAD ret ;

		SOC(in_control)->type = ct_telnet ;
		SOC(in_control)->devicename = owstrdup( SOC(in_link)->dev.tcp.host ) ;
		in_control->busmode = bus_xport_control ;
		SOC(in_control)->timeout.tv_sec = 1 ;
		SOC(in_control)->timeout.tv_usec = 0 ;

		ret = Control_com( in_control ) ;

		RemoveIn( in_control ) ;
		return ret ;
	}
	return gbBAD ;
}

static GOOD_OR_BAD Control_com( struct connection_in * in_control )
{
	RETURN_BAD_IF_BAD( COM_open( in_control ) ) ;
	COM_slurp( in_control ) ;
	RETURN_BAD_IF_BAD( COM_write_simple( (BYTE *) "\r", 1, in_control ) ) ;
	COM_slurp( in_control ) ;
	RETURN_BAD_IF_BAD( COM_write_simple( (BYTE *) "9\r", 2, in_control ) ) ;
	COM_slurp( in_control ) ;
	return gbGOOD ;
}
