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
#include "ow_connection.h"

#if OW_W1 && OW_MT

static void W1_monitor_close(struct connection_in *in);
static GOOD_OR_BAD w1_monitor_in_use(const struct connection_in * in_selected) ;

/* Device-specific functions */
GOOD_OR_BAD W1_monitor_detect(struct connection_in *in)
{
	in->file_descriptor = FILE_DESCRIPTOR_BAD;
	in->iroutines.detect = W1_monitor_detect;
	in->Adapter = adapter_w1_monitor;	/* OWFS assigned value */
	in->iroutines.reset = NULL;
	in->iroutines.next_both = NULL;
	in->iroutines.PowerByte = NULL;
	in->iroutines.ProgramPulse = NULL;
	in->iroutines.sendback_data = NULL;
	in->iroutines.sendback_bits = NULL;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = W1_monitor_close;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "W1 monitor";
	in->busmode = bus_w1_monitor ;
	
	RETURN_BAD_IF_BAD( w1_monitor_in_use(in) ) ;
	
	in->master.w1_monitor.seq = SEQ_INIT ;
	in->master.w1_monitor.pid = 0 ;
	
	if ( FILE_DESCRIPTOR_NOT_VALID(w1_bind()) ) {
		ERROR_DEBUG("Netlink problem -- are you root?");
		return gbBAD ;
	}
	
	Inbound_Control.w1_monitor = in ; // essentially a global pointer to the w1_monitor entry

	return W1_Browse() ; // creates thread that runs forever.
}

// is there already a W! monitor in the Inbound list? You only need one.
static GOOD_OR_BAD w1_monitor_in_use(const struct connection_in * in_selected)
{
	struct connection_in *in;

	for (in = Inbound_Control.head; in != NULL; in = in->next) {
		if ( in == in_selected ) {
			continue ;
		}
		if ( in->busmode != bus_w1_monitor ) {
			continue ;
		}
		return gbBAD ;
	}
	return gbGOOD;					// not found in the current inbound list
}

static void W1_monitor_close(struct connection_in *in)
{
	(void) in;
    Test_and_Close( &(Inbound_Control.w1_file_descriptor) );
	_MUTEX_DESTROY(Inbound_Control.w1_mutex);
	_MUTEX_DESTROY(Inbound_Control.w1_read_mutex);
}

#else /* OW_W1 && OW_MT */

GOOD_OR_BAD W1_monitor_detect(struct connection_in *in)
{
	(void) in ;
	LEVEL_DEFAULT("W1 (the linux kernel 1-wire system) is not supported");
	return gbBAD ;
}

#endif /* OW_W1 && OW_MT */
