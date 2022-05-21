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

#if OW_AVAHI

#ifdef HAVE_STDLIB_H
#include "stdlib.h"  // need this for NULL
#endif /* HAVE_STDLIB_H */

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

/* Prototypes */
static int AvahiBrowserNetName( const AvahiAddress *address, int port, struct connection_in * in ) ;

static void resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char * name,
	const char * type,
	const char *domain,
	const char * host_name,
	const AvahiAddress * a,
	uint16_t port,
	AvahiStringList * txt,
	AvahiLookupResultFlags flags,
	void* v) ;

static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char * name,
	const char * type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* v) ;

static void client_callback(
	AvahiClient *c, 
	AvahiClientState state, 
	AVAHI_GCC_UNUSED void * v) ;

/* Code */

/* Resolve callback -- a new owserver was found and resolved, add it to the list */
static void resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char * name,
	const char * type,
	const char *domain,
	const char * host_name,
	const AvahiAddress * address,
	uint16_t port,
	AvahiStringList * txt,
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
			LEVEL_DEBUG( "Failed to resolve service '%s' of type '%s' in domain '%s': %s", name, type, domain, avahi_strerror(avahi_client_errno(in->master.browse.client)));
			break;

		case AVAHI_RESOLVER_FOUND:
			LEVEL_DEBUG( "Service '%s' of type '%s' in domain '%s'", name, type, domain);
			if ( AvahiBrowserNetName( address, port, in ) ) {
				break ;
			}
			ZeroAdd( name, type, domain, in->master.browse.host, in->master.browse.service ) ;
			break ;
	}

	avahi_service_resolver_free(r);
}

static int AvahiBrowserNetName( const AvahiAddress *address, int port, struct connection_in * in )
{
	memset(in->master.browse.host,0,sizeof(in->master.browse.host)) ;
	memset(in->master.browse.service,0,sizeof(in->master.browse.service)) ;
	UCLIBCLOCK ;
	snprintf(in->master.browse.service, sizeof(in->master.browse.service),"%d",port) ;
	UCLIBCUNLOCK ;
	switch (address->proto) {
		case AVAHI_PROTO_INET:     /**< IPv4 */
			if ( inet_ntop( AF_INET, (const void *)&(address->data.ipv4), in->master.browse.host, sizeof(in->master.browse.host)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d", in->master.browse.host, port);
				return 0 ;
			}
			break ;
#if __HAS_IPV6__
		case AVAHI_PROTO_INET6:   /**< IPv6 */
			if ( inet_ntop( AF_INET6, (const void *)&(address->data.ipv6), in->master.browse.host, sizeof(in->master.browse.host)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d", in->master.browse.avahi_host, port);
				return 0 ;
			}
			break ;
#endif /* __HAS_IPV6__ */
		default:
			strncpy(in->master.browse.host,"Unknown address",sizeof(in->master.browse.host)) ;
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
	const char * name,
	const char * type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* v)
{

	struct connection_in * in = v;

	in->master.browse.browser = b ;

	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:

			LEVEL_DEBUG( "%s", avahi_strerror(avahi_client_errno(in->master.browse.client)));
			avahi_threaded_poll_quit(in->master.browse.poll);
			break;

		case AVAHI_BROWSER_NEW:
			LEVEL_DEBUG( "NEW: service '%s' of type '%s' in domain '%s' protocol %d", name, type, domain, (int) protocol);

			/* We ignore the returned resolver object. In the callback
			function we free it. If the server is terminated before
			the callback function is called the server will free
			the resolver for us. */

			if (!(avahi_service_resolver_new(in->master.browse.client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, (void *) in))) {
				LEVEL_DEBUG( "Failed to resolve service '%s': %s", name, avahi_strerror(avahi_client_errno(in->master.browse.client)));
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

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * v)
{

	struct connection_in * in = v;
	in->master.browse.client = c ;
	/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		LEVEL_DEBUG( "Server connection failure: %s", avahi_strerror(avahi_client_errno(c)));
		avahi_threaded_poll_quit(in->master.browse.poll);
	}
}

GOOD_OR_BAD OW_Avahi_Browse( struct connection_in * in)
{
	int error ;

	in->master.browse.poll = avahi_threaded_poll_new() ;
	in->master.browse.browser = NULL ;
	in->master.browse.client = NULL ;

	if ( in->master.browse.poll == NULL ) {
		LEVEL_CONNECT("Could not create an Avahi object for service browsing");
		return gbBAD ;
	}

	/* Allocate a new client */
	LEVEL_DEBUG("Creating Avahi client");
	in->master.browse.client = avahi_client_new(avahi_threaded_poll_get(in->master.browse.poll), 0, client_callback, (void *) in, &error);
	if (in->master.browse.client == NULL) {
		LEVEL_CONNECT("Could not create an Avahi client for service browsing");
		return gbBAD ;
	}

	/* Allocate a new browser */
	LEVEL_DEBUG("Creating Avahi browser");
	in->master.browse.browser = avahi_service_browser_new(in->master.browse.client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_owserver._tcp", NULL, 0, browse_callback, (void *) in);
	if (in->master.browse.browser == NULL) {
		LEVEL_CONNECT("Could not create an Avahi browser for service browsing");
		return gbBAD ;
	}

	/* Run the main loop */
	LEVEL_DEBUG("Starting Avahi thread");
	if ( avahi_threaded_poll_start(in->master.browse.poll) < 0 ) {
		LEVEL_CONNECT("Could not start Avahi service discovery thread") ;
	}

	// done
	/* Free the browser object */
	avahi_service_browser_free(in->master.browse.browser);
	in->master.browse.browser = NULL ;

	avahi_threaded_poll_free(in->master.browse.poll);
	LEVEL_DEBUG("Freeing Avahi objects");
	
	in->master.browse.poll = NULL ;
	in->master.browse.browser = NULL ;
	in->master.browse.client = NULL ;

	return gbBAD ;
}

#endif  /* OW_AVAHI */
