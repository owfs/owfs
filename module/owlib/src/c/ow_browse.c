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

struct BrowseStruct {
	char *name;
	char *type;
	char *domain;
};

static struct BrowseStruct *BSCreate(const char *name, const char *type, const char *domain);
static void BSKill(struct BrowseStruct *bs);
static void * OW_Browse_Bonjour(void * v) ;
static void ResolveWait( DNSServiceRef sref ) ;
static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c);
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context);


static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c)
{
	struct BrowseStruct *bs = c;
	char service[11] ;
	int sn_ret ;

	(void) tl;
	(void) t;

	UCLIBCLOCK ;
	sn_ret = snprintf(service, 10, "%d", ntohs(port) ) ;
	UCLIBCUNLOCK ;

	LEVEL_DETAIL("ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%d", (long int) s, f, i, e, n, host, ntohs(port));
	/* remove trailing .local. */
	if ( sn_ret >-1 ) {
		LEVEL_DETAIL("ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%s", (long int) s, f, i, e, n, host, service);
		ZeroAdd( bs->name, bs->type, bs->domain, host, service ) ;
	} else {
		LEVEL_DEBUG("Couldn't translate port %d",ntohs(port) ) ;
	}
	BSKill(bs);
}

static struct BrowseStruct *BSCreate(const char *name, const char *type, const char *domain)
{
	struct BrowseStruct *bs = owmalloc(sizeof(struct BrowseStruct));
	if (bs) {
		bs->name = name ? owstrdup(name) : NULL;
		bs->type = type ? owstrdup(type) : NULL;
		bs->domain = domain ? owstrdup(domain) : NULL;
	}
	return bs;
}

static void BSKill(struct BrowseStruct *bs)
{
	if (bs) {
		if (bs->name) {
			owfree(bs->name);
		}
		if (bs->type) {
			owfree(bs->type);
		}
		if (bs->domain) {
			owfree(bs->domain);
		}
		owfree(bs);
	}
}

// Wait for a resolve, then return. Timeout after 2 minutes
static void ResolveWait( DNSServiceRef sref )
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = DNSServiceRefSockFD(sref);

	if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
		while (1) {
			fd_set readfd;
			struct timeval tv = { 120, 0 };

			FD_ZERO(&readfd);
			FD_SET(file_descriptor, &readfd);
			if (select(file_descriptor + 1, &readfd, NULL, NULL, &tv) > 0) {
				if (FD_ISSET(file_descriptor, &readfd)) {
					DNSServiceProcessResult(sref);
				}
			} else if (errno == EINTR) {
				continue;
			} else {
				ERROR_CONNECT("Resolve timeout error");
			}
			break;
		}
	}
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context)
{
	struct BrowseStruct *bs;
	(void) context;
	LEVEL_DETAIL("ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s", (long int) s, f, i, e, name, type, domain);

	if (e != kDNSServiceErr_NoError) {
		return ;
	}

	bs = BSCreate( name, type, domain ) ;

	if (f & kDNSServiceFlagsAdd) {	// Add
		DNSServiceRef sr;

		if (DNSServiceResolve(&sr, 0, 0, name, type, domain, ResolveBack, (void *)bs) == kDNSServiceErr_NoError) {
			ResolveWait(sr) ;
			DNSServiceRefDeallocate(sr);
		} else {
			BSKill(bs) ;
		}
	} else { // Remove
		BSKill(bs) ;
		ZeroDel( name, type, domain ) ;
	}
}

// Called in a thread
static void * OW_Browse_Bonjour(void * v)
{
	DNSServiceErrorType dnserr;

	(void) v ;

	pthread_detach(pthread_self());

	dnserr = DNSServiceBrowse(&Globals.browse, 0, 0, "_owserver._tcp", NULL, BrowseBack, NULL);

	if (dnserr != kDNSServiceErr_NoError) {
		LEVEL_CONNECT("DNSServiceBrowse error = %d", dnserr);
		return NULL ;
	}

	// Blocks, which is why this is in it's own thread
	while (DNSServiceProcessResult(Globals.browse) == kDNSServiceErr_NoError) {
		//printf("DNSServiceProcessResult ref %ld\n",(long int)rs->sref) ;
		continue;
	}
	DNSServiceRefDeallocate(Globals.browse);
	Globals.browse = 0 ;
	return NULL;
}

void OW_Browse(void)
{
	if ( Globals.zero == zero_avahi ) {
#if !OW_CYGWIN
		pthread_t thread;
		int err = pthread_create(&thread, NULL, OW_Avahi_Browse, NULL);
		if (err) {
			LEVEL_CONNECT("Avahi Browse thread error %d.", err);
		}
#endif
	} else if ( Globals.zero == zero_bonjour ) {
		pthread_t thread;
		int err = pthread_create(&thread, NULL, OW_Browse_Bonjour, NULL);
		if (err) {
			LEVEL_CONNECT("Bonjour Browse thread error %d.", err);
		}
	}
}

#else							/* OW_MT */

void OW_Browse(void)
{
	LEVEL_CONNECT("Avahi and Bonjour requires multithreading support (a compile-time configuration setting).");
}

#endif							/* OW_MT */

#else							/* OW_ZERO */

#include "ow_debug.h"

void OW_Browse(void)
{
	LEVEL_CONNECT("OWFS was compiled without Avahi or Bonjour support.");
}

#endif							/* OW_ZERO */
