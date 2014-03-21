/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server

*/

/* New interface from Dirk Opfer
 * Code written by Dirk Opfer with minor changes for inclusion
 * 
 * Basically it's a serial bus master with an ascii interface
 * Electrically thit's a multichannel adapter with more flexibility durring active power mode 
 * so it can on other channels while temperature measurements ("conversion") is occurring
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

#define MAX_ELABNET_VERSION_LENGTH	36
#define MAX_SERIAL	8
#define ELABNET_FIFO_SIZE LINK_FIFO_SIZE

static RESET_TYPE ELABNET_reset(const struct parsedname *pn);
static enum search_status ELABNET_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD ELABNET_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD ELABNET_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t size, const struct parsedname *pn);
static GOOD_OR_BAD ELABNET_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD ELABNET_PowerBit(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD ELABNET_reconnect( const struct parsedname  *pn ) ;
static void ELABNET_close(struct connection_in *in) ;

static void ELABNET_setroutines(struct connection_in *in);

static RESET_TYPE ELABNET_reset_in(struct connection_in * in);
static GOOD_OR_BAD ELABNET_detect_serial(struct connection_in * in) ;
static GOOD_OR_BAD ELABNET_version(struct connection_in * in, char* type_string);

static void ELABNET_set_baud(struct connection_in * in) ;
static GOOD_OR_BAD ELABNET_read(BYTE * buf, size_t size, struct connection_in * in);
static GOOD_OR_BAD ELABNET_read_true_length(BYTE * buf, size_t size, struct connection_in *in) ;
static GOOD_OR_BAD ELABNET_write(const BYTE * buf, size_t size, struct connection_in *in);
static GOOD_OR_BAD ELABNET_directory(struct device_search *ds, struct connection_in * in);
static GOOD_OR_BAD ELABNET_search_type(struct device_search *ds, struct connection_in * in) ;

static GOOD_OR_BAD ELABNET_readback_data( BYTE * resp, const size_t size, struct connection_in * in);

static void ELABNET_flush( struct connection_in * in ) ;
static void ELABNET_slurp(struct connection_in *in);

static void ELABNET_setroutines(struct connection_in *in)
{
	in->iroutines.detect = ELABNET_detect;
	in->iroutines.reset = ELABNET_reset;
	in->iroutines.next_both = ELABNET_next_both;
	in->iroutines.PowerByte = ELABNET_PowerByte;
	in->iroutines.PowerBit = ELABNET_PowerBit;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = ELABNET_sendback_data;
	in->iroutines.sendback_bits = ELABNET_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = ELABNET_reconnect;
	in->iroutines.close = ELABNET_close;
	in->iroutines.flags = ADAP_FLAG_no2409path | ADAP_FLAG_no2404delay | ADAP_FLAG_unlock_during_delay;
	in->bundling_length = ELABNET_FIFO_SIZE;
}

#define ELABNET_string(x)  ((BYTE *)(x))

// bus locking done at a higher level
GOOD_OR_BAD ELABNET_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	/* By definition, this is the head adapter on this port */
	in->master.elabnet.head = in ;

	if (pin->init_data == NULL) {
		// requires input string
		LEVEL_DEFAULT("Elabnet busmaster requires port name");
		return gbBAD;
	}

	COM_set_standard( in ) ; // standard COM port settings

	switch( pin->type ) {

		case ct_serial:
			pin->baud = B9600 ;

			pin->flow = flow_first ;
			RETURN_GOOD_IF_GOOD( ELABNET_detect_serial(in) ) ;

			LEVEL_DEBUG("Second attempt at serial ELABNET setup");
			pin->flow = flow_second ;
			RETURN_GOOD_IF_GOOD( ELABNET_detect_serial(in) ) ;

			LEVEL_DEBUG("Third attempt at serial ELABNET setup");
			pin->flow = flow_first ;
			RETURN_GOOD_IF_GOOD( ELABNET_detect_serial(in) ) ;

			LEVEL_DEBUG("Fourth attempt at serial ELABNET setup");
			pin->flow = flow_second ;
			RETURN_GOOD_IF_GOOD( ELABNET_detect_serial(in) ) ;
			break ;

		default:
			return gbBAD ;
	}
	return gbBAD ;
}

