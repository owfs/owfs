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
static RESET_TYPE HA5_reset(const struct parsedname *pn);
static RESET_TYPE HA5_reset_wrapped(const struct parsedname *pn) ;
static enum search_status HA5_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD HA5_sendback_part(char cmd, const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;
static GOOD_OR_BAD HA5_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD HA5_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void HA5_setroutines(struct connection_in *in);
static void HA5_close(struct connection_in *in);
static GOOD_OR_BAD HA5_reconnect( const struct parsedname  *pn ) ;
static enum search_status HA5_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static GOOD_OR_BAD HA5_select( const struct parsedname * pn ) ;
static GOOD_OR_BAD HA5_select_wrapped( const struct parsedname * pn ) ;
static GOOD_OR_BAD HA5_resync( const struct parsedname * pn ) ;
static void HA5_powerdown(struct connection_in * in) ;
static GOOD_OR_BAD HA5_channel_list( char * alpha_string, struct parsedname * pn ) ;
static GOOD_OR_BAD HA5_test_channel( char c, const struct parsedname  *pn ) ;
static int AddChecksum( unsigned char * check_string, int length, struct connection_in * in ) ;
static GOOD_OR_BAD TestChecksum( unsigned char * check_string, int length ) ;
static GOOD_OR_BAD HA5_find_channel(struct parsedname *pn) ;

#if OW_MT

#define HA5MUTEX_INIT(in)		_MUTEX_INIT(in->master.ha5.all_channel_lock)
#define HA5MUTEX_LOCK(in)		_MUTEX_LOCK(in->master.ha5.all_channel_lock ) ;
#define HA5MUTEX_UNLOCK(in)		_MUTEX_UNLOCK(in->master.ha5.all_channel_lock);
#define HA5MUTEX_DESTROY(in)	_MUTEX_DESTROY(in->master.ha5.all_channel_lock);

#else /* OW_MT */

#define HA5MUTEX_INIT(in)			(void) in
#define HA5MUTEX_LOCK(in)			(void) in
#define HA5MUTEX_UNLOCK(in)			(void) in	
#define HA5MUTEX_DESTROY(in)		(void) in	

#endif /* ow_MT */

#define CR_char		(0x0D)

static void HA5_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA5_detect;
	in->iroutines.reset = HA5_reset;
	in->iroutines.next_both = HA5_next_both;
	in->iroutines.PowerByte = NULL;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = HA5_sendback_data;
	in->iroutines.select_and_sendback = HA5_select_and_sendback;
	//    in->iroutines.sendback_bits = ;
	in->iroutines.select = HA5_select ;
	in->iroutines.reconnect = HA5_reconnect;
	in->iroutines.close = HA5_close;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset;
	in->bundling_length = HA5_FIFO_SIZE;
}

GOOD_OR_BAD HA5_detect(struct connection_in *in)
{
	struct parsedname pn;
	struct address_pair ap ;
	GOOD_OR_BAD gbResult ;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	HA5_setroutines(in);
	
	// Poison current "Address" for adapter
	in->master.ha5.sn[0] = 0 ; // so won't match

	Parse_Address( in->name, &ap ) ;
	if ( ap.first.type==address_none ) {
		// Cannot open serial port
		Free_Address( &ap ) ;
		return gbBAD ;
	}
	strcpy(in->name, ap.first.alpha) ; // subset so will fit
	if ( BAD(COM_open(in)) ) {
		// Cannot open serial port
		Free_Address( &ap ) ;
		return gbBAD ;
	}

	// 9600 isn't valid for the HA5, so we can tell that this value was actually selected
	if ( Globals.baud != B9600 ) {
		in->baud = Globals.baud ;
	} else {
		in->baud = B115200 ; // works at this fast rate
	}

	// allowable speeds
	COM_BaudRestrict( &(in->baud), B1200, B19200, B38400, B115200, 0 ) ;

	/* Set com port speed*/
	COM_speed(in->baud,in) ;

	in->master.ha5.checksum = Globals.checksum ;
	in->Adapter = adapter_HA5 ;
	in->adapter_name = "HA5";

	/* By definition, this is the head adapter on this port */
	in->master.ha5.head = in ;
	HA5MUTEX_INIT(in);

	/* Find the channels */
	if ( ap.second.type == address_none ) { // scan for channel
		gbResult = HA5_find_channel(&pn) ;
	} else { // A list of channels
		gbResult = HA5_channel_list( ap.second.alpha, &pn ) ;
	}

	if ( BAD( gbResult ) ) {
		HA5_close(in) ;
	} else {		
		COM_slurp(in->file_descriptor) ;
		HA5_reset(&pn) ;
	}
	return gbResult;
}

