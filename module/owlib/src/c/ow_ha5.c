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

//static void byteprint( const BYTE * b, int size ) ;
GOOD_OR_BAD HA5_detect_parsed(struct address_pair *ap, struct connection_in *in);
static RESET_TYPE HA5_reset(const struct parsedname *pn);
static RESET_TYPE HA5_reset_in(struct connection_in * in) ;
static RESET_TYPE HA5_reset_wrapped(struct connection_in * in) ;
static enum search_status HA5_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD HA5_sendback_part(char cmd, const BYTE * data, BYTE * resp, const size_t size, struct connection_in * in) ;
static GOOD_OR_BAD HA5_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD HA5_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void HA5_setroutines(struct connection_in *in);
static void HA5_close(struct connection_in *in);
static GOOD_OR_BAD HA5_reconnect( const struct parsedname  *pn ) ;
static enum search_status HA5_directory(struct device_search *ds,const struct parsedname *pn);
static GOOD_OR_BAD HA5_select( const struct parsedname * pn ) ;
static GOOD_OR_BAD HA5_select_wrapped( const struct parsedname * pn ) ;
static GOOD_OR_BAD HA5_resync( struct connection_in * in ) ;
static GOOD_OR_BAD HA5_channel_list( char * alpha_string, struct connection_in * initial_in ) ;
static GOOD_OR_BAD HA5_test_channel( struct connection_in * in ) ;
static GOOD_OR_BAD TestChecksum( unsigned char * check_string, int length ) ;
static GOOD_OR_BAD HA5_checksum_support( struct connection_in * in ) ;
static GOOD_OR_BAD HA5_find_channel( struct connection_in * in ) ;
static void HA5_slurp( struct connection_in *in) ;
static void HA5_flush( struct connection_in *in) ;
static GOOD_OR_BAD HA5_read( BYTE * data, size_t length, struct connection_in *in) ;
static GOOD_OR_BAD HA5_write( char command, char * raw_string, int length, struct connection_in * in ) ;
static GOOD_OR_BAD HA5_sendback_bits(const BYTE * outbits, BYTE * inbits, const size_t length, const struct parsedname *pn) ;
static GOOD_OR_BAD HA5_bit( BYTE send, BYTE * receive, struct connection_in * in ) ;

#define CR_char		(0x0D)

static void HA5_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA5_detect;
	in->iroutines.reset = HA5_reset;
	in->iroutines.next_both = HA5_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = HA5_sendback_data;
	in->iroutines.sendback_bits = HA5_sendback_bits;
	in->iroutines.select = HA5_select ;
	in->iroutines.select_and_sendback = HA5_select_and_sendback;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = HA5_reconnect;
	in->iroutines.close = HA5_close;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay | ADAP_FLAG_presence_from_dirblob ;
	in->bundling_length = HA5_FIFO_SIZE;
}

GOOD_OR_BAD HA5_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct address_pair ap ;
	GOOD_OR_BAD gbResult ; // default

	/* Set up low-level routines */
	HA5_setroutines(in);

	/* By definition, this is the head adapter on this port */
	in->master.ha5.head = in ;

	// Poison current "Address" for adapter
	memset( in->remembered_sn, 0x00, SERIAL_NUMBER_SIZE ) ;

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("HA5 bus master requires port name");
		return gbBAD;
	}

	COM_set_standard( in ) ; // standard COM port settings

	// 9600 isn't valid for the HA5, so we can tell that this value was actually selected
	if ( Globals.baud != B9600 ) {
		pin->baud = Globals.baud ;
	} else {
		pin->baud = B115200 ; // works at this fast rate
	}

	// allowable speeds
	COM_BaudRestrict( &(pin->baud), B1200, B19200, B38400, B115200, 0 ) ;

	Parse_Address( DEVICENAME(in), &ap ) ;

	gbResult = HA5_detect_parsed( &ap, in) ;

	Free_Address( &ap ) ;

	if ( GOOD( gbResult ) ) {
		HA5_slurp(in) ;
		HA5_reset_in(in) ;
	}
	return gbResult ;
}


