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

#if OW_ZERO

#if OW_MT

#include "ow_connection.h"

struct RefStruct {
	DNSServiceRef sref;
};

struct BrowseStruct {
	char *name;
	char *type;
	char *domain;
};

static struct connection_in *FindIn(struct BrowseStruct *bs);
static struct BrowseStruct *BSCreate(const char *name, const char *type, const char *domain);
static void BSKill(struct BrowseStruct *bs);
static void *Process(void *v);
static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c);
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context);

/* Look for new services with Bonjour -- will block so done in a separate thread */
static void *Process(void *v)
{
	struct RefStruct *rs = v;
	pthread_detach(pthread_self());

	while (DNSServiceProcessResult(rs->sref) == kDNSServiceErr_NoError) {
		//printf("DNSServiceProcessResult ref %ld\n",(long int)rs->sref) ;
		continue;
	}
	DNSServiceRefDeallocate(rs->sref);
	free(rs);
	LEVEL_DEBUG("Browse: Normal exit.\n");
	return NULL;
}


static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c)
{
	ASCII name[121];
	struct BrowseStruct *bs = c;
	struct connection_in *in;
	(void) tl;
	(void) t;
	//printf("ResolveBack ref=%ld flags=%d index=%d, error=%d name=%s host=%s port=%d\n",(long int)s,f,i,e,name,host,ntohs(port)) ;
	//printf("ResolveBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,bs->name,bs->type,bs->domain) ;
	LEVEL_DETAIL("ResolveBack ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%d\n", (long int) s, f, i, e, n, host, ntohs(port));
	/* remove trailing .local. */
	if (snprintf(name, 120, "%s:%d", SAFESTRING(host), ntohs(port)) < 0) {
		ERROR_CONNECT("Trouble with zeroconf resolve return %s\n", n);
	} else {
		if ((in = FindIn(bs)) == NULL) {	// new or old connection_in slot?
			if ((in = NewIn())) {
				BUSLOCKIN(in);
				in->name = strdup(name);
				in->busmode = bus_server;
			}
		}
		if (in) {
			if (Zero_detect(in)) {
				//BadAdapter_detect(in);
				printf("Zero_detect failed\n");
				exit(1);
			} else {
				in->connin.tcp.type = strdup(bs->type);
				in->connin.tcp.domain = strdup(bs->domain);
				in->connin.tcp.fqdn = strdup(n);
			}
			BUSUNLOCKIN(in);
		}
	}
	BSKill(bs);
}

static struct BrowseStruct *BSCreate(const char *name, const char *type, const char *domain)
{
	struct BrowseStruct *bs = malloc(sizeof(struct BrowseStruct));
	if (bs) {
		bs->name = name ? strdup(name) : NULL;
		bs->type = type ? strdup(type) : NULL;
		bs->domain = domain ? strdup(domain) : NULL;
	}
	return bs;
}

static void BSKill(struct BrowseStruct *bs)
{
	if (bs) {
		if (bs->name)
			free(bs->name);
		if (bs->type)
			free(bs->type);
		if (bs->domain)
			free(bs->domain);
		free(bs);
	}
}

static struct connection_in *FindIn(struct BrowseStruct *bs)
{
	struct connection_in *now;
	for (now = head_inbound_list; now; now = now->next) {
		if (now->busmode != bus_zero || strcasecmp(now->name, bs->name)
			|| strcasecmp(now->connin.tcp.type, bs->type)
			|| strcasecmp(now->connin.tcp.domain, bs->domain)
			)
			continue;
		BUSLOCKIN(now);
		break;
	}
	return now;
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context)
{
	(void) context;
	//printf("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,name,type,domain) ;
	LEVEL_DETAIL("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n", (long int) s, f, i, e, name, type, domain);

	if (e == kDNSServiceErr_NoError) {
		struct BrowseStruct *bs = BSCreate(name, type, domain);

		CONNIN_WLOCK;
		if (bs) {
			struct connection_in *in = FindIn(bs);

			if (in) {
				FreeClientAddr(in);
				BUSUNLOCKIN(in);
			}
			if (f & kDNSServiceFlagsAdd) {	// Add
				DNSServiceRef sr;

				if (DNSServiceResolve(&sr, 0, 0, name, type, domain, ResolveBack, bs) == kDNSServiceErr_NoError) {
					int file_descriptor = DNSServiceRefSockFD(sr);
					DNSServiceErrorType err = kDNSServiceErr_Unknown;
					if (file_descriptor >= 0) {
						while (1) {
							fd_set readfd;
							struct timeval tv = { 120, 0 };

							FD_ZERO(&readfd);
							FD_SET(file_descriptor, &readfd);
							if (select(file_descriptor + 1, &readfd, NULL, NULL, &tv) > 0) {
								if (FD_ISSET(file_descriptor, &readfd)) {
									err = DNSServiceProcessResult(sr);
								}
							} else if (errno == EINTR) {
								continue;
							} else {
								ERROR_CONNECT("Resolve timeout error for %s\n", name);
							}
							break;
						}
					}
					DNSServiceRefDeallocate(sr);
					if (err == kDNSServiceErr_NoError)
						return;
				}
			}
			BSKill(bs);
		}
		CONNIN_WUNLOCK;
	}
	return;
}

void OW_Browse(void)
{
	struct RefStruct *rs;
	DNSServiceErrorType dnserr;

	if ((rs = malloc(sizeof(struct RefStruct))) == NULL)
		return;

	dnserr = DNSServiceBrowse(&Globals.browse, 0, 0, "_owserver._tcp", NULL, BrowseBack, NULL);
	rs->sref = Globals.browse;
	if (dnserr == kDNSServiceErr_NoError) {
		pthread_t thread;
		int err;
		//printf("Browse %ld %s|%s|%s\n",(long int)Globals.browse,"","_owserver._tcp","") ;
		err = pthread_create(&thread, 0, Process, (void *) rs);
		if (err) {
			ERROR_CONNECT("Zeroconf/Bounjour browsing thread error %d).\n", err);
		}
	} else {
		LEVEL_CONNECT("DNSServiceBrowse error = %d\n", dnserr);
	}
}

#else							/* OW_MT */

void OW_Browse(void)
{
	LEVEL_CONNECT("Zeroconf/Bonjour requires multithreading support (a compile-time configuration setting).\n");
}

#endif							/* OW_MT */

#else							/* OW_ZERO */

#include "ow_debug.h"

void OW_Browse(void)
{
	LEVEL_CONNECT("OWFS is compiled without Zeroconf/Bonjour support.\n");
}

#endif							/* OW_ZERO */