static GOOD_OR_BAD HA5_channel_list( char * alpha_string, struct parsedname * pn )
{
	char * current_char ;
	struct connection_in * initial_in = pn->selected_connection ;
	struct connection_in * current_in = initial_in ;

	for ( current_char = alpha_string ; current_char[0]!= '\0' ; ++current_char ) {
		char c = current_char[0];
		if ( !isalpha(c) ) {
			LEVEL_DEBUG("Urecognized HA5 channel <%c>",c) ;
			continue ;
		}
		c = tolower(c) ;
		LEVEL_DEBUG("Looking for HA5 adapter on %s:%c", current_in->name, c ) ;
		if ( GOOD( HA5_test_channel(c,pn)) ) {
			LEVEL_CONNECT("HA5 bus master FOUND on port %s at channel %c", current_in->name, current_in->master.ha5.channel ) ;
			current_in = NewIn( initial_in ) ;
			if ( current_in == NULL ) {
				break ;
			}
			pn->selected_connection = current_in ;
			current_in->name = owstrdup( initial_in->name ) ; // copy COM port
		} else {
			LEVEL_CONNECT("HA5 bus master NOT FOUND on port %s at channel %c", current_in->name, c ) ;
		}
	}
	
	// Now see if we've added any adapters
	if ( current_in == initial_in ) {
		return gbBAD ;
	}
	
	// Remove last addition, it is blank
	RemoveIn( current_in ) ;
	// Restore selected_connection
	pn->selected_connection = initial_in ;
	return gbGOOD ;
}

/* Find the HA5 channel (since none specified) */
/* Arbitrarily assign it to "a" if none found */
static GOOD_OR_BAD HA5_find_channel(struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	char c ;

	for ( c = 'a' ; c <= 'z' ; ++c ) {
		LEVEL_DEBUG("Looking for HA5 adapter on %s:%c", in->name, c ) ;
		if ( GOOD( HA5_test_channel(c,pn)) ) {
			LEVEL_CONNECT("HA5 bus master FOUND on port %s at channel %c", in->name, in->master.ha5.channel ) ;
			return gbGOOD ;
		}
	}
	LEVEL_DEBUG("HA5 bus master NOT FOUND on port %s", in->name ) ;
	in->master.ha5.channel = '\0' ; 
	return gbBAD;
}

/* Adds bytes to end of string -- either 1 byte for <CR> or 3 bytes for <checksum><CR> */
/* length if length of string without checksum/CR */
/* check_string must have enough space allocated */
static int AddChecksum( unsigned char * check_string, int length, struct connection_in * in )
{
	if ( in->master.ha5.checksum ) {
		int i ;
		unsigned int sum = 0 ;
		for ( i=0 ; i<length ; ++i ) {
			sum += check_string[i] ;
		}
		num2string((char *) &check_string[length], sum & 0xFF) ;
		check_string[length+2] = CR_char ;
		return length+3 ;
	} else {
		check_string[length] = CR_char ;
		return length+1 ;
	}
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
	COM_close(in) ;
	RETURN_BAD_IF_BAD( COM_open(in) ) ;
	return HA5_test_channel( in->master.ha5.channel, pn ) ;
}

static GOOD_OR_BAD HA5_test_channel( char c, const struct parsedname  *pn )
{
	unsigned char test_string[1+1+2+1] ;
	unsigned char test_response[1] ;
	int string_length ;
	struct connection_in * in = pn->selected_connection ;

	COM_slurp(in->file_descriptor) ;

	test_string[0] = c ;
	test_string[1] = 'R' ;
	string_length = AddChecksum( test_string, 2, in ) ;

	RETURN_BAD_IF_BAD( COM_write( test_string, string_length, pn->selected_connection ) ) ;
	RETURN_BAD_IF_BAD( COM_read( test_response, 1, pn->selected_connection ) ) ;

	in->master.ha5.channel = c ;
	COM_slurp(in->file_descriptor) ;
	return gbGOOD ;
}

static RESET_TYPE HA5_reset(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	RESET_TYPE ret ;

	HA5MUTEX_LOCK( in->master.ha5.head ) ;
	ret = HA5_reset_wrapped(pn) ;
	HA5MUTEX_UNLOCK( in->master.ha5.head ) ;

	return ret ;
}

