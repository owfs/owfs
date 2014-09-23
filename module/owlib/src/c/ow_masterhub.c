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

/* Hobby Boards Master Hub v2.3 
 * designed by Eric Vickery
 * Support by Paul Alfille 2014
 * 
 * dmesg output:
 * usb 1-1.1: new full-speed USB device number 9 using ehci-pci
 * usb 1-1.1: New USB device found, idVendor=04d8, idProduct=f897
 * usb 1-1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
 * usb 1-1.1: Product: Hobby Boards USB Master
 * usb 1-1.1: Manufacturer: Hobby Boards
 * cdc_acm 1-1.1:1.0: This device cannot do calls on its own. It is not a modem.
 * cdc_acm 1-1.1:1.0: ttyACM0: USB ACM device
 * usbcore: registered new interface driver cdc_acm
 * cdc_acm: USB Abstract Control Model driver for USB modems and ISDN adapters
 * 
 * MAC address: 00:04:a3:f4:4c:9c
 * Serial settings 9600,8N1 although other baud rates seem well tolerated
 * 
 * According to Eric Vickary
 *  
 * */

/* Ascii commands */
#define MH_available    'a'
#define MH_reset	    'r'
#define MH_bitwrite	    'B'
#define MH_bitread	    'b'
#define MH_address 	    'A'
#define MH_strong  	    'S'
#define MH_first  	    'f'
#define MH_firstcap	    'F'
#define MH_next  	    'n'
#define MH_nextcap 	    'N'
#define MH_pullup  	    'P'
#define MH_write  	    'W'
#define MH_read  	    'R'
#define MH_lastheard  	'L'
#define MH_clearcache  	'C'
#define MH_info  	    'I'
#define MH_setlocation  'l'
#define MH_version      'V'
#define MH_help         'h'
#define MH_helpcap      'H'
#define MH_sync         's'
#define MH_powerreset   'p'
#define MH_update   	'U'

//static void byteprint( const BYTE * b, int size ) ;
static RESET_TYPE MasterHub_reset(const struct parsedname *pn);
static RESET_TYPE MasterHub_reset_once(struct connection_in * in) ;

static enum search_status MasterHub_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD MasterHub_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;
static GOOD_OR_BAD MasterHub_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void MasterHub_setroutines(struct connection_in *in);
static void MasterHub_close(struct connection_in *in);
static GOOD_OR_BAD MasterHub_directory( struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD MasterHub_select( const struct parsedname * pn ) ;
static GOOD_OR_BAD MasterHub_verify( const struct parsedname * pn ) ;

static GOOD_OR_BAD MasterHub_sender_ascii( char cmd, ASCII * data, int length, struct connection_in * in ) ;
static GOOD_OR_BAD MasterHub_sender_bytes( char cmd, const BYTE * data, int length, struct connection_in * in );

static SIZE_OR_ERROR MasterHub_readin( ASCII * data, size_t minlength, size_t maxlength, struct connection_in * in ) ;
static SIZE_OR_ERROR MasterHub_readin_bytes( BYTE * data, size_t length, struct connection_in * in ) ;
static GOOD_OR_BAD MasterHub_read(BYTE * buf, size_t size, struct connection_in * in) ;

static GOOD_OR_BAD MasterHub_powercycle(struct connection_in *in) ;

static GOOD_OR_BAD MasterHub_sync(struct connection_in *in) ;
static GOOD_OR_BAD MasterHub_sync_once(struct connection_in *in) ;

static GOOD_OR_BAD MasterHub_available(struct connection_in *head) ;
static GOOD_OR_BAD MasterHub_available_once(struct connection_in *head) ;

static void MasterHub_setroutines(struct connection_in *in)
{
	in->iroutines.detect = MasterHub_detect;
	in->iroutines.reset = MasterHub_reset;
	in->iroutines.next_both = MasterHub_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = MasterHub_sendback_data;
    in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = MasterHub_select ;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE ;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = MasterHub_close;
	in->iroutines.verify = MasterHub_verify ;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay ;
	in->bundling_length = 240; // characters not bytes (in hex)
}

GOOD_OR_BAD MasterHub_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	MasterHub_setroutines(in);

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("MasterHub bus master requires port name");
		return gbBAD;
	}

	/* Open the com port */
	COM_set_standard( in ) ; // standard COM port settings
	RETURN_BAD_IF_BAD(COM_open(in)) ;

	// Sync, then get channels
	if ( GOOD( MasterHub_sync(in) ) ) {
		return MasterHub_available( in ) ;
	}
	
	LEVEL_DEFAULT("Error in MasterHub detection: can't sync and query");
	return gbBAD;
}

