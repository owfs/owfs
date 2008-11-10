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

// Some of this code taken from the Avahi project (legally)
// The following header is in those header files:
/***
This file is part of avahi.

avahi is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

avahi is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with avahi; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA.
***/
// Note that although these headers are LGPL v2.1+, the rest of OWFS is GPL v2

#ifndef OW_AVAHI_H
#define OW_AVAHI_H

#include <stdint.h>
#include <sys/time.h>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define AVAHI_GCC_SENTINEL __attribute__ ((sentinel))
#else
/** Macro for usage of GCC's sentinel compilation warnings */
#define AVAHI_GCC_SENTINEL
#endif
#ifdef __GNUC__
#define AVAHI_GCC_UNUSED __attribute__ ((unused))
#else
/** Macro for not used parameter */
#define AVAHI_GCC_UNUSED
#endif


typedef enum { EnumDummy, } Enum ;

struct AvahiClient ;

typedef struct AvahiClient AvahiClient;

/** States of a server object */
typedef enum {
	AVAHI_SERVER_INVALID,
	AVAHI_SERVER_REGISTERING,
	AVAHI_SERVER_RUNNING,
	AVAHI_SERVER_COLLISION,
	AVAHI_SERVER_FAILURE
} AvahiServerState;

/** States of an entry group object */
typedef enum {
	AVAHI_ENTRY_GROUP_UNCOMMITED,
	AVAHI_ENTRY_GROUP_REGISTERING,
	AVAHI_ENTRY_GROUP_ESTABLISHED,
	AVAHI_ENTRY_GROUP_COLLISION,
	AVAHI_ENTRY_GROUP_FAILURE
} AvahiEntryGroupState;

/** States of a client object, a superset of AvahiServerState */
typedef enum {
	AVAHI_CLIENT_S_REGISTERING = AVAHI_SERVER_REGISTERING,
	AVAHI_CLIENT_S_RUNNING = AVAHI_SERVER_RUNNING,
	AVAHI_CLIENT_S_COLLISION = AVAHI_SERVER_COLLISION,
	AVAHI_CLIENT_FAILURE = 100,
	AVAHI_CLIENT_CONNECTING = 101
} AvahiClientState;

typedef enum {
	AVAHI_PUBLISH_UNIQUE = 1,
    AVAHI_PUBLISH_NO_PROBE = 2,
    AVAHI_PUBLISH_NO_ANNOUNCE = 4,
    AVAHI_PUBLISH_ALLOW_MULTIPLE = 8,
/** \cond fulldocs */
    AVAHI_PUBLISH_NO_REVERSE = 16,
    AVAHI_PUBLISH_NO_COOKIE = 32,
/** \endcond */
    AVAHI_PUBLISH_UPDATE = 64,
/** \cond fulldocs */
    AVAHI_PUBLISH_USE_WIDE_AREA = 128,
    AVAHI_PUBLISH_USE_MULTICAST = 256
/** \endcond */
} AvahiPublishFlags;

typedef enum {
	AVAHI_CLIENT_IGNORE_USER_CONFIG = 1,
	AVAHI_CLIENT_NO_FAIL = 2
} AvahiClientFlags;

/** An I/O watch object */
typedef struct AvahiWatch AvahiWatch;

/** The function prototype for the callback of an AvahiClient */
typedef void (*AvahiClientCallback) (
	AvahiClient *s,
	AvahiClientState state /**< The new state of the client */,
	void* userdata /**< The user data that was passed to avahi_client_new() */
);

/** Called when the timeout is reached */
typedef void (*AvahiTimeoutCallback)(void *, void *);

/** Called whenever an I/O event happens  on an I/O watch */
typedef void (*AvahiWatchCallback)(void *, int, Enum, void *);

typedef struct {
	void* userdata;
	void* (*watch_new)(const void *, int, Enum, AvahiWatchCallback, void *);
	void (*watch_update)(void *, Enum);
	Enum (*watch_get_events)(void *);
	void (*watch_free)(void *);
	void* (*timeout_new)(const void *, const struct timeval *, AvahiTimeoutCallback callback, void *);
	void (*timeout_update)(void *, const struct timeval *tv);
	void (*timeout_free)(void *t);
} AvahiPoll;

