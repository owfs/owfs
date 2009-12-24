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
static int OWServer_Enet_reset(const struct parsedname *pn);
static int OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn);
static int OWServer_Enet_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;
static int OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void OWServer_Enet_setroutines(struct connection_in *in);
static void OWServer_Enet_close(struct connection_in *in);
static int OWServer_Enet_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static int OWServer_Enet_select( const struct parsedname * pn ) ;
static int OWServer_Enet_resync( const struct parsedname * pn ) ;
static void OWServer_Enet_powerdown(struct connection_in * in) ;

static void OWServer_Enet_setroutines(struct connection_in *in)
{
	in->iroutines.detect = OWServer_Enet_detect;
	in->iroutines.reset = OWServer_Enet_reset;
	in->iroutines.next_both = OWServer_Enet_next_both;
	in->iroutines.PowerByte = NULL;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = OWServer_Enet_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = OWServer_Enet_select ;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = OWServer_Enet_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset;
	in->bundling_length = HA7E_FIFO_SIZE;
}

int OWServer_Enet_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	OWServer_Enet_setroutines(in);

	// Poison current "Address" for adapter
	in->connin.enet.sn[0] = 0 ; // so won't match

	/* Open the com port */
	if (COM_open(in)) {
		return -ENODEV;
	}

	// set the baud rate to 9600. (Already set to 9600 in COM_open())
	in->baud = B9600 ;
	// allowable speeds
	COM_BaudRestrict( &(in->baud), B9600, 0 ) ;
	COM_speed(in->baud, in);
	COM_slurp(in->file_descriptor) ;

	if (OWServer_Enet_reset(&pn) == BUS_RESET_OK ) {
		in->Adapter = adapter_ENET ;
		in->adapter_name = "OWServer_Enet/S";
		return 0;
	} else if (OWServer_Enet_reset(&pn) == BUS_RESET_OK ) {
		in->Adapter = adapter_ENET ;
		in->adapter_name = "OWServer_Enet/S";
		return 0;
	}
	LEVEL_DEFAULT("error\n");
	return -ENODEV;
}

static int OWServer_Enet_reset(const struct parsedname *pn)
{
	BYTE resp[1];

	COM_flush(pn->selected_connection);
	if (COM_write((BYTE*)"R", 1, pn->selected_connection)) {
		LEVEL_DEBUG("Error sending OWServer_Enet reset\n");
		return -EIO;
	}
	if (COM_read(resp, 1, pn->selected_connection)) {
		LEVEL_DEBUG("Error reading OWServer_Enet reset\n");
		return -EIO;
	}
	if (resp[0]!=0x0D) {
		LEVEL_DEBUG("Error OWServer_Enet reset bad <cr>\n");
		return -EIO;
	}
	return BUS_RESET_OK;
}

static int OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->alarm) : &(pn->selected_connection->main);

	if (ds->LastDevice) {
		return -ENODEV;
	}

	COM_flush(pn->selected_connection);

	if (ds->index == -1) {
		if (OWServer_Enet_directory(ds, db, pn)) {
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

	LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(ds->sn));
	return ret;
}

/************************************************************************/
/*	OWServer_Enet_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/************************************************************************/
static int OWServer_Enet_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	char resp[17];
	BYTE sn[8];
	char *first, *next, *current ;

	DirblobClear(db);

	//Depending on the search type, the OWServer_Enet search function
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
		if (COM_write((BYTE*)current, 1, pn->selected_connection)) {
			return OWServer_Enet_resync(pn) ;
		}
		current = next ; // set up for next pass
		//One needs to check the first character returned.
		//If nothing is found, the OWServer_Enet will timeout rather then have a quick
		//return.  This happens when looking at the alarm directory and
		//there are no alarms pending
		//So we grab the first character and check it.  If not an E leave it
		//in the resp buffer and get the rest of the response from the OWServer_Enet
		//device
		if (COM_read((BYTE*)resp, 1, pn->selected_connection)) {
			return OWServer_Enet_resync(pn) ;
		}
		if ( resp[0] == 0x0D ) {
			return 0 ; // end of list
		}
		if (COM_read((BYTE*)&resp[1], 16, pn->selected_connection)) {
			return OWServer_Enet_resync(pn) ;
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
		memcpy( pn->selected_connection->connin.enet.sn, sn, 8) ;

		LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(sn));
		if ( resp[16]!=0x0D ) {
			return OWServer_Enet_resync(pn) ;
		}

		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s\n", sn);
			return OWServer_Enet_resync(pn) ;
		}

		DirblobAdd(sn, db);
	}
}

