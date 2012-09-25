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

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

static void Server_setroutines(struct interface_routines *f);
static void Zero_setroutines(struct interface_routines *f);
static void Server_close(struct connection_in *in);

static void Server_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
    f->reset      = NO_RESET_ROUTINE;
    f->next_both  = NO_NEXT_BOTH_ROUTINE ;
    f->PowerByte     = NO_POWERBYTE_ROUTINE;
    f->ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
    f->sendback_data = NO_SENDBACKDATA_ROUTINE;
    f->sendback_bits = NO_SENDBACKBITS_ROUTINE;
    f->select        = NO_SELECT_ROUTINE;
    f->select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	f->reconnect = NO_RECONNECT_ROUTINE;
	f->close = Server_close;
	f->flags = 0 ;
}

static void Zero_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
    f->reset      = NO_RESET_ROUTINE;
    f->next_both  = NO_NEXT_BOTH_ROUTINE ;
    f->PowerByte     = NO_POWERBYTE_ROUTINE;
    f->ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
    f->sendback_data = NO_SENDBACKDATA_ROUTINE;
    f->sendback_bits = NO_SENDBACKBITS_ROUTINE;
    f->select        = NO_SELECT_ROUTINE;
    f->select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	f->reconnect = NO_RECONNECT_ROUTINE;
	f->close = Server_close;
	f->flags = 0 ;
}

// bus_zero is a server found by zeroconf/Bonjour
// It differs in that the server must respond
GOOD_OR_BAD Zero_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	if ( in==NO_CONNECTION ) {
		return gbBAD ;
	}
	pin->type = ct_tcp ;
	pin->state = cs_virgin ;
	pin->busmode = bus_zero;

	if (pin->init_data == NULL) {
		return gbBAD;
	}

	RETURN_BAD_IF_BAD( COM_open(in) ) ;
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	Zero_setroutines(&(in->iroutines));
	return gbGOOD;
}

// Set up inbound connection to an owserver
// Actual tcp connection created as needed
GOOD_OR_BAD Server_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	if (pin->init_data == NULL) {
		return gbBAD;
	}

	pin->type = ct_tcp ;
	pin->state = cs_virgin ;
	RETURN_BAD_IF_BAD( COM_open(in) ) ;
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	pin->busmode = bus_server;
	Server_setroutines(&(in->iroutines));
	return gbGOOD;
}

// Free up the owserver inbound connection
// actual connections opened and closed independently
static void Server_close(struct connection_in *in)
{
	SAFEFREE(in->master.tcp.type) ;
	SAFEFREE(in->master.tcp.domain) ;
	SAFEFREE(in->master.tcp.name) ;
}
