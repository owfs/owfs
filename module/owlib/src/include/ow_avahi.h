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
	AVAHI_SERVER_INVALID,          /**< Invalid state (initial) */
	AVAHI_SERVER_REGISTERING,      /**< Host RRs are being registered */
	AVAHI_SERVER_RUNNING,          /**< All host RRs have been established */
	AVAHI_SERVER_COLLISION,        /**< There is a collision with a host RR. All host RRs have been withdrawn, the user should set a new host name via avahi_server_set_host_name() */
	AVAHI_SERVER_FAILURE           /**< Some fatal failure happened, the server is unable to proceed */
} AvahiServerState;

/** States of an entry group object */
typedef enum {
	AVAHI_ENTRY_GROUP_UNCOMMITED,    /**< The group has not yet been commited, the user must still call avahi_entry_group_commit() */
	AVAHI_ENTRY_GROUP_REGISTERING,   /**< The entries of the group are currently being registered */
	AVAHI_ENTRY_GROUP_ESTABLISHED,   /**< The entries have successfully been established */
	AVAHI_ENTRY_GROUP_COLLISION,     /**< A name collision for one of the entries in the group has been detected, the entries have been withdrawn */
	AVAHI_ENTRY_GROUP_FAILURE        /**< Some kind of failure happened, the entries have been withdrawn */
} AvahiEntryGroupState;

/** States of a client object, a superset of AvahiServerState */
typedef enum {
	AVAHI_CLIENT_S_REGISTERING = AVAHI_SERVER_REGISTERING,  /**< Server state: REGISTERING */
	AVAHI_CLIENT_S_RUNNING = AVAHI_SERVER_RUNNING,          /**< Server state: RUNNING */
	AVAHI_CLIENT_S_COLLISION = AVAHI_SERVER_COLLISION,      /**< Server state: COLLISION */
	AVAHI_CLIENT_FAILURE = 100,                             /**< Some kind of error happened on the client side */
	AVAHI_CLIENT_CONNECTING = 101                           /**< We're still connecting. This state is only entered when AVAHI_CLIENT_NO_FAIL has been passed to avahi_client_new() and the daemon is not yet available. */
} AvahiClientState;

typedef enum {
	AVAHI_PUBLISH_UNIQUE = 1,           /**< For raw records: The RRset is intended to be unique */
    AVAHI_PUBLISH_NO_PROBE = 2,         /**< For raw records: Though the RRset is intended to be unique no probes shall be sent */
    AVAHI_PUBLISH_NO_ANNOUNCE = 4,      /**< For raw records: Do not announce this RR to other hosts */
    AVAHI_PUBLISH_ALLOW_MULTIPLE = 8,   /**< For raw records: Allow multiple local records of this type, even if they are intended to be unique */
/** \cond fulldocs */
    AVAHI_PUBLISH_NO_REVERSE = 16,      /**< For address records: don't create a reverse (PTR) entry */
    AVAHI_PUBLISH_NO_COOKIE = 32,       /**< For service records: do not implicitly add the local service cookie to TXT data */
/** \endcond */
    AVAHI_PUBLISH_UPDATE = 64,          /**< Update existing records instead of adding new ones */
/** \cond fulldocs */
    AVAHI_PUBLISH_USE_WIDE_AREA = 128,  /**< Register the record using wide area DNS (i.e. unicast DNS update) */
    AVAHI_PUBLISH_USE_MULTICAST = 256   /**< Register the record using multicast DNS */
/** \endcond */
} AvahiPublishFlags;

typedef enum {
	AVAHI_CLIENT_IGNORE_USER_CONFIG = 1, /**< Don't read user configuration */
	AVAHI_CLIENT_NO_FAIL = 2        /**< Don't fail if the daemon is not available when avahi_client_new() is called, instead enter AVAHI_CLIENT_CONNECTING state and wait for the daemon to appear */
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
	const AvahiPoll *poll_api /**< The abstract event loop API to use */,
	AvahiClientFlags flags /**< Some flags to modify the behaviour of  the client library */,
	AvahiClientCallback callback /**< A callback that is called whenever the state of the client changes. This may be NULL. Please note that this function is called for the first time from within the avahi_client_new() context! Thus, in the callback you should not make use of global variables that are initialized only after your call to avahi_client_new(). A common mistake is to store the AvahiClient pointer returned by avahi_client_new() in a global variable and assume that this global variable already contains the valid pointer when the callback is called for the first time. A work-around for this is to always use the AvahiClient pointer passed to the callback function instead of the global pointer.  */,
	void *userdata /**< Some arbitrary user data pointer that will be passed to the callback function */,
	int *error /**< If creation of the client fails, this integer will contain the error cause. May be NULL if you aren't interested in the reason why avahi_client_new() failed. */
);
			       			       
