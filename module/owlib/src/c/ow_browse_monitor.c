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

static void Browse_close(struct connection_in *in);
static GOOD_OR_BAD browse_in_use(const struct connection_in * in_selected) ;

/* Device-specific functions */
GOOD_OR_BAD Browse_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	in->iroutines.detect = Browse_detect;
	in->Adapter = adapter_browse_monitor;	/* OWFS assigned value */
	in->iroutines.reset = NO_RESET_ROUTINE;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = Browse_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "ZeroConf monitor";
	pin->busmode = bus_browse ;
	
	RETURN_BAD_IF_BAD( browse_in_use(in) ) ;

	in->master.browse.bonjour_browse = 0 ;

#if OW_AVAHI	
	in->master.browse.browser = NULL ;
	in->master.browse.poll    = NULL ;
	in->master.browse.client  = NULL ;
#endif /* OW_AVAHI */

	if (Globals.zero == zero_none ) {
		LEVEL_DEFAULT("Zeroconf/Bonjour is disabled since Bonjour or Avahi library wasn't found.");
		return gbBAD;
	} else {
		OW_Browse(in);
	}
	return gbGOOD ;
}

static GOOD_OR_BAD browse_in_use(const struct connection_in * in_selected)
{
	struct port_in * pin ;
	
	for ( pin = Inbound_Control.head_port ; pin ; pin = pin->next ) {
		struct connection_in *cin;

		if ( pin->busmode != bus_browse ) {
			continue ;
		}
		for (cin = pin->first; cin != NO_CONNECTION; cin = cin->next) {
			if ( cin == in_selected ) {
				continue ;
			}
			return gbBAD ;
		}
	}
	return gbGOOD;					// not found in the current inbound list
}

static void Browse_close(struct connection_in *in)
{
#if OW_ZERO
	if (in->master.browse.bonjour_browse && (libdnssd != NULL)) {
		DNSServiceRefDeallocate(in->master.browse.bonjour_browse);
		in->master.browse.bonjour_browse = 0 ;
	}
#if OW_AVAHI	
	if ( in->master.browse.poll != NULL ) {
		// Signal avahi loop to quit (in ow_avahi_browse.c)
		// and clean up for itself
		avahi_threaded_poll_quit(in->master.browse.poll);
	}
#endif /* OW_AVAHI */
#else /* OW_ZERO */
	(void) in ;
#endif /* OW_ZERO */
}