// Detect now that the name has been parsed
GOOD_OR_BAD HA5_detect_parsed(struct address_pair *ap, struct connection_in *in)
{
	char * channel_list = NULL ;

	// Create name (ip:port or /dev/tty) and optional channel list
	switch (in->pown->type) {
		case ct_telnet:
			// address:port:channel_list
			if ( ap->first.type != address_none ) {
				// IP address exists
				strcpy(DEVICENAME(in), ap->first.alpha) ; // subset so will fit
				if ( ap->second.type != address_none ) {
					// Add :port
					strcat(DEVICENAME(in), ":") ; // subset so will fit
					strcat(DEVICENAME(in), ap->second.alpha) ; // subset so will fit
				}
			} else if ( ap->second.type != address_none ) {
				// just :port
				strcpy(DEVICENAME(in), ap->second.alpha) ; // subset so will fit
			} else {
				// No tcp address or port specified
				return gbBAD ;
			}

			switch ( ap->third.type ) {
				case address_all:
					channel_list = "abcdefghijklmnopqrstuvwxyz" ;
					break ;
				case address_alpha:		
					// Channel specified
					channel_list = ap->third.alpha ;
					break ;
				default:
					channel_list = NULL ;
			}
			break ;

		case ct_serial:
		default:
			// device:channel_list
			if ( ap->first.type != address_none ) {
				// device address exists
				strcpy(DEVICENAME(in), ap->first.alpha) ; // subset so will fit
			} else {
				// No serial port specified
				return gbBAD ;
			}

			switch ( ap->second.type ) {
				case address_all:
					channel_list = "abcdefghijklmnopqrstuvwxyz" ;
					break ;
				case address_alpha:		
					// Channel specified
					channel_list = ap->second.alpha ;
					break ;
				default:
					channel_list = NULL ;
			}
			break ;
	}

	if ( BAD(COM_open(in->master.ha5.head )) ) {
		// Cannot open serial port
		return gbBAD ;
	}

	in->master.ha5.checksum = 1 ;
	in->Adapter = adapter_HA5 ;
	in->adapter_name = "HA5";

	/* Find the channels */
	if ( channel_list == NULL ) { // scan for channel
		if ( BAD( HA5_find_channel(in) ) ) {
			RETURN_BAD_IF_BAD( serial_powercycle(in) ) ;
			return HA5_find_channel(in) ;
		}
	} else { // A list of channels
		if ( BAD( HA5_channel_list( channel_list, in ) ) ) {
			RETURN_BAD_IF_BAD( serial_powercycle(in) ) ;
			return HA5_channel_list( channel_list, in ) ;
		}
	}
	return gbGOOD ;
}

static GOOD_OR_BAD HA5_channel_list( char * alpha_string, struct connection_in * initial_in )
{
	char * current_char ;
	int first_time = 1 ; // work-around for new USB com port
	struct connection_in * current_in = initial_in ;

	for ( current_char = alpha_string ; current_char[0]!= '\0' ; ++ current_char ) {
		char c = current_char[0];
		if ( !isalpha(c) ) {
			LEVEL_DEBUG("Urecognized HA5 channel <%c>",c) ;
			continue ;
		}

		current_in->master.ha5.channel = tolower(c) ;
		LEVEL_DEBUG("Looking for HA5 adapter on %s:%c", DEVICENAME(current_in), current_in->master.ha5.channel ) ;

		if (
		GOOD( HA5_test_channel(current_in))
		|| ( first_time && GOOD( HA5_test_channel(current_in)) )
		) {

			// work around for new USB serial port that seems to need a repeat test
			first_time = 0 ;

			LEVEL_CONNECT("HA5 bus master FOUND on port %s at channel %c", DEVICENAME(current_in), current_in->master.ha5.channel ) ;
			current_in = AddtoPort( initial_in->pown ) ; // point to newer space for next candidate

			if ( current_in == NO_CONNECTION ) {
				break ;
			}
			continue ;
		}

		LEVEL_CONNECT("HA5 bus master NOT FOUND on port %s at channel %c", DEVICENAME(current_in), c ) ;
	}
	
	// Now see if we've added any adapters
	if ( current_in == initial_in ) {
		LEVEL_CONNECT("No HA5 adapters found in channel(s): %s",SAFESTRING(alpha_string) );
		return gbBAD ;
	}
	
	if ( current_in != NO_CONNECTION ) {
		// Remove last addition, it is blank
		RemoveIn( current_in ) ;
	}
	
	return gbGOOD ;
}

