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
#include "ow_connection.h"

#if OW_ZERO && OW_MT

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/* Special functions that need to be linked in dynamically
-- libavahi-client.so
avahi_client_errno
avahi_client_free
avahi_client_new
avahi_entry_group_add_service
avahi_entry_group_commit
avahi_entry_group_is_empty
avahi_entry_group_new
avahi_entry_group_reset
--libavahi-common.so
avahi_simple_poll_free
avahi_simple_poll_get
avahi_simple_poll_loop
avahi_simple_poll_new
avahi_simple_poll_quit
avahi_strerror
*/


struct announce_avahi_struct {
	AvahiEntryGroup *group ;
	AvahiSimplePoll *poll ;
	AvahiClient *client ;
	struct connection_out *out ;
} ;

static void entry_group_callback(AvahiEntryGroup *group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void * v) ;
static void create_services(struct announce_avahi_struct * aas) ;
static void client_callback(AvahiClient *client, AvahiClientState state, AVAHI_GCC_UNUSED void * v) ;

static void entry_group_callback(AvahiEntryGroup *group, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void * v)
{
	struct announce_avahi_struct * aas = v ;

	aas->group = group ;
	/* Called whenever the entry group state changes */
	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			break;

		case AVAHI_ENTRY_GROUP_COLLISION :
			// Shouldn't happen since we put the pid in the service name
			LEVEL_DEBUG("inexplicable service name collison");
			avahi_simple_poll_quit(aas->poll);
			break;

		case AVAHI_ENTRY_GROUP_FAILURE :
			LEVEL_DEBUG("group failure: %s", avahi_strerror(avahi_client_errno(aas->client)));
			avahi_simple_poll_quit(aas->poll);
			break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			break ;
	}
}

static void create_services(struct announce_avahi_struct * aas)
{
	/* If this is the first time we're called, let's create a new
	* entry group if necessary */
	if (aas->group==NULL) {
		aas->group = avahi_entry_group_new(aas->client, entry_group_callback, aas) ;
		if ( aas->group==NULL) {
			LEVEL_DEBUG("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(aas->client)));
			avahi_simple_poll_quit(aas->poll);
			return ;
		}
	}

	/* If the group is empty (either because it was just created, or
	* because it was reset previously, add our entries.  */
	if (avahi_entry_group_is_empty(aas->group)) {
		struct sockaddr sa;
		//socklen_t sl = sizeof(sa);
		socklen_t sl = 128;
		int ret1 = 0 ;
		int ret2 = 0 ;
		uint16_t port ;
		char * service_name ;
		char name[63] ;
		if (getsockname(aas->out->file_descriptor, &sa, &sl)) {
			LEVEL_CONNECT("Could not get port number of device.");
			avahi_simple_poll_quit(aas->poll);
			return;
		}
		port = ntohs(((struct sockaddr_in *) (&sa))->sin_port) ;
		/* Add the service */
		switch (Globals.opt) {
			case opt_httpd:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Web" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( aas->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_http._tcp", NULL, NULL, port, NULL) ;
				ret2 = avahi_entry_group_add_service( aas->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_owhttpd._tcp", NULL, NULL, port, NULL) ;
				break ;
			case opt_server:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Server" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( aas->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_owserver._tcp", NULL, NULL, port, NULL) ;
				break;
			case opt_ftpd:
				service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) FTP" ;
				UCLIBCLOCK;
				snprintf(name,62,"%s <%d>",service_name,(int)port);
				UCLIBCUNLOCK;
				ret1 = avahi_entry_group_add_service( aas->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name,"_ftp._tcp", NULL, NULL, port, NULL) ;
				break;
			default:
				avahi_simple_poll_quit(aas->poll);
				return;
		}
		//printf("AVAHI ANNOUNCE: domain=<%s>\n",avahi_client_get_domain_name(aas->client) ) ;
		if ( ret1 < 0 || ret2 < 0 ) {
			LEVEL_DEBUG("Failed to add a service: %s or %s", avahi_strerror(ret1), avahi_strerror(ret2));
			avahi_simple_poll_quit(aas->poll);
			return;
		}

		/* Tell the server to register the service */
		ret1 = avahi_entry_group_commit(aas->group) ;
		if ( ret1 < 0 ) {
			LEVEL_DEBUG("Failed to commit entry group: %s", avahi_strerror(ret1));
			avahi_simple_poll_quit(aas->poll);
			return;
		}
		/* Record the service to prevent browsing to ourself */
		SAFEFREE( aas->out->zero.name ) ;
		aas->out->zero.name = owstrdup(name) ;

		SAFEFREE( aas->out->zero.type ) ;
		switch (Globals.opt) {
			case opt_httpd:
				aas->out->zero.type = owstrdup("_owhttpd._tcp") ;
				break ;
			case opt_server:
				aas->out->zero.type = owstrdup("_owserver._tcp") ;
				break;
			case opt_ftpd:
				aas->out->zero.type = owstrdup("_ftp._tcp") ;
				break;
			default:
				break ;
		}

		SAFEFREE( aas->out->zero.domain ) ;
		aas->out->zero.domain = owstrdup(avahi_client_get_domain_name(aas->client)) ;
	}
}

static void client_callback(AvahiClient *client, AvahiClientState state, AVAHI_GCC_UNUSED void * v)
{
	struct announce_avahi_struct * aas = v ;
	if ( client==NULL ) {
		LEVEL_DEBUG("null client") ;
		avahi_simple_poll_quit(aas->poll);
		return ;
	}

	//Set this link here so called routines can use it
	aas->client = client ;

	/* Called whenever the client or server state changes */
	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:

			/* The server has startup successfully and registered its host
			* name on the network, so it's time to create our services */
		create_services(aas);
		break;

		case AVAHI_CLIENT_FAILURE:

			LEVEL_DEBUG( "Client failure: %s", avahi_strerror(avahi_client_errno(client)));
			avahi_simple_poll_quit(aas->poll);

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
			if (aas->group != NULL) {
				avahi_entry_group_reset(aas->group);
			}

			break;

		case AVAHI_CLIENT_CONNECTING:
			break ;
	}
}

// Should be called in it's own thread
void *OW_Avahi_Announce( void * v )
{
	struct connection_out *out = v ;
	struct announce_avahi_struct aas = {
		.group = NULL,
		.poll = NULL,
		.client = NULL,
		.out = out ,
	} ;
	DETACH_THREAD;

	aas.poll = avahi_simple_poll_new() ;
	if ( aas.poll != NULL ) {
		int error ;
		/* Allocate a new client */
		aas.client = avahi_client_new(avahi_simple_poll_get(aas.poll), 0, client_callback, (void *)(&aas), &error);
		if (aas.client!=NULL) {
			/* Run the main loop */
			avahi_simple_poll_loop(aas.poll);
			// done
			avahi_client_free(aas.client);
		} else {
			LEVEL_CONNECT("Failed to create client: %s", avahi_strerror(error)) ;
		}

		avahi_simple_poll_free(aas.poll);
	} else {
		LEVEL_CONNECT("Failed to create simple poll object.");
	}

	LEVEL_DEBUG("Normal exit.");
	pthread_exit(NULL);
	return VOID_RETURN ;
}

#else /* OW_ZERO && OW_MT */

// Should be called in it's own thread
void *OW_Avahi_Announce( void * v )
{
	(void) v ;
	LEVEL_CONNECT("Avahi support not present in this build");
	return VOID_RETURN ;
}

#endif /* OW_ZERO && OW_MT */