static GOOD_OR_BAD ELABNET_detect_serial(struct connection_in * in)
{
	int i;
	static char *name[] = { "PBM(0)", "PBM(1)", "PBM(2)"};
	char tmp[MAX_ELABNET_VERSION_LENGTH+1];
	char * search;
	struct port_in * pin = in->pown ;
	unsigned int serial_number = 0;
	unsigned int majorvers = 0, minorvers = 0;
	size_t actual_size;
	/* Set up low-level routines */
	ELABNET_setroutines(in);
	pin->timeout.tv_sec = Globals.timeout_serial ;
	pin->timeout.tv_usec = 0 ;

	/* Open the com port */
	RETURN_BAD_IF_BAD(COM_open(in)) ;
	
	//COM_break( in ) ;
	LEVEL_DEBUG("Slurp in initial bytes");
	ELABNET_slurp( in ) ;
	UT_delay(100) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	ELABNET_slurp( in ) ;

	if ( BAD( ELABNET_version(in, tmp) )) {
		COM_close(in) ;
		LEVEL_DEFAULT("ELABNET detection error");
		return gbBAD;
	}

	search = strchr(tmp, '[');
	if (search) {
	    sscanf(search+1, "%d", &serial_number);
	}

	pin->baud = B230400;
	ELABNET_set_baud(in);

	/* read more info from device */
	if ( BAD( ELABNET_write(ELABNET_string("i"), 1, in) ) ) {
		COM_close(in) ;
		LEVEL_DEFAULT("ELABNET detection error");
		return gbBAD;
	}
	/* read the version string */
	/* 1W PBM (3 Port) [xxxxxxxx]*/
	LEVEL_DEBUG("Checking ELABNET advanced infos");

	actual_size = COM_read_with_timeout(ELABNET_string((tmp)), sizeof(tmp), in);
	if ( actual_size <= 0) {
		LEVEL_DEBUG("No answer from device!!!");
		return gbBAD;
	}
	ELABNET_slurp( in ) ;

	sscanf(tmp, "Version:%d.%d;", &majorvers, &minorvers);

	LEVEL_DEBUG("Adding child ports version");
	in->adapter_name = name[0];
	in->Adapter = adapter_elabnet;
	in->master.elabnet.channel = 0;
	in->master.elabnet.serial_number = serial_number;
	in->master.elabnet.version = (majorvers<<16) | minorvers;

	for (i = 1; i < 3; ++i) {
		LEVEL_DEBUG("Trying to add ELABNET port: %d",i);
		struct connection_in * added = AddtoPort(in->pown);
		LEVEL_DEBUG("Success adding ELABNET port: %d",i);
		if (added == NO_CONNECTION) {
			return gbBAD;
		}
		added->adapter_name = name[i];
		added->master.elabnet.channel = i;
	}

	return gbGOOD;
}

static GOOD_OR_BAD ELABNET_version(struct connection_in * in, char* type_string)
{
	char version_string[MAX_ELABNET_VERSION_LENGTH+1] = {0};

	if ( BAD( ELABNET_write(ELABNET_string(" "), 1, in) ) ) {
		LEVEL_DEFAULT("ELABNET version string cannot be requested");
		return gbBAD ;
	}
		
	/* read the version string */
	/* 1W BM (3 Port) [xxxxxxxx]*/
	LEVEL_DEBUG("Checking ELABNET version");
	if ( BAD(ELABNET_read(ELABNET_string((version_string)), 26, in)) ) {
		LEVEL_DEBUG("No answer from ELABNET!");
		return gbBAD;
	}
	
	if (type_string)
	    strcpy(type_string, version_string);

	return gbGOOD;
}

static GOOD_OR_BAD ELABNET_reconnect( const struct parsedname  *pn )
{
	struct connection_in * in = pn->selected_connection ;
	struct port_in * pin = in->pown ;

	COM_close(in->master.elabnet.head ) ;
	pin->baud = B9600;

	RETURN_BAD_IF_BAD( COM_open(in->master.elabnet.head ) ) ;
	if ( BAD( ELABNET_version(in, NULL) )) {
		COM_close(in) ;
		LEVEL_DEFAULT("ELABNET: detection error");
		return gbBAD;
	}

	pin->baud = B115200;
	ELABNET_set_baud(in);
	return gbGOOD;
}