/* Find the HA5 channel (since none specified) */
/* Arbitrarily assign it to "a" if none found */
static GOOD_OR_BAD HA5_find_channel( struct connection_in * in )
{
	char test_chars[] = "aabcdefghijklmnopqrstuvwxyz" ; // note 'a' is repeated
	char * c ;

	for ( c = test_chars ; c[0] != '\0' ; ++c ) {
		// test char set
		in->master.ha5.channel = c[0] ;

		LEVEL_DEBUG("Looking for HA5 adapter on %s:%c", DEVICENAME(in), in->master.ha5.channel ) ;
		if ( GOOD( HA5_test_channel(in)) ) {
			LEVEL_CONNECT("HA5 bus master FOUND on port %s at channel %c", DEVICENAME(in), in->master.ha5.channel ) ;
			return gbGOOD ;
		}
	}
	LEVEL_DEBUG("HA5 bus master NOT FOUND on port %s", DEVICENAME(in) ) ;
	in->master.ha5.channel = '\0' ; 
	return gbBAD;
}

static GOOD_OR_BAD HA5_write( char command, char * raw_string, int length, struct connection_in * in )
{
	int i ;
	unsigned int sum = 0 ;
	BYTE mod_buffer[length+5] ;

	// add the channel
	mod_buffer[0] = in->master.ha5.channel ;

	// Add the HA5 command
	mod_buffer[1] = command ;

	// Copy the string
	if ( length > 0 ) {
		memcpy( &mod_buffer[2], raw_string, length ) ;
	}

	// Compute a simple checksum
	for ( i=0 ; i<length+2 ; ++i ) {
		sum += mod_buffer[i] ;
	}

	// Add the checksum
	num2string( (char *) &mod_buffer[length+2], sum & 0xFF) ;

	// Add the <CR>
	mod_buffer[length+4] = CR_char ;

	// Write full string to the HA5
	if ( BAD(COM_write( mod_buffer, length+5, in->master.ha5.head )) ) {
		LEVEL_DEBUG("Error with sending HA5 block") ;
		return gbBAD ;
	}
	return gbGOOD ;
}

static GOOD_OR_BAD HA5_read( BYTE * data, size_t length, struct connection_in * in)
{
	return COM_read( data, length, in->master.ha5.head  ) ;
}

static GOOD_OR_BAD TestChecksum( unsigned char * check_string, int length )
{
	int i ;
	unsigned char sum = 0 ;
	for ( i=0 ; i<length ; ++i ) {
		sum += check_string[i] ;
	}
	if ( string2num( (char *) & check_string[length]) == (sum & 0xFF) ) {
		return gbGOOD ;
	}
	return gbBAD ;
}

static GOOD_OR_BAD HA5_reconnect( const struct parsedname  *pn )
{
	struct connection_in * in = pn->selected_connection ;
	RETURN_BAD_IF_BAD(serial_powercycle(in->master.ha5.head)) ;
	return HA5_test_channel( in ) ;
}

// test char already set
static GOOD_OR_BAD HA5_test_channel( struct connection_in * in )
{
	HA5_slurp(in) ;

	RETURN_BAD_IF_BAD( HA5_checksum_support(in) ) ;

	if ( HA5_reset_wrapped( in ) == BUS_RESET_OK ) {
		return gbGOOD ;
	}

	LEVEL_DEBUG("Succeed despite bad reset");
	return gbGOOD ;
}

static void HA5_slurp( struct connection_in * in )
{
	COM_slurp( in->master.ha5.head  ) ;
} 

static void HA5_flush( struct connection_in * in )
{
	COM_flush( in->master.ha5.head  ) ;
} 

