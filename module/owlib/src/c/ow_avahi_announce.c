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

#if OW_AVAHI

#include "ow.h"
#include "ow_connection.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

static void entry_group_callback(AvahiEntryGroup *group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void * v) ;
static void create_services(struct connection_out * out) ;
static void client_callback(AvahiClient *client, AvahiClientState state, AVAHI_GCC_UNUSED void * v) ;

static void entry_group_callback(AvahiEntryGroup *group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void * v)
{
	struct connection_out * out = v ;

	out->group = group ;
	/* Called whenever the entry group state changes */
	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			break;

		case AVAHI_ENTRY_GROUP_COLLISION :
			// Shouldn't happen since we put the pid in the service name
			LEVEL_DEBUG("inexplicable service name collison");
			avahi_threaded_poll_quit(out->poll);
			break;

		case AVAHI_ENTRY_GROUP_FAILURE :
			LEVEL_DEBUG("group failure: %s", avahi_strerror(avahi_client_errno(out->client)));
			avahi_threaded_poll_quit(out->poll);
			break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			break ;
	}
}

static void create_services(struct connection_out * out)
{
	/* If this is the first time we're called, let's create a new
	* entry group if necessary */
	if (out->group==NULL) {
		out->group = avahi_entry_group_new(out->client, entry_group_callback, out) ;
		if ( out->group==NULL) {
			LEVEL_DEBUG("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(out->client)));
			avahi_threaded_poll_quit(out->poll);
			return ;
		}
	}

	/* If the group is empty (either because it was just created, or
	* because it was reset previously, add our entries.  */
	if (avahi_entry_group_is_empty(out->group)) {
		struct sockaddr sa;
		//socklen_t sl = sizeof(sa);
		socklen_t sl = 128;
		int ret1 = 0 ;
		int ret2 = 0 ;
		uint16_t port ;
		char * service_name ;
		char name[63] ;
		if (getsockname(out->file_descriptor, &sa, &sl)) {
			LEVEL_CONNECT("Could not get port number of device.");
			avahi_threaded_poll_quit(out->poll);
			return;
		}
		port = ntohs(((struct sockaddr_in *) (&sa))->sin_port) ;
		/* Add the service */
		switch (Globals.program_type) {
			case program_type_httpd:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Web" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( out->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_http._tcp", NULL, NULL, port, NULL) ;
				ret2 = avahi_entry_group_add_service( out->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_owhttpd._tcp", NULL, NULL, port, NULL) ;
				break ;
			case program_type_server:
			case program_type_external:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Server" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( out->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_owserver._tcp", NULL, NULL, port, NULL) ;
				break;
			case program_type_ftpd:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) FTP" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( out->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_ftp._tcp", NULL, NULL, port, NULL) ;
				break;
			default:
				LEVEL_DEBUG("This program type doesn't support service announcement");
				avahi_threaded_poll_quit(out->poll);
				return;
		}

		if ( ret1 < 0 || ret2 < 0 ) {
			LEVEL_DEBUG("Failed to add a service: %s or %s", avahi_strerror(ret1), avahi_strerror(ret2));
			avahi_threaded_poll_quit(out->poll);
			return;
		}

		/* Tell the server to register the service */
		ret1 = avahi_entry_group_commit(out->group) ;
		if ( ret1 < 0 ) {
			LEVEL_DEBUG("Failed to commit entry group: %s", avahi_strerror(ret1));
			avahi_threaded_poll_quit(out->poll);
			return;
		}
		/* Record the service to prevent browsing to ourself */
		SAFEFREE( out->zero.name ) ;
		out->zero.name = owstrdup(name) ;

		SAFEFREE( out->zero.type ) ;
		switch (Globals.program_type) {
			case program_type_httpd:
				out->zero.type = owstrdup("_owhttpd._tcp") ;
				break ;
			case program_type_server:
				out->zero.type = owstrdup("_owserver._tcp") ;
				break;
			case program_type_external:
				out->zero.type = owstrdup("_owexternal._tcp") ;
				break;
			case program_type_ftpd:
				out->zero.type = owstrdup("_ftp._tcp") ;
				break;
			default:
				break ;
		}

		SAFEFREE( out->zero.domain ) ;
		out->zero.domain = owstrdup(avahi_client_get_domain_name(out->client)) ;
	}
}

static void client_callback(AvahiClient *client, AvahiClientState state, AVAHI_GCC_UNUSED void * v)
{
	struct connection_out * out = v ;
	if ( client==NULL ) {
		LEVEL_DEBUG("Avahi client is null") ;
		avahi_threaded_poll_quit(out->poll);
		return ;
	}

	//Set this link here so called routines can use it
	// (sets it earlier than return code does)
	out->client = client ;

	/* Called whenever the client or server state changes */
	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:

			/* The server has startup successfully and registered its host
			* name on the network, so it's time to create our services */
			create_services(out);
			break;

		case AVAHI_CLIENT_FAILURE:

			LEVEL_DEBUG( "Avahi client failure: %s", avahi_strerror(avahi_client_errno(client)));
			avahi_threaded_poll_quit(out->poll);
			break;

		case AVAHI_CLIENT_S_COLLISION:
			/* Let's drop our registered services. When the server is back
			* in AVAHI_SERVER_RUNNING state we will register them
			* again with the new host name. */

		case AVAHI_CLIENT_S_REGISTERING:
			/* The server records are now being established. This
			* might be caused by a host name change. We need to wait
			* for our own records to register until the host name is
			* properly esatblished. */
			if (out->group != NULL) {
				avahi_entry_group_reset(out->group);
			}
			break;

		case AVAHI_CLIENT_CONNECTING:
			break ;
	}
}

// Now uses Avahi threads
GOOD_OR_BAD OW_Avahi_Announce( struct connection_out *out )
{
	int error ;

	out->poll = avahi_threaded_poll_new() ;
	out->client = NULL ;
	out->group = NULL ;

	if ( out->poll == NULL ) {
		LEVEL_CONNECT("Could not create an Avahi object for service announcement");
		return gbBAD ;
	}

	/* Allocate a new client */
	LEVEL_DEBUG("Creating Avahi client");
	out->client = avahi_client_new(avahi_threaded_poll_get(out->poll), 0, client_callback, (void *) out, &error);
	if (out->client == NULL) {
		LEVEL_CONNECT("Could not create an Avahi client for service announcement");
		return gbBAD ;
	}

	/* Run the main loop */
	LEVEL_DEBUG("Starting Avahi thread");
	if ( avahi_threaded_poll_start(out->poll) < 0 ) {
		LEVEL_CONNECT("Could not start Avahi service discovery thread") ;
	}

	// done
	avahi_threaded_poll_free(out->poll);
	LEVEL_DEBUG("Freeing Avahi objects");
	
	out->poll = NULL ;
	out->client = NULL ;
	out->group = NULL ;

	return gbBAD ;
}

#endif /* OW_AVAHI */
