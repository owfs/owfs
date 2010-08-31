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

/* not our owserver, but a stand-alone device made by EDS called OW-SERVER-ENET */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"
#include "ow_specialcase.h"

struct toENET {
	ASCII *command;
	ASCII address[16];
	const BYTE *data;
	size_t length;
};

//static void byteprint( const BYTE * b, int size ) ;
static RESET_TYPE OWServer_Enet_reset(const struct parsedname *pn);
static void OWServer_Enet_close(struct connection_in *in);

static enum search_status OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void OWServer_Enet_setroutines(struct connection_in *in);
static GOOD_OR_BAD OWServer_Enet_select( const struct parsedname * pn ) ;

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
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2409path | ADAP_FLAG_presence_from_dirblob ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

GOOD_OR_BAD OWServer_Enet_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	OWServer_Enet_setroutines(in);

	if (in->name == NULL) {
		return gbBAD;
	}

	RETURN_BAD_IF_BAD(ClientAddr(in->name, DEFAULT_ENET_PORT, in)) ;

	in->Adapter = adapter_ENET;
	in->adapter_name = "OWServer_Enet";
	in->busmode = bus_enet;

	RETURN_BAD_IF_BAD(ENET_get_detail(&pn)) ;

	return gbGOOD;
}

static RESET_TYPE OWServer_Enet_reset(const struct parsedname *pn)
{
	(void) pn ;
	return BUS_RESET_OK;
}

static enum search_status OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct dirblob *db = &(pn->selected_connection->main) ;

	if ( ds->search == _1W_CONDITIONAL_SEARCH_ROM ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(ENET_get_detail(pn)) ) {
			return search_error;
		}
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, db) ) {
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

/* select a device for reference */
/* Don't do much in this case */
static GOOD_OR_BAD OWServer_Enet_select( const struct parsedname * pn )
{
	// Set as current "Address" for adapter
	memcpy( pn->selected_connection->master.enet.sn, pn->sn, SERIAL_NUMBER_SIZE) ;

	return gbGOOD ;
}

static GOOD_OR_BAD OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	(void) data;
	(void) resp;
	(void) size;
	(void) pn ;
	return gbGOOD;
}

static void OWServer_Enet_close(struct connection_in *in)
{
	FreeClientAddr(in);
}