static void ELABNET_set_baud(struct connection_in * in)
{
	struct port_in * pin = in->pown ;
	char * speed_code ;

	if ( pin->type == ct_telnet ) {
		// telnet pinned at 115200
		return ;
	}
	LEVEL_DEBUG("ELABNET baud set to %d" ,COM_BaudRate(pin->baud));

	COM_BaudRestrict( &(pin->baud), B9600, B19200, B38400, B57600, B115200, B230400 ) ;

	LEVEL_DEBUG("ELABNET baud checked, now %d",COM_BaudRate(pin->baud));
	// Find rate parameter
	switch ( pin->baud ) {
		case B9600:
			COM_break(in) ;
			ELABNET_flush(in);
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
		case B115200:
			speed_code = "=" ;
			break ;
		case B230400:
			speed_code = "$" ;
			break ;
		default:
			LEVEL_DEBUG("ELABNET: Unrecognized baud rate");
			return ;
	}

	LEVEL_DEBUG("ELABNET change baud string <%s>",speed_code);
	ELABNET_flush(in);
	if ( BAD( ELABNET_write(ELABNET_string(speed_code), 1, in) ) ) {
		LEVEL_DEBUG("ELABNET change baud error -- will return to 9600");
		pin->baud = B9600 ;
		++in->changed_bus_settings ;
		return ;
	}


	// Send configuration change
	ELABNET_flush(in);

	// Change OS view of rate
	UT_delay(5);
	COM_change(in) ;
	UT_delay(5);
	ELABNET_slurp(in);

	return ;
}

static void ELABNET_flush( struct connection_in * in )
{
	COM_flush(in->master.elabnet.head) ;
}

static GOOD_OR_BAD ELABNET_SelectChannel(struct connection_in * in)
{
	BYTE buf[2+in->CRLF_size] ;
	char resp[1+in->CRLF_size];

	buf[0] = 'c';			//select port
	buf[1] = '1' + in->channel;
	LEVEL_DEBUG("ELABNET channel: %d",in->channel);
	RETURN_BAD_IF_BAD(ELABNET_write(buf, 2, in) );
	RETURN_BAD_IF_BAD(ELABNET_read(ELABNET_string(resp), 1, in));
	return gbGOOD ;
}

static RESET_TYPE ELABNET_reset(const struct parsedname *pn)
{
	return ELABNET_reset_in(pn->selected_connection);
}

static RESET_TYPE ELABNET_reset_in(struct connection_in * in)
{
	BYTE resp[1+in->CRLF_size];

	if (in->changed_bus_settings > 0) {
		--in->changed_bus_settings ;
		ELABNET_set_baud(in);	// reset paramters
	} else {
		ELABNET_flush(in);
	}

	ELABNET_SelectChannel(in);
	if ( BAD(ELABNET_write(ELABNET_string("r"), 1, in) || BAD( ELABNET_read(resp, 1, in))) ) {
		LEVEL_DEBUG("Error resetting ELABNET device");
		ELABNET_slurp(in);
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
		LEVEL_DEBUG("Unknown ELABNET response %c", resp[0]);
		ELABNET_slurp(in);
		return BUS_RESET_ERROR;
	}
}

static enum search_status ELABNET_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;

	//Special case for DS2409 hub, use low-level code
	if ( pn->ds2409_depth>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(ELABNET_directory(ds, in)) ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("ELABNET slave index %d", ds->index);

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

static void ELABNET_slurp(struct connection_in *in)
{
	COM_slurp(in->master.elabnet.head);
}

static GOOD_OR_BAD ELABNET_read(BYTE * buf, size_t size, struct connection_in *in)
{
	return ELABNET_read_true_length( buf, size+in->CRLF_size, in) ;
}

static GOOD_OR_BAD ELABNET_read_true_length(BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_read( buf, size, in->master.elabnet.head ) ;
}

// Write a string to the serial port
// return 0=good,
//          -EIO = error
//Special processing for the remote hub (add 0x0A)
static GOOD_OR_BAD ELABNET_write(const BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_write( buf, size, in->master.elabnet.head ) ;
}

static GOOD_OR_BAD ELABNET_search_type(struct device_search *ds, struct connection_in * in)
{
	char resp[3+in->CRLF_size];
	int response_length ;

	response_length = 2 ;

	//Depending on the search type, the ELABNET search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching
	
	RETURN_BAD_IF_BAD(ELABNET_SelectChannel(in));

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		RETURN_BAD_IF_BAD(ELABNET_write(ELABNET_string("tEC"), 3, in)) ;
		RETURN_BAD_IF_BAD(ELABNET_read(ELABNET_string(resp), response_length, in)) ;
		if (strstr(resp, "EC") == NULL) {
			LEVEL_DEBUG("ELABNET did not change to conditional search");
			return gbBAD;
		}
		LEVEL_DEBUG("ELABNET set for conditional search");
	} else {
		RETURN_BAD_IF_BAD( ELABNET_write(ELABNET_string("tF0"), 3, in));
		RETURN_BAD_IF_BAD(ELABNET_read(ELABNET_string(resp), response_length, in));
		if (strstr(resp, "F0") == NULL) {
			LEVEL_DEBUG("ELABNET did not change to normal search");
			return gbBAD;
		}
		LEVEL_DEBUG("ELABNET set for normal search");
	}
	return gbGOOD ;
}