// test char already set
static GOOD_OR_BAD HA5_checksum_support( struct connection_in * in )
{
	BYTE resp[3] = { 'X', 'X', 'X', } ;

	// write a BYTE and see if any response (device present)
	// and then see if a checksum is included in the response

	if ( BAD(HA5_write( 'W', "01FF", 4, in)) ) {
		LEVEL_DEBUG("Error sending Bit");
		return gbBAD;
	}
	if ( BAD(HA5_read(resp, 3, in)) ) {
		LEVEL_DEBUG("Error reading bit response");
		return gbBAD;
	}

	if ( resp[2] == 0x0D ) {
		in->master.ha5.checksum = 0 ;
		LEVEL_DETAIL("HA5 %s in NON-CHECKSUM mode",SAFESTRING(DEVICENAME(in)));
		return gbGOOD ;
	} else if ( resp[2] == 'X' ) {
		// nothing actually read
		return gbBAD ;
	}
	resp[0] = 'X' ;
	resp[1] = 'X' ;
	if ( BAD(HA5_read(resp, 2, in)) ) {
		LEVEL_DEBUG("Error reading Bit checksum");
		return gbBAD;
	}
	if ( resp[1] != 0x0D ) {
		LEVEL_DEBUG("Bad response");
		return gbBAD ;
	}
	in->master.ha5.checksum = 1 ;
	LEVEL_DETAIL("HA5 %s in CHECKSUM mode",SAFESTRING(DEVICENAME(in)));
	return gbGOOD ;
}

static RESET_TYPE HA5_reset(const struct parsedname *pn)
{
	return HA5_reset_in( pn->selected_connection ) ;
}

static RESET_TYPE HA5_reset_in( struct connection_in * in )
{
	return HA5_reset_wrapped(in) ;
}

static RESET_TYPE HA5_reset_wrapped( struct connection_in * in )
{
	BYTE resp[2] = { 'X', 'X', } ;

	if ( BAD(HA5_write( 'R', "", 0, in)) ) {
		LEVEL_DEBUG("Error sending HA5 reset");
		return BUS_RESET_ERROR;
	}

	// For some reason, the HA5 doesn't use a checksum for RESET response.
	if ( BAD(HA5_read(resp, 2, in)) ) {
		LEVEL_DEBUG("Error reading HA5 reset");
		return BUS_RESET_ERROR;
	}

	switch( resp[0] ) {
		case 'P':
			in->AnyDevices = anydevices_yes ;
			return BUS_RESET_OK;
		case 'N':
			in->AnyDevices = anydevices_no ;
			return BUS_RESET_OK;
		default:
			LEVEL_DEBUG("Error HA5 reset bad response %c (0x%.2X)", resp[0], resp[0]);
			return BUS_RESET_ERROR;
	}
}

static enum search_status HA5_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;

	if (ds->LastDevice) {
		return search_done;
	}

	HA5_flush(in);

	if (ds->index == -1) {
		if ( HA5_directory(ds, pn) != search_good ) {
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

/************************************************************************/
/*	HA5_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/************************************************************************/
static enum search_status HA5_directory(struct device_search *ds, const struct parsedname *pn)
{
	unsigned char resp[20];
	struct connection_in * in = pn->selected_connection ;

	DirblobClear( &(ds->gulp) );

	//Depending on the search type, the HA5 search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if ( BAD(HA5_write( (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ? 'C' : 'S', ",FF", 3, in)) ) {
		HA5_resync(in) ;
		return search_error ;
	}

	if ( BAD(HA5_read(resp, 1, in)) ) {
		HA5_resync(in) ;
		return search_error ;
	}

	while ( resp[0] != CR_char ) {
		BYTE sn[SERIAL_NUMBER_SIZE];
		char wrap_char ;
		//One needs to check the first character returned.
		//If nothing is found, the ha5 will timeout rather then have a quick
		//return.  This happens when looking at the alarm directory and
		//there are no alarms pending
		//So we grab the first character and check it.  If not an E leave it
		//in the resp buffer and get the rest of the response from the HA5
		//device

		if ( in->master.ha5.checksum ) {
			if ( BAD(HA5_read(&resp[1], 19, in)) ) {
				HA5_resync(in) ;
				return search_error ;
			}
			if ( resp[18]!=CR_char ) {
				HA5_resync(in) ;
				return search_error ;
			}
			wrap_char = resp[19] ;
			if ( BAD( TestChecksum( resp, 16 )) ) {
				HA5_resync(in) ;
				return search_error ;
			}
		} else {
			if ( BAD(HA5_read(&resp[1], 17, in)) ) {
				HA5_resync(in) ;
				return search_error ;
			}
			if ( resp[16]!=CR_char ) {
				HA5_resync(in) ;
				return search_error ;
			}
			wrap_char = resp[17] ;
		}
		sn[7] = string2num((char *)&resp[0]);
		sn[6] = string2num((char *)&resp[2]);
		sn[5] = string2num((char *)&resp[4]);
		sn[4] = string2num((char *)&resp[6]);
		sn[3] = string2num((char *)&resp[8]);
		sn[2] = string2num((char *)&resp[10]);
		sn[1] = string2num((char *)&resp[12]);
		sn[0] = string2num((char *)&resp[14]);

		// Set as current "Address" for adapter
		memcpy( in->remembered_sn, sn, SERIAL_NUMBER_SIZE ) ;

		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));
		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s", sn);
			HA5_resync(in) ;
			return search_error ;
		}
		DirblobAdd(sn, &(ds->gulp) );
		resp[0] = wrap_char ;
	}
	return search_good ;
}

