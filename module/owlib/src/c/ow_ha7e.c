/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

// regex

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

//static void byteprint( const BYTE * b, int size ) ;
static RESET_TYPE HA7E_reset(const struct parsedname *pn);
static enum search_status HA7E_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD HA7E_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;
static GOOD_OR_BAD HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void HA7E_setroutines(struct connection_in *in);
static void HA7E_close(struct connection_in *in);
static GOOD_OR_BAD HA7E_directory( struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD HA7E_select( const struct parsedname * pn ) ;
static GOOD_OR_BAD HA7E_resync( const struct parsedname * pn ) ;
static void HA7E_powerdown(struct connection_in * in) ;

static void HA7E_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA7E_detect;
	in->iroutines.reset = HA7E_reset;
	in->iroutines.next_both = HA7E_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = HA7E_sendback_data;
    in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = HA7E_select ;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE ;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = HA7E_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

GOOD_OR_BAD HA7E_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	HA7E_setroutines(in);

	// Poison current "Address" for adapter
	memset( in->remembered_sn, 0x00, SERIAL_NUMBER_SIZE ) ;

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("HA7E bus master requires port name");
		return gbBAD;
	}

	/* Open the com port */
	COM_set_standard( in ) ; // standard COM port settings
	RETURN_BAD_IF_BAD(COM_open(in)) ;
	COM_slurp(in) ;
	if ( GOOD( gbRESET( HA7E_reset(&pn) ) ) ) {
		in->Adapter = adapter_HA7E ;
		in->adapter_name = "HA7E/S";
		return gbGOOD;
	}
	if ( GOOD( serial_powercycle(in) ) ) {
		COM_slurp(in) ;
		if ( GOOD( gbRESET( HA7E_reset(&pn) ) ) ) {
			in->Adapter = adapter_HA7E ;
			in->adapter_name = "HA7E/S";
			return gbGOOD;
		}
	}
	
	pin->flow = flow_second; // flow control
	RETURN_BAD_IF_BAD(COM_change(in)) ;
	COM_slurp(in) ;
	if ( GOOD( gbRESET( HA7E_reset(&pn) ) ) ) {
		in->Adapter = adapter_HA7E ;
		in->adapter_name = "HA7E/S";
		return gbGOOD;
	}
	
	LEVEL_DEFAULT("Error in HA7E detection: can't perform RESET");
	return gbBAD;
}

static RESET_TYPE HA7E_reset(const struct parsedname *pn)
{
	BYTE resp[1];

	COM_flush(pn->selected_connection);
	if ( BAD(COM_write((BYTE*)"R", 1, pn->selected_connection)) ) {
		LEVEL_DEBUG("Error sending HA7E reset");
		return BUS_RESET_ERROR;
	}
	if ( BAD(COM_read(resp, 1, pn->selected_connection)) ) {
		LEVEL_DEBUG("Error reading HA7E reset");
		return BUS_RESET_ERROR;
	}
	if (resp[0]!=0x0D) {
		LEVEL_DEBUG("Error HA7E reset bad <cr>");
		return BUS_RESET_ERROR;
	}
	return BUS_RESET_OK;
}

static enum search_status HA7E_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->LastDevice) {
		return search_done;
	}

	COM_flush(pn->selected_connection);

	if (ds->index == -1) {
		if ( BAD( HA7E_directory(ds, pn) ) ) {
			return search_error;
		}
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp)) ) {
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
/*	HA7E_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm directory 				                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/************************************************************************/
static GOOD_OR_BAD HA7E_directory(struct device_search *ds, const struct parsedname *pn)
{
	char resp[17];
	BYTE sn[SERIAL_NUMBER_SIZE];
	char *first, *next, *current ;

	DirblobClear( &(ds->gulp) );

	//Depending on the search type, the HA7E search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		first = "C" ;
		next = "c" ;
	} else {
		first = "S" ;
		next = "s" ;
	}
	current = first ;

	while (1) {
		if ( BAD(COM_write((BYTE*)current, 1, pn->selected_connection)) ) {
			return HA7E_resync(pn) ;
		}
		current = next ; // set up for next pass
		//One needs to check the first character returned.
		//If nothing is found, the ha7e will timeout rather then have a quick
		//return.  This happens when looking at the alarm directory and
		//there are no alarms pending
		//So we grab the first character and check it.  If not an E leave it
		//in the resp buffer and get the rest of the response from the HA7E
		//device
		if ( BAD(COM_read((BYTE*)resp, 1, pn->selected_connection)) ) {
			return HA7E_resync(pn) ;
		}
		if ( resp[0] == 0x0D ) {
			return gbGOOD ; // end of list
		}
		if ( BAD(COM_read((BYTE*)&resp[1], 16, pn->selected_connection)) ) {
			return HA7E_resync(pn) ;
		}
		sn[7] = string2num(&resp[0]);
		sn[6] = string2num(&resp[2]);
		sn[5] = string2num(&resp[4]);
		sn[4] = string2num(&resp[6]);
		sn[3] = string2num(&resp[8]);
		sn[2] = string2num(&resp[10]);
		sn[1] = string2num(&resp[12]);
		sn[0] = string2num(&resp[14]);

		// Set as current "Address" for adapter
		memcpy( pn->selected_connection->remembered_sn, sn, SERIAL_NUMBER_SIZE) ;

		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));
		if ( resp[16]!=0x0D ) {
			return HA7E_resync(pn) ;
		}

		// CRC check
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s", sn);
			return HA7E_resync(pn) ;
		}

		DirblobAdd(sn, &(ds->gulp) );
	}
}

