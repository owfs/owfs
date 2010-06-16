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
//    f->reset         =;
//    f->next_both  ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
	f->reconnect = NULL;
	f->close = Server_close;
	f->flags = 0 ;
}

static void Zero_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
//    f->reset         =;
//    f->next_both  ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
	f->reconnect = NULL;
	f->close = Server_close;
	f->flags = 0 ;
}

// bus_zero is a server found by zeroconf/Bonjour
// It differs in that the server must respond
GOOD_OR_BAD Zero_detect(struct connection_in *in)
{
	in->busmode = bus_zero;
	in->file_descriptor = FILE_DESCRIPTOR_BAD;	// No persistent connection yet
	if (in->name == NULL) {
		return gbBAD;
	}
	RETURN_BAD_IF_BAD(ClientAddr(in->name, DEFAULT_SERVER_PORT, in)) ;
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	Zero_setroutines(&(in->iroutines));
	return gbGOOD;
}

// Set up inbound connection to an owserver
// Actual tcp connection created as needed
GOOD_OR_BAD Server_detect(struct connection_in *in)
{
	if (in->name == NULL) {
		return gbBAD;
	}
	RETURN_BAD_IF_BAD(ClientAddr(in->name, DEFAULT_SERVER_PORT, in)) ;
	in->file_descriptor = FILE_DESCRIPTOR_BAD;	// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_server;
	Server_setroutines(&(in->iroutines));
	return gbGOOD;
}

// Free up the owserver inbound connection
// actual connections opened and closed independently
static void Server_close(struct connection_in *in)
{
	Test_and_Close( &(in->file_descriptor) ) ;
	SAFEFREE(in->connin.tcp.type) ;
	SAFEFREE(in->connin.tcp.domain) ;
	SAFEFREE(in->connin.tcp.name) ;
	FreeClientAddr(in);
}
