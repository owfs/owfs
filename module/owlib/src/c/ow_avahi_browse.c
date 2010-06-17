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

#ifdef HAVE_STDLIB_H
#include "stdlib.h"  // need this for NULL
#endif

#if OW_ZERO

#if OW_CYGWIN
// not used for cygwin
#else

#include "ow_avahi.h"
#include "ow_connection.h"
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/* Prototypes */
static int AvahiBrowserNetName( const AvahiAddress *address, int port, struct connection_in * in ) ;
static void resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *address,
	uint16_t port,
	void *txt,
	AvahiLookupResultFlags flags,
	void* v) ;
static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* v) ;
static void client_callback(AvahiClient *c, AvahiClientState state, void * v) ;

/* Code */

/* Resolve callback -- a new owserver was found and resolved, add it to the list */
static void resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *address,
	uint16_t port,
	void *txt,
	AvahiLookupResultFlags flags,
	void* v)
{
	struct connection_in * in = v;

	(void) host_name ;
	(void) txt ;
	(void) flags ;

	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			LEVEL_DEBUG( "Failed to resolve service '%s' of type '%s' in domain '%s': %s", name, type, domain, avahi_strerror(avahi_client_errno(in->connin.browse.avahi_client)));
			break;

		case AVAHI_RESOLVER_FOUND:
			LEVEL_DEBUG( "Service '%s' of type '%s' in domain '%s'", name, type, domain);
			if ( AvahiBrowserNetName( address, port, in ) ) {
				break ;
			}
			ZeroAdd( name, type, domain, in->connin.browse.avahi_host, in->connin.browse.avahi_service ) ;
			break ;
	}

	avahi_service_resolver_free(r);
}

static int AvahiBrowserNetName( const AvahiAddress *address, int port, struct connection_in * in )
{
	memset(in->connin.browse.avahi_host,0,sizeof(in->connin.browse.avahi_host)) ;
	memset(in->connin.browse.avahi_service,0,sizeof(in->connin.browse.avahi_service)) ;
	UCLIBCLOCK ;
	snprintf(in->connin.browse.avahi_service, sizeof(in->connin.browse.avahi_service),"%d",port) ;
	UCLIBCUNLOCK ;
	switch (address->proto) {
		case AVAHI_PROTO_INET:     /**< IPv4 */
			if ( inet_ntop( AF_INET, (const void *)&(address->data.ipv4), in->connin.browse.avahi_host, sizeof(in->connin.browse.avahi_host)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d", in->connin.browse.avahi_host, port);
				return 0 ;
			}
			break ;
#if __HAS_IPV6__
		case AVAHI_PROTO_INET6:   /**< IPv6 */
			if ( inet_ntop( AF_INET6, (const void *)&(address->data.ipv6), in->connin.browse.avahi_host, sizeof(in->connin.browse.avahi_host)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d", in->connin.browse.avahi_host, port);
				return 0 ;
			}
			break ;
#endif
		default:
			strncpy(in->connin.browse.avahi_host,"Unknown address",sizeof(in->connin.browse.avahi_host)) ;
			LEVEL_DEBUG( "Unknown internet protocol");
			break ;
	}
	return 1 ;
}

static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* v)
{

	struct connection_in * in = v;

	in->connin.browse.avahi_browser = b ;

	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:

			LEVEL_DEBUG( "%s", avahi_strerror(avahi_client_errno(in->connin.browse.avahi_client)));
			avahi_simple_poll_quit(in->connin.browse.avahi_poll);
			return;

		case AVAHI_BROWSER_NEW:
			LEVEL_DEBUG( "NEW: service '%s' of type '%s' in domain '%s'", name, type, domain);

			/* We ignore the returned resolver object. In the callback
			function we free it. If the server is terminated before
			the callback function is called the server will free
			the resolver for us. */

			if (!(avahi_service_resolver_new(in->connin.browse.avahi_client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, in))) {
				LEVEL_DEBUG( "Failed to resolve service '%s': %s", name, avahi_strerror(avahi_client_errno(in->connin.browse.avahi_client)));
			}
			break;

		case AVAHI_BROWSER_REMOVE:
			LEVEL_DEBUG( "REMOVE: service '%s' of type '%s' in domain '%s'", name, type, domain);
			ZeroDel(name, type, domain ) ;
			break;

		case AVAHI_BROWSER_ALL_FOR_NOW:
			LEVEL_DEBUG( "ALL_FOR_NOW" );
			break;
			
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			LEVEL_DEBUG( "CACHE_EXHAUSTED" );
			break;
	}
}

static void client_callback(AvahiClient *c, AvahiClientState state, void * v)
{

	struct connection_in * in = v;
	in->connin.browse.avahi_client = c ;
	/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		LEVEL_DEBUG( "Server connection failure: %s", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(in->connin.browse.avahi_poll);
	}
}

void * OW_Avahi_Browse(void * v)
{
	struct connection_in * in = v ;
	int error;

#if OW_MT
	pthread_detach(pthread_self());
#endif

	/* Create main loop object */
	in->connin.browse.avahi_poll = avahi_simple_poll_new() ;

	/* Check whether creating the loop object succeeded */
	if (in->connin.browse.avahi_poll!=NULL) {
		
		/* Create a new client */
		in->connin.browse.avahi_client = avahi_client_new(avahi_simple_poll_get(in->connin.browse.avahi_poll), 0, client_callback, (void *) in, &error);

		/* Check whether creating the client object succeeded */
		if (in->connin.browse.avahi_client!=NULL) {
			
			/* Create a new browser */
			in->connin.browse.avahi_browser = avahi_service_browser_new(in->connin.browse.avahi_client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_owserver._tcp", NULL, 0, browse_callback, (void *) in);

			/* Check whether creating the browser object succeeded */
			if (in->connin.browse.avahi_browser!=NULL) {
				
				/* Run the main loop */
				avahi_simple_poll_loop(in->connin.browse.avahi_poll);
				
				/* Free the browser object */
				avahi_service_browser_free(in->connin.browse.avahi_browser);
				in->connin.browse.avahi_browser = NULL ;
			} else {
				LEVEL_DEBUG( "Failed to create service browser: %s", avahi_strerror(avahi_client_errno(in->connin.browse.avahi_client)));
			}
			/* Free the client object */
			avahi_client_free(in->connin.browse.avahi_client);
			in->connin.browse.avahi_client = NULL ;
		} else {
			LEVEL_DEBUG( "Failed to create client: %s", avahi_strerror(error));
		}
		/* Free the loop object */
		avahi_simple_poll_free(in->connin.browse.avahi_poll);
		in->connin.browse.avahi_poll = NULL ;
	} else {
		LEVEL_DEBUG( "Failed to create simple poll object.");
	}

	return NULL;
}

#endif /* OW_CYGWIN */

#else /* OW_ZERO */
void * OW_Avahi_Browse(void * v) {
	(void) v ;
	return NULL ;
}
#endif /* OW_ZERO */