static RESET_TYPE MasterHub_reset(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	
	if ( MasterHub_reset_once(in) == BUS_RESET_OK ) {
		return BUS_RESET_OK ;
	}

	// resync and retry
	if ( GOOD( MasterHub_sync( in ) ) ) {
		return MasterHub_reset_once( in ) ;
	}

	// resync and retry
	if ( GOOD( MasterHub_sync( in ) ) ) {
		return MasterHub_reset_once( in ) ;
	}

	return BUS_RESET_ERROR ;
}

// single send
static RESET_TYPE MasterHub_reset_once(struct connection_in * in)
{
	ASCII resp[11];
	SIZE_OR_ERROR length ;
	
	RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_reset, "", 0, in ) ) ; // send reset
	
	// two possible returns: "Presence" and "No Presence"
	length = MasterHub_readin( resp, 8, 11, in ) ;
	
	if ( length == -1 ) {
		return BUS_RESET_ERROR;
	}
	
	if ( length == 11 ) {
		in->AnyDevices = anydevices_no ;
	}
	return BUS_RESET_OK;
}

static enum search_status MasterHub_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		// gulp it in
		if ( BAD( MasterHub_directory(ds, pn) ) ) {
			return search_error;
		}
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp)) ) {
	case 0:
		LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(ds->sn));
		return search_good;
	case -ENODEV:
	default:
		ds->LastDevice = 1;
		LEVEL_DEBUG("SN finished");
		return search_done;
	}
}

/************************************************************************/
/*	MasterHub_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm directory 				                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/************************************************************************/
static GOOD_OR_BAD MasterHub_directory(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	char first, next, current ;

	DirblobClear( &(ds->gulp) );

	//Depending on the search type, the MasterHub search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response

	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		LEVEL_DEBUG("Conditional search not supported on MasterHub")
		first = MH_first ;
		next = MH_next ;
	} else {
		first = MH_first ;
		next = MH_next ;
	}
	current = first ;

	if ( MasterHub_reset( pn ) != BUS_RESET_OK ) {
		LEVEL_DEBUG("Unable to reset MasterHub for directory search") ;
		return gbBAD ;
	}
	
	while (1) {
		SIZE_OR_ERROR length ;
		ASCII resp[16];
		BYTE sn[SERIAL_NUMBER_SIZE];

		RETURN_BAD_IF_BAD( MasterHub_sender_ascii( current, "", 0, in ) ) ; // request directory element
	
		/* three possible returns:
		* No more devices
		* No devices found
		* 6300080054141010
		* */
		length = MasterHub_readin( resp, 15, 16, in ) ;

		if ( length < 0 ) {
			LEVEL_DEBUG("Error reading MasterHub directory" );
			return gbBAD ;
		}

		current = next ; // set up for next pass

		if ( length == 15 ) {
			// No more devices
			return gbGOOD ;
		}
		
		if ( strncmp( resp, "No devices found", 16 ) == 0 ) {
			return gbGOOD ;
		}

		sn[7] = string2num(&resp[0]);
		sn[6] = string2num(&resp[2]);
		sn[5] = string2num(&resp[4]);
		sn[4] = string2num(&resp[6]);
		sn[3] = string2num(&resp[8]);
		sn[2] = string2num(&resp[10]);
		sn[1] = string2num(&resp[12]);
		sn[0] = string2num(&resp[14]);

		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));

		// CRC check
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("CRC error sn = %s", sn);
			return gbBAD ;
		}

		DirblobAdd(sn, &(ds->gulp) );
	}
}