/************************************************************************/
/*									*/
/*	ELABNET_directory: searches the Directory stores it in a dirblob	*/
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

static GOOD_OR_BAD ELABNET_directory(struct device_search *ds, struct connection_in * in)
{
	char resp[DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH+in->CRLF_size];

	DirblobClear( &(ds->gulp) );

	// Send the configuration command and check response
	RETURN_BAD_IF_BAD( ELABNET_search_type( ds, in )) ;

	LEVEL_DEBUG("ELABNET channel: %d",in->channel);

	// send the first search
	RETURN_BAD_IF_BAD(ELABNET_write(ELABNET_string("f"), 1, in)) ;

	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the ELABNET
	//device

	RETURN_BAD_IF_BAD(ELABNET_read(ELABNET_string(resp), 1, in)) ;
	
	switch (resp[0]) {
		case 'E':
			LEVEL_DEBUG("ELABNET returned E: No devices in alarm");
			// pass through
		case 'N':
			// remove extra 2 bytes
			LEVEL_DEBUG("ELABNET returned E or N: Empty bus");
			if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
				in->AnyDevices = anydevices_no;
			}
			return gbGOOD ;
		default:
			break ;
	}
	
	if ( BAD(ELABNET_read(ELABNET_string(&resp[1+in->CRLF_size]), DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH-1-in->CRLF_size, in)) ) {
		return gbBAD;
	}

	// Check if we should start scanning
	switch (resp[0]) {
	case '*':
	case '-':
	case '+':
		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			in->AnyDevices = anydevices_yes;
		}
		break;
	default:
		LEVEL_DEBUG("ELABNET_search unrecognized case");
		return gbBAD;
	}

	/* Join the loop after the first query -- subsequent handled differently */
	while ((resp[0] == '+') || (resp[0] == '-') || (resp[0] == '*')) {
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
			if ( BAD(ELABNET_write(ELABNET_string("n"), 1, in))) {
				return gbBAD;
			}
			if ( BAD(ELABNET_read(ELABNET_string((resp)), DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH, in)) ) {
				return gbBAD;
			}
			break;
		case '-':
			return gbGOOD;
		case '*':
			// More devices detected
			return gbGOOD;
		default:
			break;
		}
	}

	return gbGOOD;
}

static void ELABNET_close(struct connection_in *in)
{
	// the standard COM_free routine cleans up the connection
	(void) in ;
}

static GOOD_OR_BAD ELABNET_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	ASCII buf[3] = "pxx";
	BYTE respond[2+in->CRLF_size] ;
	
	num2string(&buf[1], data);

	RETURN_BAD_IF_BAD(ELABNET_SelectChannel(in));
	
	RETURN_BAD_IF_BAD(ELABNET_write(ELABNET_string(buf), 3, in) ) ;

	// flush the buffers
	RETURN_BAD_IF_BAD(ELABNET_write(ELABNET_string("\r"), 1, in) ) ;
	
	RETURN_BAD_IF_BAD( ELABNET_readback_data( ELABNET_string(respond), 2, in) ) ;
	
	resp[0] = string2num((const ASCII *) respond);

	/* Release port mutex to give others a chance to access buses on the same port */
	PORTUNLOCKIN(in);

	/* delay */
	UT_delay(delay);

	/* lock port mutex again, caller expects locked port */
	// now need to relock for further work in transaction
	CHANNELUNLOCKIN(in); // have to release channel, too
	BUSLOCKIN(in);

	return gbGOOD ;
}

//  _sendback_bits
//  Send data and return response block
//  return 0=good
static GOOD_OR_BAD ELABNET_PowerBit(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	BYTE buf[2+1+1+in->CRLF_size] ;
	
	buf[0] = '~';			//put in power bit mode
	buf[1] = data ? '1' : '0' ;
	
	// send to ELABNET (wait for final CR)
	RETURN_BAD_IF_BAD(ELABNET_SelectChannel(in));
	RETURN_BAD_IF_BAD(ELABNET_write(buf, 2, in) ) ;

	/* Release port mutex to give others a chance to access buses on the same port */
	PORTUNLOCKIN(in);

	// delay
	UT_delay(delay);

	/* lock port mutex again, caller expects locked port */
	CHANNELUNLOCKIN(in); // have to release channel, too
	BUSLOCKIN(in);

	// // take out of power bit mode
	RETURN_BAD_IF_BAD(ELABNET_write(ELABNET_string("\r"), 1, in) ) ;
		
	// read back
	RETURN_BAD_IF_BAD( ELABNET_readback_data(buf, 1, in) ) ;
	
	// place data (converted back to hex) in resp
	resp[0] = (buf[0]=='0') ? 0x00 : 0xFF ;
	return gbGOOD;
}


