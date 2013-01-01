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

/* Routines for handling a linked list of connections in and out */
/* connections are to  */

/* Globals */
struct connection_in *head_inbound_list = NULL;
int count_inbound_connections = 0;

struct connection_in *find_connection_in(OWNET_HANDLE handle)
{
	struct connection_in *c_in;

	// step through head_inbound_list linked list
	for (c_in = head_inbound_list; c_in != NULL; c_in = c_in->next) {
		if (c_in->handle == handle) {
			return c_in;
		}
	}
	return NULL;
}

enum bus_mode get_busmode(struct connection_in *in)
{
	if (in == NULL)
		return bus_unknown;
	return in->busmode;
}

/* Make a new head_inbound_list, and place it in the chain */
struct connection_in *NewIn(void)
{
	struct connection_in *now;

	now = malloc(sizeof(struct connection_in));
	if (now != NULL) {
		memset(now, 0, sizeof(struct connection_in));
		
		now->next = head_inbound_list;	/* put in linked list at start */
		now->prior = NULL ;
		
		// new pointer to start of list
		head_inbound_list = now;
		
		// old start of list now points here
		if ( now->next ) {
			now->next->prior = now ;
		}
		
		now->handle = count_inbound_connections++;
		now->file_descriptor = FILE_DESCRIPTOR_BAD ;
		my_pthread_mutex_init(&(now->bus_mutex), Mutex.pmattr);
	}

	return now;
}

void FreeIn(struct connection_in *target)
{
	if (target == NULL) {
		return;
	}

	if (target->tcp.type) {
		free(target->tcp.type);
	}
		
	if (target->tcp.domain) {
		free(target->tcp.domain);
	}
	
	if (target->tcp.fqdn) {
		free(target->tcp.fqdn);
	}
	
	LEVEL_DEBUG("FreeClientAddr\n");
	FreeClientAddr(target);

	if (target->name) {
		free(target->name);
		target->name = NULL;
	}

	my_pthread_mutex_destroy(&(target->bus_mutex));

	// file descriptor
	if ( target->file_descriptor > FILE_DESCRIPTOR_BAD ) {
		close( target->file_descriptor ) ;
	}

	// Fix link of prior connection in list
	if ( target->prior ) {
		target->prior->next = target->next ;
	} else {
		head_inbound_list = target->next ;
	}
	
	// Fix link of next connection in list
	if ( target->next ) {
		target->next->prior = target->prior ;
	}
	
	free( target ) ;
}

void FreeInAll( void )
{
	while ( head_inbound_list ) {
		FreeIn(head_inbound_list);
	}
}

