/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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

/* Multimaster:
 * In general the link is NOT a multimaster design (only one bus master per serial or telnet port)
 * The LinkHubE is supposed to have separately addressable lines, but this is not implemented
 * None the less, all settings are assigned to the head line
 */
 
/* lsusb output for LinkUSB
	Bus 001 Device 024: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC
	Device Descriptor:
	  bLength                18
	  bDescriptorType         1
	  bcdUSB               2.00
	  bDeviceClass            0 (Defined at Interface level)
	  bDeviceSubClass         0 
	  bDeviceProtocol         0 
	  bMaxPacketSize0         8
	  idVendor           0x0403 Future Technology Devices International, Ltd
	  idProduct          0x6001 FT232 USB-Serial (UART) IC
	  bcdDevice            6.00
	  iManufacturer           1 FTDI
	  iProduct                2 FT232R USB UART
	  iSerial                 3 A900a3Z7
	  bNumConfigurations      1
	  Configuration Descriptor:
		bLength                 9
		bDescriptorType         2
		wTotalLength           32
		bNumInterfaces          1
		bConfigurationValue     1
		iConfiguration          0 
		bmAttributes         0xa0
		  (Bus Powered)
		  Remote Wakeup
		MaxPower               90mA
		Interface Descriptor:
		  bLength                 9
		  bDescriptorType         4
		  bInterfaceNumber        0
		  bAlternateSetting       0
		  bNumEndpoints           2
		  bInterfaceClass       255 Vendor Specific Class
		  bInterfaceSubClass    255 Vendor Specific Subclass
		  bInterfaceProtocol    255 Vendor Specific Protocol
		  iInterface              2 FT232R USB UART
		  Endpoint Descriptor:
			bLength                 7
			bDescriptorType         5
			bEndpointAddress     0x81  EP 1 IN
			bmAttributes            2
			  Transfer Type            Bulk
			  Synch Type               None
			  Usage Type               Data
			wMaxPacketSize     0x0040  1x 64 bytes
			bInterval               0
		  Endpoint Descriptor:
			bLength                 7
			bDescriptorType         5
			bEndpointAddress     0x02  EP 2 OUT
			bmAttributes            2
			  Transfer Type            Bulk
			  Synch Type               None
			  Usage Type               Data
			wMaxPacketSize     0x0040  1x 64 bytes
			bInterval               0
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
	{"1.5", "LinkUSB V1.5", adapter_LINK_15},
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

static void LINK_setroutines(struct connection_in *in);
static void LINKE_setroutines(struct connection_in *in);

static RESET_TYPE LINK_reset_in(struct connection_in * in);
static GOOD_OR_BAD LINK_detect_serial(struct connection_in * in) ;
static GOOD_OR_BAD LINK_detect_serial0(struct connection_in * in, int timeout_secs) ;
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
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = LINK_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
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
	for ( version_pointer = reported_string ; (*version_pointer) != '\0' ; ++version_pointer )  {
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
GOOD_OR_BAD LINK_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	if (pin->init_data == NULL) {
		// requires input string
		LEVEL_DEFAULT("LINK busmaster requires port name");
		return gbBAD;
	}

	COM_set_standard( in ) ; // standard COM port settings

	switch( pin->type ) {
		case ct_telnet:
			// LinkHub-E
			pin->baud = B115200 ;
			LEVEL_DEBUG("Attempt connection to networked LINK at 115200 baud");
			RETURN_GOOD_IF_GOOD(  LINK_detect_net( in )  );

			// Xport or ser2net
			pin->baud = B9600 ;
			LEVEL_DEBUG("Attempt connection to networked LINK at 9600 baud");
			RETURN_GOOD_IF_GOOD(  LINK_detect_net( in )  );

			break ;

			

		case ct_serial:
		case ct_ftdi:
		{
			/* The LINK (both regular and USB) can run in different baud modes.
			 * On power reset it will always use 9600bps, but it can be configured for other
			 * baud rates by sending different commands.
			 * For ct_serial links we use 9600bps, but for the improved ct_ftdi we
			 * go ahead and improve the speed as well, trying to keep it at 19200bps.
			 *
			 * The Link DOES support higher baud rates (38400, 56700), BUT then we have
			 * to enforce output throttling in order to not overflow the 1-Wire bus...
			 * So, keep it at 19200.
			 */
			RETURN_BAD_IF_BAD(LINK_detect_serial(in));

			if(pin->type == ct_serial) {
				return gbGOOD;
			} else if(pin->type == ct_ftdi) {
				// ct_serial handling of BREAK, which is required to re-set to 9600bps, is not
				// reliable. Only enable for ftdi devices with proper BREAK support.
				LEVEL_DEBUG("Reconfiguring found LinkUSB to 19200bps");
				pin->baud = B19200;
				LINK_set_baud(in);

				// ensure still found
				RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;
				ERROR_CONNECT("LINK baud reconfigure failed, cannot find it after 19200 change.");
			}
			break;
		}

		default:
			return gbBAD ;
	}
	return gbBAD ;
}
/* Detect serial device by probing with different baud-rates. Calls LINK_detect_serial0 */
static GOOD_OR_BAD LINK_detect_serial(struct connection_in * in) {
	/* LINK docs does not say anything about flow control. Studying the LinkOEM
	 * datasheet however, indicates that we only have Tx and Rx; no RTS/CTS.
	 * And it does not mention XON/XOFF..
	 * And it works fine with no flow control... So, let's go with no flow control.
	 *
	 * For startup detection, we first try with 9600, then 19200. We do however
	 * force a shorter timeout, the standard 5s is pretty overkill..
	 */
	int i;
	struct port_in * pin = in->pown ;
	speed_t bauds[] = {B9600, B19200, B38400
#ifdef B57600
, B57600
#endif
		, 0};
#define SHORT_TIMEOUT 1

	/* A break should put the Link in 9600...
	 * However, the LinkUSB v1.5 fails to do this, unless we are in byte mode.
	 * If we are, it reacts to break, and changes to 9600..
	 * But, in plain command mode, break does not work.
	 *
	 * iButtonLink said they would investige this bug(?), but never replied.
	 */
	COM_break( in ) ;

	i=0;
	// First try quickly in all baudrates with no flow control
	while((pin->baud = bauds[i++]) != 0) {
		pin->flow = flow_none;
		LEVEL_DEBUG("Detecting %s LINK using %d bps",
				(pin->type == ct_serial ? "serial" : "ftdi"),
				COM_BaudRate(pin->baud));
		RETURN_GOOD_IF_GOOD( LINK_detect_serial0(in, SHORT_TIMEOUT) ) ;
	}

	i=0;
	// No? Try different flow controls then (the way it was done in the old code..)
	while((pin->baud = bauds[i++]) != 0) {
		LEVEL_DEBUG("Second attempt at serial LINK setup, %d bps", COM_BaudRate(pin->baud));
		pin->flow = flow_first;
		RETURN_GOOD_IF_GOOD( LINK_detect_serial0(in, Globals.timeout_serial) ) ;

		LEVEL_DEBUG("Third attempt at serial LINK setup");
		pin->flow = flow_second ;
		RETURN_GOOD_IF_GOOD( LINK_detect_serial0(in, Globals.timeout_serial) ) ;
	}

	return gbBAD;
}

