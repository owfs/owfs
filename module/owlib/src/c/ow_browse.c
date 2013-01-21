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

#include "ow_connection.h"

struct BrowseStruct {
	char *name;
	char *type;
	char *domain;
};

static struct BrowseStruct *Browse_Struct_Create(const char *name, const char *type, const char *domain);
static void Browse_Struct_Destroy(struct BrowseStruct *browse_struct);
static void * OW_Browse_Bonjour(void * v) ;
static void ResolveWait( DNSServiceRef sref ) ;
static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const unsigned char *t, void *c);
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context);


static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const unsigned char *t, void *c)
{
	struct BrowseStruct *browse_struct = c;
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
		ZeroAdd( browse_struct->name, browse_struct->type, browse_struct->domain, host, service ) ;
	} else {
		LEVEL_DEBUG("Couldn't translate port %d",ntohs(port) ) ;
	}
	Browse_Struct_Destroy(browse_struct);
}

static struct BrowseStruct *Browse_Struct_Create(const char *name, const char *type, const char *domain)
{
	struct BrowseStruct *browse_struct = owmalloc(sizeof(struct BrowseStruct));
	
	if ( browse_struct == NULL ) {
		return NULL ;
	}

	browse_struct->name   = ( name    != NULL ) ? owstrdup(name)   : NULL;
	browse_struct->type   = ( type    != NULL ) ? owstrdup(type)   : NULL;
	browse_struct->domain = ( domain  != NULL ) ? owstrdup(domain) : NULL;

	return browse_struct;
}

static void Browse_Struct_Destroy(struct BrowseStruct *browse_struct)
{
	if ( browse_struct == NULL ) {
		return;
	}

	SAFEFREE(browse_struct->name  ) ;
	SAFEFREE(browse_struct->type  ) ;
	SAFEFREE(browse_struct->domain) ;

	owfree(browse_struct);
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
	struct BrowseStruct *browse_struct;
	(void) context;
	LEVEL_DETAIL("ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s", (long int) s, f, i, e, name, type, domain);

	if (e != kDNSServiceErr_NoError) {
		return ;
	}

	browse_struct = Browse_Struct_Create( name, type, domain ) ;

	if (f & kDNSServiceFlagsAdd) {	// Add
		DNSServiceRef sr;

		if (DNSServiceResolve(&sr, 0, 0, name, type, domain, ResolveBack, (void *)browse_struct) == kDNSServiceErr_NoError) {
			ResolveWait(sr) ;
			DNSServiceRefDeallocate(sr);
		} else {
			Browse_Struct_Destroy(browse_struct) ;
		}
	} else { // Remove
		Browse_Struct_Destroy(browse_struct) ;
		ZeroDel( name, type, domain ) ;
	}
}

// Called in a thread
static void * OW_Browse_Bonjour(void * v)
{
	struct connection_in * in = v ;
	DNSServiceErrorType dnserr;

	DETACH_THREAD;
	MONITOR_RLOCK ;
	dnserr = DNSServiceBrowse(&in->master.browse.bonjour_browse, 0, 0, "_owserver._tcp", NULL, BrowseBack, NULL);

	if (dnserr != kDNSServiceErr_NoError) {
		LEVEL_CONNECT("DNSServiceBrowse error = %d", dnserr);
		MONITOR_RUNLOCK ;
		return VOID_RETURN ;
	}

	// Blocks, which is why this is in it's own thread
	while (DNSServiceProcessResult(in->master.browse.bonjour_browse) == kDNSServiceErr_NoError) {
		//printf("DNSServiceProcessResult ref %ld\n",(long int)rs->sref) ;
		continue;
	}
	DNSServiceRefDeallocate(in->master.browse.bonjour_browse);
	in->master.browse.bonjour_browse = 0 ;
	MONITOR_RUNLOCK ;
	return VOID_RETURN;
}

void OW_Browse(struct connection_in *in)
{
	if ( Globals.zero == zero_avahi ) {
#if ! OW_CYGWIN && ! OW_DARWIN
		pthread_t thread;
		int err = pthread_create(&thread, DEFAULT_THREAD_ATTR, OW_Avahi_Browse, (void *) in);
		if (err) {
			LEVEL_CONNECT("Avahi Browse thread error %d.", err);
		}
#endif /* ! OW_CYGWIN && ! OW_DARWIN */
	} else if ( Globals.zero == zero_bonjour ) {
		pthread_t thread;
		int err = pthread_create(&thread, DEFAULT_THREAD_ATTR, OW_Browse_Bonjour, (void *) in);
		if (err) {
			LEVEL_CONNECT("Bonjour Browse thread error %d.", err);
		}
	}
}

#else	/* OW_ZERO */

void OW_Browse(struct connection_in *in)
{
	(void) in ;
	LEVEL_CONNECT("Avahi and Bonjour (and Multithreading) was not enabled");
}


#endif	/* OW_ZERO */