static GOOD_OR_BAD HA5_resync( struct connection_in * in )
{
	HA5_flush(in->master.ha5.head );
	HA5_reset_wrapped(in);
	HA5_flush(in->master.ha5.head );

	// Poison current "Address" for adapter
	memset( in->remembered_sn, 0, SERIAL_NUMBER_SIZE ) ; // so won't match

	return gbBAD ;
}

static GOOD_OR_BAD HA5_select( const struct parsedname * pn )
{
	if ( (pn->selected_device==NO_DEVICE) || (pn->selected_device==DeviceThermostat) ) {
		RETURN_BAD_IF_BAD( gbRESET( HA5_reset_in(pn->selected_connection) ) ) ;
	}

	return HA5_select_wrapped(pn) ;
}

static GOOD_OR_BAD HA5_select_wrapped( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	char send_address[16] ;
	unsigned char resp_address[19] ;

	num2string( &send_address[ 0], pn->sn[7] ) ;
	num2string( &send_address[ 2], pn->sn[6] ) ;
	num2string( &send_address[ 4], pn->sn[5] ) ;
	num2string( &send_address[ 6], pn->sn[4] ) ;
	num2string( &send_address[ 8], pn->sn[3] ) ;
	num2string( &send_address[10], pn->sn[2] ) ;
	num2string( &send_address[12], pn->sn[1] ) ;
	num2string( &send_address[14], pn->sn[0] ) ;

	if ( BAD(HA5_write( 'A', send_address, 16, in)) ) {
		LEVEL_DEBUG("Error sending HA5 A-ddress") ;
		return HA5_resync(in) ;
	}

	if ( in->master.ha5.checksum ) {
		if ( BAD(HA5_read(resp_address,19,in)) ) {
			LEVEL_DEBUG("Error with reading HA5 select") ;
			return HA5_resync(in) ;
		}
		if ( BAD(TestChecksum( resp_address, 16)) ) {
			LEVEL_DEBUG("HA5 select checksum error") ;
			return HA5_resync(in) ;
		}
	} else {
		if ( BAD(HA5_read(resp_address,17,in)) ) {
			LEVEL_DEBUG("Error with reading HA5 select") ;
			return HA5_resync(in) ;
		}
	}
	if ( memcmp( resp_address, send_address, 16) ) {
		LEVEL_DEBUG("Error with HA5 select response") ;
		return HA5_resync(in) ;
	}

	// Set as current "Address" for adapter
	memcpy( in->remembered_sn, pn->sn, SERIAL_NUMBER_SIZE) ;

	return gbGOOD ;
}