/** Creates a new client instance */
AvahiClient* (*avahi_client_new) (
	const AvahiPoll *poll_api,
	AvahiClientFlags flags,
	AvahiClientCallback callback,
	void *userdata,
	int *error
);
			       			       
void (*avahi_client_free)(AvahiClient *client);

int (*avahi_client_errno) (AvahiClient*);

typedef struct AvahiEntryGroup AvahiEntryGroup;

typedef void (*AvahiEntryGroupCallback) (
	AvahiEntryGroup *g,
	AvahiEntryGroupState state,
	void* userdata
);

struct AvahiEntryGroup {
	char *path;
	AvahiEntryGroupState state;
	int state_valid;
	AvahiClient *client;
	AvahiEntryGroupCallback callback;
	void *userdata;
	AvahiEntryGroup* groups;
};

AvahiEntryGroup* (*avahi_entry_group_new) (
	AvahiClient* c,
	AvahiEntryGroupCallback callback,
	void *userdata
);

int (*avahi_entry_group_commit) (AvahiEntryGroup*);

int (*avahi_entry_group_reset) (AvahiEntryGroup*);

int (*avahi_entry_group_is_empty) (AvahiEntryGroup*);

typedef enum {
	AVAHI_PROTO_INET = 0,     /**< IPv4 */
	AVAHI_PROTO_INET6 = 1,   /**< IPv6 */
	AVAHI_PROTO_UNSPEC = -1  /**< Unspecified/all protocol(s) */
} AvahiProtocol;

typedef enum {
	AVAHI_IF_UNSPEC = -1       /**< Unspecified/all interface(s) */
} AvahiIfIndex;

int (*avahi_entry_group_add_service)(
	AvahiEntryGroup *group,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiPublishFlags flag,
	const char *name,
	const char *type,
	const char *domain,
	const char *host,
	uint16_t port,
	...
) AVAHI_GCC_SENTINEL;

typedef struct AvahiSimplePoll AvahiSimplePoll;

typedef int (*AvahiPollFunc)(void *, unsigned int, int, void *);

struct AvahiSimplePoll {
	AvahiPoll api;
	AvahiPollFunc poll_func;
	void *poll_func_userdata;
	
	struct pollfd* pollfds;
	int n_pollfds, max_pollfds, rebuild_pollfds;
	
	int watch_req_cleanup, timeout_req_cleanup;
	int quit;
	int events_valid;
	
	int n_watches;
	AvahiWatch* watches;
	void* timeouts;
	
	int wakeup_pipe[2];
	int wakeup_issued;
	
	int prepared_timeout;
	
	enum {
		STATE_INIT,
		STATE_PREPARING,
		STATE_PREPARED,
		STATE_RUNNING,
		STATE_RAN,
		STATE_DISPATCHING,
		STATE_DISPATCHED,
		STATE_QUIT,
		STATE_FAILURE
	} state;
};

/** Create a new main loop object */
AvahiSimplePoll *(*avahi_simple_poll_new)(void);

/** Free a main loop object */
void (*avahi_simple_poll_free)(AvahiSimplePoll *s);

const AvahiPoll* (*avahi_simple_poll_get)(AvahiSimplePoll *s);

void (*avahi_simple_poll_quit)(AvahiSimplePoll *s);

int (*avahi_simple_poll_loop)(AvahiSimplePoll *s);

const char *(*avahi_strerror)(int error);

struct AvahiClient {
	const AvahiPoll *poll_api;
	void * poll; //DBusConnection *bus;
	int error;
	AvahiClientState state;
	AvahiClientFlags flags;
	
	/* Cache for some seldom changing server data */
	char *version_string, *host_name, *host_name_fqdn, *domain_name;
	uint32_t local_service_cookie;
	int local_service_cookie_valid;
	
	AvahiClientCallback callback;
	void *userdata;
	
