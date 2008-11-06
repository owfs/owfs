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


//#include <arpa/inet.h>

#include <config.h>
#include "owfs_config.h"

#if OW_ZERO

#include "ow_avahi.h"
#include "ow_connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct browse_avahi_struct {
	AvahiServiceBrowser *browser ;
	AvahiSimplePoll *poll ;
	AvahiClient *client ;
} ;

static void AvahiBrowserNetName( const AvahiAddress *address, int port ) ;
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
void * OW_Avahi_Browse(void * v) ;

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
	struct browse_avahi_struct * bas = v;
	
	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			LEVEL_DEBUG( "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(bas->client)));
			break;

		case AVAHI_RESOLVER_FOUND:
			LEVEL_DEBUG( "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);
			AvahiBrowserNetName( address, port ) ;
			break ;
	}

	avahi_service_resolver_free(r);
}

static void AvahiBrowserNetName( const AvahiAddress *address, int port )
{
	switch (address->proto) {
		case AVAHI_PROTO_INET:     /**< IPv4 */
		{
			char name[INET_ADDRSTRLEN+1] ;
			memset(name,0,sizeof(name)) ;
			if ( inet_ntop( AF_INET, (void *)&(address->data.ipv4), name, sizeof(name)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d:\n", name, port);
			}
		}
			break ;
		case AVAHI_PROTO_INET6:   /**< IPv6 */
		{
			char name[INET6_ADDRSTRLEN+1] ;
			memset(name,0,sizeof(name)) ;
			if ( inet_ntop( AF_INET6, (void *)&(address->data.ipv6), name, sizeof(name)) != NULL ) {
				LEVEL_DEBUG( "Address '%s' Port %d:\n", name, port);
			}
			break ;
	}
}

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
	
	struct browse_avahi_struct * bas = v;
	
	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:

			LEVEL_DEBUG( "(Browser) %s\n", avahi_strerror(avahi_client_errno(bas->client)));
			avahi_simple_poll_quit(bas->poll);
			return;

		case AVAHI_BROWSER_NEW:
			LEVEL_DEBUG( "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

			/* We ignore the returned resolver object. In the callback
			function we free it. If the server is terminated before
			the callback function is called the server will free
			the resolver for us. */

		if (!(avahi_service_resolver_new(bas->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, bas)))
			LEVEL_DEBUG( "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(bas->client)));

		break;

		case AVAHI_BROWSER_REMOVE:
			LEVEL_DEBUG( "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
			break;

		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			LEVEL_DEBUG( "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
			break;
	}
}

static void client_callback(AvahiClient *c, AvahiClientState state, void * v)
{
	
	struct browse_avahi_struct * bas = v;
	bas->client = c ;
	/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		LEVEL_DEBUG( "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(bas->poll);
	}
}

void * OW_Avahi_Browse(void * v)
{
	struct browse_avahi_struct bas ;
	int error;
	
	/* Allocate main loop object */
	bas.poll = avahi_simple_poll_new() ;
	if (bas.poll==NULL) {
		LEVEL_DEBUG( "Failed to create simple poll object.\n");
		return NULL ;
	}
    
	/* Allocate a new client */
	bas.client = avahi_client_new(avahi_simple_poll_get(bas.poll), 0, client_callback, &bas, &error);
    
	/* Check wether creating the client object succeeded */
	if (bas.client==NULL) {
		LEVEL_DEBUG( "Failed to create client: %s\n", avahi_strerror(error));
		avahi_simple_poll_free(bas.poll);
		return NULL;
	}
    
	bas.browser = avahi_service_browser_new(bas.client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_owserver._tcp", NULL, 0, browse_callback, &bas);
	/* Create the service browser */
	if (bas.browser==NULL) {
		LEVEL_DEBUG( "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(bas.client)));
		avahi_simple_poll_free(bas.poll);
		avahi_client_free(bas.client);
		return NULL;
	}
    
	/* Run the main loop */
	avahi_simple_poll_loop(bas.poll);
       
	/* Cleanup things */
	avahi_service_browser_free(bas.browser);
	avahi_simple_poll_free(bas.poll);
	avahi_client_free(bas.client);
    
	return NULL;
}
#else /* OW_ZERO */
void * OW_Avahi_Browse(void * v) {
	return NULL ;
}
#endif /* OW_ZERO */