//  Send data and return response block -- up to 32 bytes
static GOOD_OR_BAD HA5_sendback_part(char cmd, const BYTE * data, BYTE * resp, const size_t size, struct connection_in * in)
{
	char send_data[2+2*size] ;
	unsigned char get_data[32*2+3] ;

	num2string(   &send_data[0], size ) ;
	bytes2string( &send_data[2], data, size) ;

	if ( BAD(HA5_write( cmd, send_data, 2+2*size, in)) ) {
		LEVEL_DEBUG("Error with sending HA5 block") ;
		HA5_resync(in) ;
		return gbBAD ;
	}

	if ( in->master.ha5.checksum ) {
		if ( BAD(HA5_read( get_data, size*2+3, in)) ) {
			LEVEL_DEBUG("Error with reading HA5 block") ;
			HA5_resync(in) ;
			return gbBAD ;
		}
		if ( BAD(TestChecksum( get_data, size*2)) ) {
			LEVEL_DEBUG("HA5 block read checksum error") ;
			HA5_resync(in) ;
			return gbBAD ;
		}
	} else {
		if ( BAD(HA5_read( get_data, size*2+1, in)) ) {
			LEVEL_DEBUG("Error with reading HA5 block") ;
			HA5_resync(in) ;
			return gbBAD ;
		}
	}
	string2bytes( (char *)get_data, resp, size) ;
	return gbGOOD ;
}

static GOOD_OR_BAD HA5_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;
	struct connection_in * in = pn->selected_connection ;

	for ( left=size ; left>0 ; left -= 32 ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>32)?32:left ;

		RETURN_BAD_IF_BAD( HA5_sendback_part( 'W', &data[pass_start], &resp[pass_start], pass_size, in ) ) ;
	}
	return gbGOOD;
}

static GOOD_OR_BAD HA5_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int left;
	char block_cmd ;

	if ( memcmp( pn->sn, in->remembered_sn, SERIAL_NUMBER_SIZE ) ) {
		// Need a formal change of device
		RETURN_BAD_IF_BAD( HA5_select(pn) ) ;
		block_cmd = 'W' ;
	} else {
		// Same device
		block_cmd = 'J' ;
	}

	for ( left=size ; left>0 ; left -= 32 ) {
		GOOD_OR_BAD ret ;
		size_t pass_start = size - left ;
		size_t pass_size = (left>32)?32:left ;

		ret = HA5_sendback_part( block_cmd, &data[pass_start], &resp[pass_start], pass_size, in ) ;
		block_cmd = 'W' ; // for next pass
		RETURN_BAD_IF_BAD( ret ) ;
	}
	return gbGOOD;
}

/********************************************************/
/* HA5_sendback_bits -- bit-mode communication      	*/
/********************************************************/

static GOOD_OR_BAD HA5_sendback_bits(const BYTE * outbits, BYTE * inbits, const size_t length, const struct parsedname *pn)
{
	size_t counter ;
	struct connection_in * in = pn->selected_connection ;

	for ( counter = 0 ; counter < length ; ++counter ) {
		RETURN_BAD_IF_BAD( HA5_bit( outbits[counter], &inbits[counter], in ) ) ;
	} 

	return gbGOOD;
}

static GOOD_OR_BAD HA5_bit( BYTE outbit, BYTE * inbit, struct connection_in * in )
{
	BYTE resp[1] = { 'X', } ;

	if ( BAD(HA5_write( 'B', outbit ? "1" : "0" , 1, in)) ) {
		LEVEL_DEBUG("Error sending HA5 bit");
		return gbBAD;
	}

	// For some reason, the HA5 doesn't use a checksum for BIT response.
	if ( BAD(HA5_read(resp, 2, in)) ) {
		LEVEL_DEBUG("Error reading HA5 bit");
		return gbBAD;
	}

	switch ( resp[0] ) {
		case '0':
			inbit[0] = 0 ;
			return gbGOOD ;
		case '1':
			inbit[0] = 1 ;
			return gbGOOD ;
			default:
			LEVEL_DEBUG("Unclear HA5 bit response \'%c\'",resp[0]);
			return gbBAD ;
	}
}	

/********************************************************/
/* HA5_close -- clean local resources before      	*/
/*                closing the serial port           	*/
/********************************************************/

static void HA5_close(struct connection_in *in)
{
	if ( in->master.ha5.head == in ) {
	} else {
		in->pown->state = cs_virgin ;
	}
}