static int OWServer_Enet_resync( const struct parsedname * pn )
{
	COM_flush(pn->selected_connection);
	OWServer_Enet_reset(pn);
	COM_flush(pn->selected_connection);

	// Poison current "Address" for adapter
	pn->selected_connection->connin.enet.sn[0] = 0 ; // so won't match

	return -EIO ;
}

static int OWServer_Enet_select( const struct parsedname * pn )
{
	char send_address[18] ;
	char resp_address[17] ;

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

	if ( (pn->selected_device==NULL) || (pn->selected_device==DeviceThermostat) ) {
		return OWServer_Enet_reset(pn) ;
	}

	if ( memcmp( pn->sn, pn->selected_connection->connin.enet.sn, 8 ) ) {
		if ( COM_write((BYTE*)send_address,18,pn->selected_connection) ) {
			LEVEL_DEBUG("Error with sending OWServer_Enet A-ddress\n") ;
			return OWServer_Enet_resync(pn) ;
		}
	} else {
		if ( COM_write((BYTE*)"M",1,pn->selected_connection) ) {
			LEVEL_DEBUG("Error with sending OWServer_Enet M-atch\n") ;
			return OWServer_Enet_resync(pn) ;
		}
	}
	if ( COM_read((BYTE*)resp_address,17,pn->selected_connection) ) {
		LEVEL_DEBUG("Error with reading OWServer_Enet select\n") ;
		return OWServer_Enet_resync(pn) ;
	}
	if ( memcmp( &resp_address[0],&send_address[1],17) ) {
		LEVEL_DEBUG("Error with OWServer_Enet select response\n") ;
		return OWServer_Enet_resync(pn) ;
	}

	// Set as current "Address" for adapter
	memcpy( pn->selected_connection->connin.enet.sn, pn->sn, 8) ;

	return 0 ;
}

//  Send data and return response block -- up to 32 bytes
static int OWServer_Enet_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	char send_data[1+2+32*2+1] ;
	char get_data[32*2+1] ;

	send_data[0] = 'W' ;
	num2string( &send_data[1], size ) ;
	bytes2string(&send_data[3], data, size) ;
	send_data[3+2*size] = 0x0D ;

	if ( COM_write((BYTE*)send_data,size*2+4,pn->selected_connection) ) {
		LEVEL_DEBUG("Error with sending OWServer_Enet block\n") ;
		return OWServer_Enet_resync(pn) ;
	}
	if ( COM_read((BYTE*)get_data,size*2+1,pn->selected_connection) ) {
		LEVEL_DEBUG("Error with reading OWServer_Enet block\n") ;
		return OWServer_Enet_resync(pn) ;
	}

	string2bytes(get_data, resp, size) ;
	return 0 ;
}

static int OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;

	for ( left=size ; left>0 ; left -= 32 ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>32)?32:left ;
		if ( OWServer_Enet_sendback_part( &data[pass_start], &resp[pass_start], pass_size, pn ) ) {
			return -EIO ;
		}
	}
	return 0;
}

/********************************************************/
/* OWServer_Enet_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void OWServer_Enet_close(struct connection_in *in)
{
	OWServer_Enet_powerdown(in) ;
	COM_close(in);

}

static void OWServer_Enet_powerdown(struct connection_in * in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	COM_write((BYTE*)"P", 1, in) ;
	if ( in->file_descriptor > -1 ) {
		COM_slurp(in->file_descriptor) ;
	}
}