/** Free a client instance. This will automatically free all
* associated browser, resolve and entry group objects. All pointers
* to such objects become invalid! */
void (*avahi_client_free)(AvahiClient *client);

/** Get the last error number. See avahi_strerror() for converting this error code into a human readable string. */
int (*avahi_client_errno) (AvahiClient*);

/** An entry group object */
typedef struct AvahiEntryGroup AvahiEntryGroup;
/** The function prototype for the callback of an AvahiEntryGroup */

typedef void (*AvahiEntryGroupCallback) (
	AvahiEntryGroup *g,
	AvahiEntryGroupState state /**< The new state of the entry group */,
	void* userdata /* The arbitrary user data pointer originally passed to avahi_entry_group_new()*/
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

/** Create a new AvahiEntryGroup object */
AvahiEntryGroup* (*avahi_entry_group_new) (
	AvahiClient* c,
	AvahiEntryGroupCallback callback /**< This callback is called whenever the state of this entry group changes. May not be NULL. Please note that this function is called for the first time from within the avahi_entry_group_new() context! Thus, in the callback you should not make use of global variables that are initialized only after your call to avahi_entry_group_new(). A common mistake is to store the AvahiEntryGroup pointer returned by avahi_entry_group_new() in a global variable and assume that this global variable already contains the valid pointer when the callback is called for the first time. A work-around for this is to always use the AvahiEntryGroup pointer passed to the callback function instead of the global pointer. */,
	void *userdata /**< This arbitrary user data pointer will be passed to the callback functon */
);

/** Commit an AvahiEntryGroup. The entries in the entry group are now registered on the network. Commiting empty entry groups is considered an error. */
int (*avahi_entry_group_commit) (AvahiEntryGroup*);

/** Reset an AvahiEntryGroup. This takes effect immediately. */
int (*avahi_entry_group_reset) (AvahiEntryGroup*);

/** Check if an AvahiEntryGroup is empty */
int (*avahi_entry_group_is_empty) (AvahiEntryGroup*);
/** Values for AvahiProtocol */

typedef enum {
	AVAHI_PROTO_INET = 0,     /**< IPv4 */
	AVAHI_PROTO_INET6 = 1,   /**< IPv6 */
	AVAHI_PROTO_UNSPEC = -1  /**< Unspecified/all protocol(s) */
} AvahiProtocol;

/** Special values for AvahiIfIndex */
typedef enum {
	AVAHI_IF_UNSPEC = -1       /**< Unspecified/all interface(s) */
} AvahiIfIndex;

/** Add a service. Takes a variable NULL terminated list of TXT record strings as last arguments. Please note that this service is not announced on the network before avahi_entry_group_commit() is called. */
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

/** A main loop object. Main loops of this type aren't very flexible
* since they only support a single wakeup type. Nevertheless it
* should suffice for small test and example applications.  */
typedef struct AvahiSimplePoll AvahiSimplePoll;
/** Prototype for a poll() type function */
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

/** Return the abstracted poll API object for this main loop
* object. The is will return the same pointer each time it is
* called. */
const AvahiPoll* (*avahi_simple_poll_get)(AvahiSimplePoll *s);

/** Request that the main loop quits. If this is called the next
call to avahi_simple_poll_iterate() will return 1 */
void (*avahi_simple_poll_quit)(AvahiSimplePoll *s);

/** Call avahi_simple_poll_iterate() in a loop and return if it returns non-zero */
int (*avahi_simple_poll_loop)(AvahiSimplePoll *s);

/** Return a human readable error string for the specified error code */
const char *(*avahi_strerror)(int error);

/** The head of the linked list. Use this in the structure that shall
* contain the head of the linked list */
#define AVAHI_LLIST_HEAD(t,name) void * name

/** A connection context */
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
	
	AVAHI_LLIST_HEAD(AvahiEntryGroup, groups);
	AVAHI_LLIST_HEAD(AvahiDomainBrowser, domain_browsers);
	AVAHI_LLIST_HEAD(AvahiServiceBrowser, service_browsers);
	AVAHI_LLIST_HEAD(AvahiServiceTypeBrowser, service_type_browsers);
	AVAHI_LLIST_HEAD(AvahiServiceResolver, service_resolvers);
	AVAHI_LLIST_HEAD(AvahiHostNameResolver, host_name_resolvers);
	AVAHI_LLIST_HEAD(AvahiAddressResolver, address_resolvers);
	AVAHI_LLIST_HEAD(AvahiRecordBrowser, record_browsers);
};

#endif 	/* OW_AVAHI_H */