static GOOD_OR_BAD HA7E_resync( const struct parsedname * pn )
{
	COM_flush(pn->selected_connection);
	HA7E_reset(pn);
	COM_flush(pn->selected_connection);

	// Poison current "Address" for adapter
	memset( pn->selected_connection->remembered_sn, 0, SERIAL_NUMBER_SIZE ) ; // so won't match

	return gbBAD ;
}

static GOOD_OR_BAD HA7E_select( const struct parsedname * pn )
{
	char send_address[18] ;
	char resp_address[17] ;

	if ( (pn->selected_device==NO_DEVICE) || (pn->selected_device==DeviceThermostat) ) {
		return gbRESET( HA7E_reset(pn) ) ;
	}

	send_address[0] = 'A' ;
	num2string( &send_address[ 1], pn->sn[7] ) ;
	num2string( &send_address[ 3], pn->sn[6] ) ;
	num2string( &send_address[ 5], pn->sn[5] ) ;
	num2string( &send_address[ 7], pn->sn[4] ) ;
	num2string( &send_address[ 9], pn->sn[3] ) ;
	num2string( &send_address[11], pn->sn[2] ) ;
	num2string( &send_address[13], pn->sn[1] ) ;
	num2string( &send_address[15], pn->sn[0] ) ;
	send_address[17] = 0x0D;

	if ( memcmp( pn->sn, pn->selected_connection->remembered_sn, SERIAL_NUMBER_SIZE ) ) {
		if ( BAD(COM_write((BYTE*)send_address,18,pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with sending HA7E A-ddress") ;
			return HA7E_resync(pn) ;
		}
	} else {
		if ( BAD(COM_write((BYTE*)"M",1,pn->selected_connection)) ) {
			LEVEL_DEBUG("Error with sending HA7E M-atch") ;
			return HA7E_resync(pn) ;
		}
	}
	if ( BAD(COM_read((BYTE*)resp_address,17,pn->selected_connection)) ) {
		LEVEL_DEBUG("Error with reading HA7E select") ;
		return HA7E_resync(pn) ;
	}
	if ( memcmp( &resp_address[0],&send_address[1],17) ) {
		LEVEL_DEBUG("Error with HA7E select response") ;
		return HA7E_resync(pn) ;
	}

	// Set as current "Address" for adapter
	memcpy( pn->selected_connection->remembered_sn, pn->sn, SERIAL_NUMBER_SIZE) ;

	return gbGOOD ;
}

//  Send data and return response block -- up to 32 bytes
static GOOD_OR_BAD HA7E_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	char send_data[1+2+32*2+1] ;
	char get_data[32*2+1] ;

	send_data[0] = 'W' ;
	num2string( &send_data[1], size ) ;
	bytes2string(&send_data[3], data, size) ;
	send_data[3+2*size] = 0x0D ;

	if ( BAD(COM_write((BYTE*)send_data,size*2+4,pn->selected_connection)) ) {
		LEVEL_DEBUG("Error with sending HA7E block") ;
		HA7E_resync(pn) ;
		return gbBAD ;
	}
	if ( BAD(COM_read((BYTE*)get_data,size*2+1,pn->selected_connection)) ) {
		LEVEL_DEBUG("Error with reading HA7E block") ;
		HA7E_resync(pn) ;
		return gbBAD ;
	}

	string2bytes(get_data, resp, size) ;
	return gbGOOD ;
}

static GOOD_OR_BAD HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;

	for ( left=size ; left>0 ; left -= 32 ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>32)?32:left ;
		RETURN_BAD_IF_BAD( HA7E_sendback_part( &data[pass_start], &resp[pass_start], pass_size, pn ) ) ;
	}
	return gbGOOD;
}

/********************************************************/
/* HA7E_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void HA7E_close(struct connection_in *in)
{
	HA7E_powerdown(in) ;
	COM_close(in);

}

static void HA7E_powerdown(struct connection_in * in)
{
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	COM_write((BYTE*)"P", 1, in) ;
	COM_slurp(in) ;
}