static GOOD_OR_BAD MasterHub_select( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	char send_address[16] ;
	char resp[22] ;
	SIZE_OR_ERROR length ;

	if ( (pn->selected_device==NO_DEVICE) || (pn->selected_device==DeviceThermostat) ) {
		return gbRESET( MasterHub_reset(pn) ) ;
	}

	if ( MasterHub_reset( pn ) != BUS_RESET_OK ) {
		LEVEL_DEBUG("Unable to reset MasterHub for device addressing") ;
		return gbBAD ;
	}
	
		
	num2string( &send_address[ 0], pn->sn[7] ) ;
	num2string( &send_address[ 2], pn->sn[6] ) ;
	num2string( &send_address[ 4], pn->sn[5] ) ;
	num2string( &send_address[ 6], pn->sn[4] ) ;
	num2string( &send_address[ 8], pn->sn[3] ) ;
	num2string( &send_address[10], pn->sn[2] ) ;
	num2string( &send_address[12], pn->sn[1] ) ;
	num2string( &send_address[14], pn->sn[0] ) ;
	
	// Send command
	RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_address, send_address, 16, in ) ) ;
	
	/* Possible responses:
	 * +
	 * ! Network Error
	 * ! Device Not Found Error
	 * */
	length = MasterHub_readin( resp, 0, 22, in ) ;
	
	if ( length < 0 ) {
		return gbBAD ;
	}

	return gbGOOD ;
}

static GOOD_OR_BAD MasterHub_verify( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	char send_address[16] ;
	char resp[22] ;
	SIZE_OR_ERROR length ;

	if ( (pn->selected_device==NO_DEVICE) || (pn->selected_device==DeviceThermostat) ) {
		return gbRESET( MasterHub_reset(pn) ) ;
	}

	if ( MasterHub_reset( pn ) != BUS_RESET_OK ) {
		LEVEL_DEBUG("Unable to reset MasterHub for device addressing") ;
		return gbBAD ;
	}
	
		
	num2string( &send_address[ 0], pn->sn[7] ) ;
	num2string( &send_address[ 2], pn->sn[6] ) ;
	num2string( &send_address[ 4], pn->sn[5] ) ;
	num2string( &send_address[ 6], pn->sn[4] ) ;
	num2string( &send_address[ 8], pn->sn[3] ) ;
	num2string( &send_address[10], pn->sn[2] ) ;
	num2string( &send_address[12], pn->sn[1] ) ;
	num2string( &send_address[14], pn->sn[0] ) ;
	
	// Send command
	RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_strong, send_address, 16, in ) ) ;
	
	/* Possible responses:
	 * +
	 * ! Network Error
	 * ! Device Not Found Error
	 * */
	length = MasterHub_readin( resp, 0, 22, in ) ;
	
	if ( length < 0 ) {
		return gbBAD ;
	}

	return gbGOOD ;
}

//  Send data and return response block -- up to 120 bytes
static GOOD_OR_BAD MasterHub_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	SIZE_OR_ERROR length ;

	RETURN_BAD_IF_BAD( MasterHub_sender_bytes( MH_write, data, size, in ) ) ;
	
	length = MasterHub_readin_bytes( resp, size, in ) ;
	
	if ( length != (int) size ) {
		return gbBAD ;
	}
	return gbGOOD ;
}

static GOOD_OR_BAD MasterHub_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;

	for ( left=size ; left>0 ; left -= 120 ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>120)?120:left ;
		RETURN_BAD_IF_BAD( MasterHub_sendback_part( &data[pass_start], &resp[pass_start], pass_size, pn ) ) ;
	}
	return gbGOOD;
}