/* Detect serial device with a specific configured baud-rate */
static GOOD_OR_BAD LINK_detect_serial0(struct connection_in * in, int timeout_secs)
{
	struct port_in * pin = in->pown ;
	/* Set up low-level routines */
	LINK_setroutines(in);
	pin->timeout.tv_sec = timeout_secs ;
	pin->timeout.tv_usec = 0 ;

	/* Open the com port */
	RETURN_BAD_IF_BAD(COM_open(in)) ;

	COM_break( in ) ; // BREAK also sends it into command mode, if in another mode
	LEVEL_DEBUG("Slurp in initial bytes");
	LINK_slurp( in ) ;
	UT_delay(100) ; // based on https://web.archive.org/web/20080624060114/http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89
	LINK_slurp( in ) ;
	
	RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;
	LEVEL_DEFAULT("LINK detection error, trying powercycle");
	
	serial_powercycle(in) ;
	LEVEL_DEBUG("Slurp in initial bytes");
	LINK_slurp( in ) ;
	UT_delay(100) ; // based on https://web.archive.org/web/20080624060114/http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89
	LINK_slurp( in ) ;
	
	RETURN_GOOD_IF_GOOD( LINK_version(in) ) ;
	LEVEL_DEFAULT("LINK detection error, giving up");
	COM_close(in) ;
	return gbBAD;
}