static RESET_TYPE HA5_reset_wrapped(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int reset_length ;
	BYTE reset[5] ;
	BYTE resp[2];

	reset[0] = in->master.ha5.channel ;
	reset[1] = 'R' ;
	reset_length = AddChecksum( reset, 2, in ) ;

	if ( BAD(COM_write(reset, reset_length, pn->selected_connection)) ) {
		LEVEL_DEBUG("Error sending HA5 reset");
		return BUS_RESET_ERROR;
	}
	// For some reason, the HA5 doesn't use a checksum for RESET response.
	if ( BAD(COM_read(resp, 2, pn->selected_connection)) ) {
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
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(in->alarm) : &(in->main);

	if (ds->LastDevice) {
		return search_done;
	}

	COM_flush(pn->selected_connection);

	if (ds->index == -1) {
		enum search_status ret ;
		HA5MUTEX_LOCK( in->master.ha5.head ) ;
		ret = HA5_directory(ds, db, pn) ;
		HA5MUTEX_UNLOCK( in->master.ha5.head ) ;

		if ( ret != search_good ) {
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
static enum search_status HA5_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	unsigned char resp[20];
	unsigned char query[5+2+1] ;
	int query_length ;
	struct connection_in * in = pn->selected_connection ;

	DirblobClear(db);

	//Depending on the search type, the HA5 search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	query[0] = in->master.ha5.channel ;
	query[1] = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ? 'C' : 'S' ;
	query[2] = ',' ;
	query[3] = 'F' ;
	query[4] = 'F' ;
	query_length = AddChecksum( query, 5, in ) ;

	if ( BAD(COM_write( query, query_length, pn->selected_connection)) ) {
		HA5_resync(pn) ;
		return search_error ;
	}

	if ( BAD(COM_read(resp, 1, pn->selected_connection)) ) {
		HA5_resync(pn) ;
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
			if ( BAD(COM_read(&resp[1], 19, pn->selected_connection)) ) {
				HA5_resync(pn) ;
				return search_error ;
			}
			if ( resp[18]!=CR_char ) {
				HA5_resync(pn) ;
				return search_error ;
			}
			wrap_char = resp[19] ;
			if ( BAD( TestChecksum( resp, 16 )) ) {
				HA5_resync(pn) ;
				return search_error ;
			}
		} else {
			if ( BAD(COM_read(&resp[1], 17, pn->selected_connection)) ) {
				HA5_resync(pn) ;
				return search_error ;
			}
			if ( resp[16]!=CR_char ) {
				HA5_resync(pn) ;
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
		memcpy( pn->selected_connection->master.ha5.sn, sn, 8) ;

		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));
		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s", sn);
			HA5_resync(pn) ;
			return search_error ;
		}
		DirblobAdd(sn, db);
		resp[0] = wrap_char ;
	}
	return search_good ;
}

static GOOD_OR_BAD HA5_resync( const struct parsedname * pn )
{
	COM_flush(pn->selected_connection);
	HA5_reset(pn);
	COM_flush(pn->selected_connection);

	// Poison current "Address" for adapter
	pn->selected_connection->master.ha5.sn[0] = 0 ; // so won't match

	return gbBAD ;
}

static GOOD_OR_BAD HA5_select( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	GOOD_OR_BAD ret ;

	if ( (pn->selected_device==NULL) || (pn->selected_device==DeviceThermostat) ) {
		RETURN_BAD_IF_BAD( gbRESET( HA5_reset(pn) ) ) ;
	}

	HA5MUTEX_LOCK( in->master.ha5.head ) ;
	ret = HA5_select_wrapped(pn) ;
	HA5MUTEX_UNLOCK( in->master.ha5.head ) ;

	return ret ;
}

static GOOD_OR_BAD HA5_select_wrapped( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	unsigned char send_address[21] ;
	unsigned char resp_address[19] ;
	int send_length ;

	send_address[0] = in->master.ha5.channel ;
	send_address[1] = 'A' ;
	num2string( (char *)&send_address[ 2], pn->sn[7] ) ;
	num2string( (char *)&send_address[ 4], pn->sn[6] ) ;
	num2string( (char *)&send_address[ 6], pn->sn[5] ) ;
	num2string( (char *)&send_address[ 8], pn->sn[4] ) ;
	num2string( (char *)&send_address[10], pn->sn[3] ) ;
	num2string( (char *)&send_address[12], pn->sn[2] ) ;
	num2string( (char *)&send_address[14], pn->sn[1] ) ;
	num2string( (char *)&send_address[16], pn->sn[0] ) ;

	send_length = AddChecksum( send_address, 18, in ) ;

	if ( BAD(COM_write( send_address, send_length, pn->selected_connection)) ) {
		LEVEL_DEBUG("Error with sending HA5 A-ddress") ;
		return HA5_resync(pn) ;
	}

	if ( in->master.ha5.checksum ) {
		if ( BAD(COM_read(resp_address,19,pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with reading HA5 select") ;
			return HA5_resync(pn) ;
		}
		if ( BAD(TestChecksum( resp_address, 16)) ) {
			LEVEL_DEBUG("HA5 select checksum error") ;
			return HA5_resync(pn) ;
		}
	} else {
		if ( BAD(COM_read(resp_address,17,pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with reading HA5 select") ;
			return HA5_resync(pn) ;
		}
	}
	if ( memcmp( &resp_address[0],&send_address[2],16) ) {
		LEVEL_DEBUG("Error with HA5 select response") ;
		return HA5_resync(pn) ;
	}

	// Set as current "Address" for adapter
	memcpy( in->master.ha5.sn, pn->sn, SERIAL_NUMBER_SIZE) ;

	return gbGOOD ;
}

//  Send data and return response block -- up to 32 bytes
static GOOD_OR_BAD HA5_sendback_part(char cmd, const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	unsigned char send_data[2+2+32*2+3] ;
	unsigned char get_data[32*2+3] ;
	int send_length ;

	send_data[0] = in->master.ha5.channel ;
	send_data[1] = cmd ;
	num2string( (char *)&send_data[2], size ) ;
	bytes2string( (char *)&send_data[4], data, size) ;
	send_length = AddChecksum( send_data, 4+size*2, in ) ;

	if ( BAD(COM_write( send_data, send_length, pn->selected_connection)) ) {
		LEVEL_DEBUG("Error with sending HA5 block") ;
		HA5_resync(pn) ;
		return gbBAD ;
	}

	if ( in->master.ha5.checksum ) {
		if ( BAD(COM_read( get_data, size*2+3, pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with reading HA5 block") ;
			HA5_resync(pn) ;
			return gbBAD ;
		}
		if ( BAD(TestChecksum( get_data, size*2)) ) {
			LEVEL_DEBUG("HA5 block read checksum error") ;
			HA5_resync(pn) ;
			return gbBAD ;
		}
	} else {
		if ( BAD(COM_read( get_data, size*2+1, pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with reading HA5 block") ;
			HA5_resync(pn) ;
			return gbBAD ;
		}
	}
	string2bytes( (char *)get_data, resp, size) ;
	return gbGOOD ;
}

static GOOD_OR_BAD HA5_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int left;

	for ( left=size ; left>0 ; left -= 32 ) {
		GOOD_OR_BAD ret ;
		size_t pass_start = size - left ;
		size_t pass_size = (left>32)?32:left ;

		HA5MUTEX_LOCK( in->master.ha5.head ) ;
		ret = HA5_sendback_part( 'W', &data[pass_start], &resp[pass_start], pass_size, pn ) ;
		HA5MUTEX_UNLOCK( in->master.ha5.head ) ;

		RETURN_BAD_IF_BAD( ret ) ;
	}
	return gbGOOD;
}

static GOOD_OR_BAD HA5_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int left;
	char block_cmd ;

	if ( memcmp( pn->sn, in->master.ha5.sn, SERIAL_NUMBER_SIZE ) ) {
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

		HA5MUTEX_LOCK( in->master.ha5.head ) ;
		ret = HA5_sendback_part( block_cmd, &data[pass_start], &resp[pass_start], pass_size, pn ) ;
		HA5MUTEX_UNLOCK( in->master.ha5.head ) ;
		block_cmd = 'W' ; // for next pass
		RETURN_BAD_IF_BAD( ret ) ;
	}
	return gbGOOD;
}

/********************************************************/
/* HA5_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void HA5_close(struct connection_in *in)
{
	HA5_powerdown(in) ;
	if ( in->master.ha5.head == in ) {
		HA5MUTEX_DESTROY(in);
	}
	COM_close(in);
}

static void HA5_powerdown(struct connection_in * in)
{
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	COM_write((BYTE*)"P", 1, in) ;
	if ( FILE_DESCRIPTOR_VALID(in->file_descriptor) ) {
		COM_slurp(in->file_descriptor) ;
	}
}
