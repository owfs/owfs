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

#include "ow_w1.h"

static void W1_monitor_close(struct connection_in *in);
static GOOD_OR_BAD w1_monitor_in_use(const struct connection_in * in_selected) ;

/* Device-specific functions */
GOOD_OR_BAD W1_monitor_detect(struct connection_in *in)
{
	struct timeval tvslack = { 1, 0 } ; // 1 second
	
	SOC(in)->file_descriptor = FILE_DESCRIPTOR_BAD;
	SOC(in)->type = ct_none ;
	in->iroutines.detect = W1_monitor_detect;
	in->Adapter = adapter_w1_monitor;	/* OWFS assigned value */
	in->iroutines.reset = NO_RESET_ROUTINE;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = W1_monitor_close;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "W1 monitor";
	in->busmode = bus_w1_monitor ;
	
	RETURN_BAD_IF_BAD( w1_monitor_in_use(in) ) ;
	
	// Initial setup
	Inbound_Control.w1_monitor = in ; // essentially a global pointer to the w1_monitor entry
	_MUTEX_INIT(in->master.w1_monitor.seq_mutex);
	_MUTEX_INIT(in->master.w1_monitor.read_mutex);

	timernow( &(in->master.w1_monitor.last_read) );
	timeradd( &(in->master.w1_monitor.last_read), &tvslack, &(in->master.w1_monitor.last_read) );

	in->master.w1_monitor.seq = SEQ_INIT ;
	in->master.w1_monitor.pid = 0 ;
	
	w1_bind(in) ; // sets in->file_descriptor
	if ( FILE_DESCRIPTOR_NOT_VALID( SOC(in)->file_descriptor ) ) {
		ERROR_DEBUG("Netlink problem -- are you root?");
		Inbound_Control.w1_monitor = NO_CONNECTION ;
		return gbBAD ;
	}
	
	return W1_Browse() ; // creates thread that runs forever.
}

// is there already a W! monitor in the Inbound list? You only need one.
static GOOD_OR_BAD w1_monitor_in_use(const struct connection_in * in_selected)
{
	struct connection_in *in;

	for (in = Inbound_Control.head; in != NO_CONNECTION; in = in->next) {
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
	_MUTEX_DESTROY(in->master.w1_monitor.seq_mutex);
	_MUTEX_DESTROY(in->master.w1_monitor.read_mutex);
}

#else /* OW_W1 && OW_MT */

GOOD_OR_BAD W1_monitor_detect(struct connection_in *in)
{
	(void) in ;
	LEVEL_DEFAULT("W1 (the linux kernel 1-wire system) is not supported");
	return gbBAD ;
}

#endif /* OW_W1 && OW_MT */