//  _sendback_data
//  Send data and return response block
//  return 0=good
#define ELABNET_SEND_SIZE  32
static GOOD_OR_BAD ELABNET_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t left = size;
	size_t location = 0 ;
	BYTE buf[1+ELABNET_SEND_SIZE*2+1+1+in->CRLF_size] ;
	
	if (size == 0) {
		return gbGOOD;
	}
	
	RETURN_BAD_IF_BAD(ELABNET_SelectChannel(in));

	//Debug_Bytes( "ELABNET sendback send", data, size) ;
	while (left > 0) {
		// Loop through taking only 32 bytes at a time
		size_t this_length = (left > ELABNET_SEND_SIZE) ? ELABNET_SEND_SIZE : left;
		size_t this_length2 = 2 * this_length ; // doubled for switch from hex to ascii
		
		buf[0] = 'b';			//put in byte mode
		bytes2string((char *) &buf[1], &data[location], this_length); // load in data as ascii data
		buf[1+this_length2] = '\r';	// take out of byte mode
		
		// send to ELABNET
		RETURN_BAD_IF_BAD(ELABNET_write(buf, 1+this_length2+1, in) ) ;
		
		// read back
		RETURN_BAD_IF_BAD( ELABNET_readback_data(buf, this_length2, in) ) ;
		
		// place data (converted back to hex) in resp
		string2bytes((char *) buf, &resp[location], this_length);
		left -= this_length;
		location += this_length ;
	}
	//Debug_Bytes( "ELABNET sendback get", resp, size) ;
	return gbGOOD;
}

//  _sendback_bits
//  Send data and return response block
//  return 0=good
static GOOD_OR_BAD ELABNET_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t left = size;
	size_t location = 0 ;
	BYTE buf[1+ELABNET_SEND_SIZE+1+1+in->CRLF_size] ;
	
	if (size == 0) {
		return gbGOOD;
	}

	RETURN_BAD_IF_BAD(ELABNET_SelectChannel(in));

	Debug_Bytes( "ELABNET sendback bits send", databits, size) ;
	while (left > 0) {
		// Loop through taking only 32 bytes at a time
		size_t this_length = (left > ELABNET_SEND_SIZE) ? ELABNET_SEND_SIZE : left;
		size_t i ;
		
		buf[0] = 'j';			//put in bit mode
		for ( i=0 ; i<this_length ; ++i ) {
			buf[i+1] = databits[i+location] ? '1' : '0' ;
		}
		buf[1+this_length] = '\r';	// take out of bit mode
		
		// send to ELABNET
		RETURN_BAD_IF_BAD(ELABNET_write(buf, 1+this_length+1, in) ) ;
		
		// read back
		RETURN_BAD_IF_BAD( ELABNET_readback_data(buf, this_length, in) ) ;
		
		// place data (converted back to hex) in resp
		for ( i=0 ; i<this_length ; ++i ) {
			respbits[i+location] = (buf[i]=='0') ? 0x00 : 0xFF ;
		}
		left -= this_length;
		location += this_length ;
	}
	Debug_Bytes( "ELABNET sendback bits success", respbits, size) ;
	return gbGOOD;
}

static GOOD_OR_BAD ELABNET_readback_data( BYTE * buf, const size_t size, struct connection_in * in)
{
	// read back
	RETURN_BAD_IF_BAD( ELABNET_read(buf, size, in) ) ;
	return gbGOOD;
}

SIZE_OR_ERROR ELABNET_SendCMD(BYTE * tx, const size_t size, BYTE * rx, const size_t rxsize, struct connection_in * in, int tout)
{
	size_t actual_size;
	struct port_in * pin = in->pown ;

	pin->timeout.tv_sec = tout / 1000;
	pin->timeout.tv_usec = 1000 * (tout % 1000);

	/* write string */
	if (size && BAD( ELABNET_write(ELABNET_string(tx), size, in) ) ) {
		LEVEL_DEFAULT("ELABNET: error sending cmd");
		return 0;
	}

	actual_size = COM_read_with_timeout(ELABNET_string((rx)), rxsize, in);
	if ( actual_size <= 0) {
		LEVEL_DEBUG("ELABNET: no answer from device!");
	}
	ELABNET_slurp( in ) ;
	pin->timeout.tv_sec = Globals.timeout_serial ;
	pin->timeout.tv_usec = 0 ;

	return actual_size;
}
