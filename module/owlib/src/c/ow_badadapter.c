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
#include "ow_connection.h"

/* All the rest of the program sees is the BadAdapter_detect and the entry in iroutines */

static RESET_TYPE BadAdapter_reset(const struct parsedname *pn);
static GOOD_OR_BAD BadAdapter_sendback_bits(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn);
static void BadAdapter_close(struct connection_in *in);

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
GOOD_OR_BAD BadAdapter_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	pin->type = ct_none ;
	pin->file_descriptor = FILE_DESCRIPTOR_BAD ;
	in->Adapter = adapter_Bad;	/* OWFS assigned value */
	in->iroutines.reset = BadAdapter_reset;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = BadAdapter_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = BadAdapter_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "Bad Adapter";
	SAFEFREE( DEVICENAME(in) ) ;
	DEVICENAME(in) = owstrdup("None") ;
	pin->busmode = bus_bad ;
	return gbGOOD;
}

static RESET_TYPE BadAdapter_reset(const struct parsedname *pn)
{
	(void) pn;
	return BUS_RESET_ERROR;
}

static GOOD_OR_BAD BadAdapter_sendback_bits(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn)
{
	(void) pn;
	(void) data;
	(void) resp;
	(void) len;
	return gbBAD;
}

static void BadAdapter_close(struct connection_in *in)
{
	(void) in;
}