static GOOD_OR_BAD LINK_detect_net(struct connection_in * in)
{
	struct port_in * pin = in->pown ;
	/* Set up low-level routines */
	LINKE_setroutines(in);
	pin->timeout.tv_sec = 0 ;
	pin->timeout.tv_usec = 300000 ;

	/* Open the tcp port */
	RETURN_BAD_IF_BAD( COM_open(in) ) ;

	LEVEL_DEBUG("Slurp in initial bytes");
//	LINK_slurp( in ) ;
	UT_delay(1000) ; // based on http://morpheus.wcf.net/phpbb2/viewtopic.php?t=89&sid=3ab680415917a0ebb1ef020bdc6903ad
	LINK_slurp( in ) ;
//	LINK_flush(in);

	pin->dev.telnet.telnet_negotiated = needs_negotiation ;
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
				break ;
			default:
				switch ( lvs ) {
					case lvs_string:
						// add to string
						break ; 
					case lvs_0d:
						++in->CRLF_size;
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
	struct port_in * pin = in->pown ;
	int send_break = 0;
	char * speed_code ;

	if ( pin->type == ct_telnet ) {
		// telnet pinned at 115200
		return ;
	}

	COM_BaudRestrict( &(pin->baud), B9600, B19200, B38400, B57600, 0 ) ;

	LEVEL_DEBUG("LINK set baud to %d",COM_BaudRate(pin->baud));
	// Find rate parameter
	switch ( pin->baud ) {
		case B9600:
			// Put in Byte mode, and then send break.
			speed_code = "b";
			send_break = 1;
			break;
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
		pin->baud = B9600 ;
		++in->changed_bus_settings ;
		return ;
	}

	if(send_break)  {
		COM_break(in);
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
	BYTE resp[1+in->CRLF_size];

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
	if ( pn->ds2409_depth>0 ) {
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
			LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
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
	return LINK_read_true_length( buf, size+in->CRLF_size, in ) ;
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
	char resp[3+in->CRLF_size];
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
	char resp[DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH+in->CRLF_size];

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
			__attribute__ ((fallthrough));
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
	
	if ( BAD(LINK_read(LINK_string(&resp[1+in->CRLF_size]), DEVICE_LENGTH+COMMA_LENGTH+PLUS_LENGTH-1-in->CRLF_size, in)) ) {
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

/* Control The Link AUX port, an extra physical line which
 * can be set high, low or HiZ by means of reading line state */
GOOD_OR_BAD LINK_aux_write(int level, struct connection_in * in) {
	BYTE out[1];
	if(level) {
		// Set the Auxiliary line to the HIGH (default) level and low impedance.
		out[0] = 'd';
	}
	else
	{
		// Set the Auxiliary line to the LOW level and low impedance.
		out[0] = 'z';
	}
	RETURN_BAD_IF_BAD(LINK_write(out, 1, in)) ;
	// These commands does not generate any response

	return gbGOOD;
}

GOOD_OR_BAD LINK_aux_read(int *level_out, struct connection_in * in) {
	BYTE buf[1+in->CRLF_size] ;

	// Set the Auxiliary line to high impedance and report the current intended input level.
	RETURN_BAD_IF_BAD(LINK_write(LINK_string("&"), 1, in)) ;
	RETURN_BAD_IF_BAD(LINK_readback_data(buf, 1, in)) ;

	*level_out = (buf[0]=='0') ? 0 : 1 ;
	return gbGOOD;
}


static void LINK_close(struct connection_in *in) {
	struct port_in * pin = in->pown ;
	if ( pin->state == cs_virgin ) {
		LEVEL_DEBUG("LINK_close called on already closed connection");
		return ;
	}

	if(pin->baud != B9600) {
		// Ensure we reset it to 9600bps to speed up detection on next startup
		LEVEL_DEBUG("Reconfiguring adapter to 9600bps before closing");
		pin->baud = B9600;
		LINK_set_baud(in);
	}

	// the standard COM_free routine cleans up the connection
}

static GOOD_OR_BAD LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	ASCII buf[3] = "pxx";
	BYTE respond[2+in->CRLF_size] ;
	
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
	BYTE buf[2+1+1+in->CRLF_size] ;
	
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
	BYTE buf[1+LINK_SEND_SIZE*2+1+1+in->CRLF_size] ;
	
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
	BYTE buf[1+LINK_SEND_SIZE+1+1+in->CRLF_size] ;
	
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
