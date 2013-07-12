/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include "owshell.h"

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * owserver_connection ...         */

struct global Globals;

/* Globals */
int count_inbound_connections = 0;
struct connection_in s_owserver_connection;
struct connection_in *owserver_connection = &s_owserver_connection;


/* All ow library setup */
void Setup(void)
{
#if OW_ZERO
	OW_Load_dnssd_library();
#endif
	memset(owserver_connection, 0, sizeof(s_owserver_connection));

	// global structure of configuration parameters
	memset(&Globals, 0, sizeof(struct global));
	Globals.timeout_network = 1;
	errno = 0;					/* set error level none */
}

static void Cleanup(void)
{
#if OW_ZERO
	OW_Free_dnssd_library();
#endif
}

void Exit(int exit_code)
{
	Cleanup();
	exit(exit_code);
}