/********************************************************/
/* MasterHub_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void MasterHub_close(struct connection_in *in)
{
	COM_close(in);

}

// Send bytes to the master hub --
// this routine converts the bytes to ascii hex and uses MasterHub_sender_ascii
static GOOD_OR_BAD MasterHub_sender_bytes( char cmd, const BYTE * data, int length, struct connection_in * in )
{
	char send_string[250] ;
	bytes2string( send_string, data, length ) ;
	return MasterHub_sender_ascii( cmd, send_string, length*2, in  ) ;
}

static GOOD_OR_BAD MasterHub_sender_ascii( char cmd, ASCII * data, int length, struct connection_in * in )
{
	char send_string[250] = { cmd, } ;
	int length_so_far = 1 ;
	
	if ( length > 120 ) {
		LEVEL_DEBUG("String too long for MasterHub" ) ;
		return gbBAD ;
	}
	
	// potentially add the channel char based on type of command
	switch (cmd) {
		case MH_available:	//'a'
		case MH_next:		//'n'
		case MH_nextcap:	//'N'
		case MH_lastheard:	//'L'
		case MH_clearcache:	//'C'
		case MH_info:		//'I'
		case MH_setlocation://'l'
		case MH_version:	//'V'
		case MH_help:		//'h'
		case MH_helpcap:	//'H'
		case MH_sync:		//'s'
		case MH_powerreset: //'p'
		case MH_update:		//'U'
			// command with no channel included
			break ;
		case MH_reset:		//'r'
		case MH_bitwrite:	//'B'
		case MH_bitread:	//'b'
		case MH_address:	//'A'
		case MH_strong:		//'S'
		case MH_first:		//'f'
		case MH_firstcap:	//'F'
		case MH_pullup:		//'P'
		case MH_write:		//'W'
		case MH_read:		//'R'
			// this command needs the channel char added
			send_string[length_so_far] = in->master.masterhub.channel_char ;
			++length_so_far ;
			break ;
	}
	
	memcpy( &send_string[length_so_far], data, length ) ;
	length_so_far += length ;
	
	strcpy( &send_string[length_so_far], "\r" ) ;
	length_so_far += 1 ;
	
	LEVEL_DEBUG("Sending %d chars to masterhub <%s>", length_so_far, send_string ) ;
	
	return COM_write( (BYTE*)send_string, length_so_far, in ) ;
}	
	
static GOOD_OR_BAD MasterHub_read(BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_read( buf, size, in ) ;
}

static GOOD_OR_BAD MasterHub_sync(struct connection_in *in)
{
	int attempts ;
	
	// Try to sync a few times until successful
	for ( attempts = 0 ; attempts < 4 ; ++ attempts ) {
		if ( GOOD( MasterHub_sync_once( in ) ) ) {
			return gbGOOD ;
		}
		LEVEL_DEBUG("Try power cycle to recapture MasterHub" ) ;
		RETURN_BAD_IF_BAD( MasterHub_powercycle( in ) ) ;	
	}

	return MasterHub_sync_once( in ) ;
}

static GOOD_OR_BAD MasterHub_sync_once(struct connection_in *in)
{
	int attempts ;
	
	// Try to sync a few times until successful
	for ( attempts = 0 ; attempts < 4 ; ++ attempts ) {
		ASCII read_buffer[10] ;
	
		COM_slurp(in) ; // empty pending data

		RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_sync, "", 0, in ) ) ; // send sync
		
		// read back
		if ( MasterHub_readin( read_buffer, 0, 10, in ) == 0 ) {
		LEVEL_DEBUG("Sync success") ;
			return gbGOOD ;
		}
		
		LEVEL_DEBUG( "Bad sync response %d", attempts ) ;
	}

	LEVEL_DEBUG("Sync failure") ;
	return gbBAD ;
}

static GOOD_OR_BAD MasterHub_powercycle(struct connection_in *in)
{
	LEVEL_DEBUG("MasterHub initial power cycle start" );
	RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_powerreset, "", 0, in ) ); // send power cycle
	sleep(2) ;
	LEVEL_DEBUG("MasterHub initial power cycle done" );
	return gbGOOD ;
}

static SIZE_OR_ERROR MasterHub_readin_bytes( BYTE * data, size_t length, struct connection_in * in )
{
	ASCII buffer[250] ;
	SIZE_OR_ERROR lengthback = MasterHub_readin( buffer, length*2, length*2, in ) ;
	
	if ( lengthback < 0 ) {
		return lengthback ;
	}
	
	string2bytes( buffer, data, length ) ;
	
	return length ;
}

// data is a buffer of *length size
// returns data and length  -- don't include preamble (+ ) and post-amble (\r\n?)
//
// Also special case in minlength==0: "+\r\n?" ( no space after + )
static SIZE_OR_ERROR MasterHub_readin( ASCII * data, size_t minlength, size_t maxlength, struct connection_in * in )
{
	size_t min_l = 2 + 2 + 1 ; // "+ \r\n?" or "! \r\n?"
	size_t max_l = 250 ;
	BYTE buffer[max_l+min_l] ;
	SIZE_OR_ERROR length ;
	BYTE end_string[] = { 0x0D, 0x0A, '?' } ;
	
	if ( max_l < maxlength ) {
		LEVEL_DEBUG( "read too large" ) ;
		return gbBAD ;
	}
	
	memset( data, 0, maxlength ) ; // zero-fill
	
	if ( minlength > 0 ) {
		// read enough for error
		RETURN_BAD_IF_BAD( MasterHub_read( buffer, min_l, in ) ) ;
		
		if ( maxlength == 0 ) {
			// no data
			return 0 ;
		}
		
		// read rest of minimum
		RETURN_BAD_IF_BAD( MasterHub_read( &buffer[min_l], minlength, in ) ) ;
	} else {
		// special case with minlength -- 0 to allow "+\t\n?" -- no space
		// read enough for error
		RETURN_BAD_IF_BAD( MasterHub_read( buffer, min_l-1, in ) ) ;
		
		if ( memcmp( &buffer[1], end_string, 3 ) == 0 ) {
			// Success!
			if ( buffer[0] != '+' ) {
				// an error of sorts
				return -1 ;
			}
			return 0 ;
		} 
		// read rest of minimum ( for the ignored space
		RETURN_BAD_IF_BAD( MasterHub_read( &buffer[min_l-1], 1, in ) ) ;
	}
	
	for ( length = minlength ; length < ( ssize_t) maxlength ; ++length ) {
		if ( memcmp( &buffer[ length+min_l-3 ], end_string, 3 ) == 0 ) {
			// Success!
			memcpy( data, &buffer[2], length ) ;
			if ( buffer[0] != '+' ) {
				// an error of sorts
				return -length ;
			}
			return length ;
		} 
		RETURN_BAD_IF_BAD( MasterHub_read( &buffer[min_l+length], 1, in ) ) ;
	}

	memcpy( data, &buffer[2], maxlength ) ;
	if ( buffer[0] != '+' ) {
		COM_slurp( in ) ;
		return -maxlength ;
	}
	return maxlength ;
}	

// Not only finds the available channels, but sets up the port and connections.
// Note this is the only place to change if future hubs support more channels
static GOOD_OR_BAD MasterHub_available(struct connection_in *head)
{
	int attempts ;
	
	for ( attempts = 0 ; attempts < 4 ; ++ attempts ) {
		if ( GOOD( MasterHub_available_once( head ) ) ) {
			return gbGOOD ;
		}
		LEVEL_DEBUG("Need to try searching for available MasterHub channels again") ;
		RETURN_BAD_IF_BAD( MasterHub_sync( head ) ) ;
	}
		
	return MasterHub_available_once( head ) ; ;
}

static GOOD_OR_BAD MasterHub_available_once(struct connection_in *head)
{
	SIZE_OR_ERROR length ;
	int index ;
	ASCII data[6] ;
	char *name[] = { "MasterHub(All)", "MasterHub(1)", "MasterHub(2)",
		"MasterHub(3)", "MasterHub(4)", "MasterHub(W)",
	};
	
	RETURN_BAD_IF_BAD( MasterHub_sender_ascii( MH_available, "", 0, head ) ) ; // send available
	
	// two possible returns: A1234 and A1234W
	length = MasterHub_readin( data, 5, 6, head ) ;
	
	if ( length < 0 ) {
		return gbBAD ;
	}
	
	LEVEL_DEBUG( "Available %*s", length, data ) ;
	
	for ( index = 1 ; index < length ; ++index ) {
		struct connection_in * added ;
		if ( index == 1 ) {
			// already head
			added = head ;
		} else {
			added = AddtoPort(head->pown);
			if (added == NO_CONNECTION) {
				return gbBAD;
			}
		}
		added->master.masterhub.channel_char = data[index] ;
		added->master.masterhub.channels = length - 1 ; // ignore 'A' (All channels)
		added->adapter_name = name[index];
		added->Adapter = adapter_masterhub ;
		LEVEL_DEBUG( "Added %s on channel %c", added->adapter_name, added->master.masterhub.channel_char ) ;
	}
		
	return gbGOOD ;
}