	AvahiEntryGroup * groups;
	void * domain_browsers;
	void * service_browsers;
	void * service_type_browsers;
	void * service_resolvers;
	void * host_name_resolvers;
	void * address_resolvers;
	void * record_browsers;
};

const char * (* avahi_client_get_domain_name)( AvahiClient *) ;

/* Browsing-specific */
typedef enum {
	AVAHI_BROWSER_NEW,
	AVAHI_BROWSER_REMOVE,
	AVAHI_BROWSER_CACHE_EXHAUSTED,
	AVAHI_BROWSER_ALL_FOR_NOW,
	AVAHI_BROWSER_FAILURE
} AvahiBrowserEvent;

/** Type of callback event when resolving */
typedef enum {
	AVAHI_RESOLVER_FOUND,
	AVAHI_RESOLVER_FAILURE
} AvahiResolverEvent;

typedef enum {
	AVAHI_LOOKUP_RESULT_CACHED = 1,
	AVAHI_LOOKUP_RESULT_WIDE_AREA = 2,
	AVAHI_LOOKUP_RESULT_MULTICAST = 4,
	AVAHI_LOOKUP_RESULT_LOCAL = 8,
	AVAHI_LOOKUP_RESULT_OUR_OWN = 16,
	AVAHI_LOOKUP_RESULT_STATIC = 32
} AvahiLookupResultFlags;

struct AvahiServiceBrowser ;
typedef struct AvahiServiceBrowser AvahiServiceBrowser;

typedef void (*AvahiServiceBrowserCallback) (
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AvahiLookupResultFlags flags,
	void *userdata);

struct AvahiServiceBrowser {
	char *path;
	AvahiClient *client;
	AvahiServiceBrowserCallback callback;
	void *userdata;
	AvahiServiceBrowser *service_browsers_next;
	AvahiServiceBrowser *service_browsers_prev ;
	
	char *type, *domain;
	AvahiIfIndex interface;
	AvahiProtocol protocol;
};

typedef struct AvahiServiceResolver AvahiServiceResolver;

/** An IPv4 address */
typedef struct AvahiIPv4Address {
	uint32_t address; /**< Address data in network byte order. */
} AvahiIPv4Address;

/** An IPv6 address */
typedef struct AvahiIPv6Address {
	uint8_t address[16]; /**< Address data */
} AvahiIPv6Address;

/** Protocol (address family) independent address structure */
typedef struct AvahiAddress {
	AvahiProtocol proto; /**< Address family */
	union {
		AvahiIPv6Address ipv6;  /**< Address when IPv6 */
		AvahiIPv4Address ipv4;  /**< Address when IPv4 */
		uint8_t data[1];        /**< Type-independent data field */
	} data;
} AvahiAddress;

AvahiServiceBrowser* (*avahi_service_browser_new) (
	AvahiClient *client,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	const char *type,
	const char *domain,
	int flags,
	AvahiServiceBrowserCallback callback,
	void *userdata);

/** Cleans up and frees an AvahiServiceBrowser object */
int (*avahi_service_browser_free) (AvahiServiceBrowser *);

int (*avahi_service_resolver_free)(AvahiServiceResolver *r);

typedef void (*AvahiServiceResolverCallback) (
	AvahiServiceResolver *r,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *a,
	uint16_t port,
	void *txt,
	AvahiLookupResultFlags flags,
	void *userdata);
					      
AvahiServiceResolver * (*avahi_service_resolver_new)(
	AvahiClient *client,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	const char *name,
	const char *type,
	const char *domain,
	AvahiProtocol aprotocol,
	AvahiLookupResultFlags flags,
	AvahiServiceResolverCallback callback,
	void *userdata);
						  

/**********************************************/
/* Prototypes */

struct connection_out ;
int OW_Load_avahi_library(void) ;
void OW_Free_avahi_library(void) ;
void *OW_Avahi_Announce( void * v ) ;
void *OW_Avahi_Browse(void * v) ;

#endif 	/* OW_AVAHI_H */
