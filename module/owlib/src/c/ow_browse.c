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

static struct connection_in *FindIn(struct BrowseStruct *bs);
static struct BrowseStruct *BSCreate(const char *name, const char *type, const char *domain);
static void BSKill(struct BrowseStruct *bs);
static void * OW_Browse_Bonjour(void * v) ;
static void ResolveWait( DNSServiceRef sref ) ;
static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c);
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context);


#if 0
static void RequestBack(DNSServiceRef sr,
						DNSServiceFlags flags,
						uint32_t interfaceIndex,
						DNSServiceErrorType errorCode,
						const char *fullname, uint16_t rrtype, uint16_t rrclass, uint16_t rdlen, const void *rdata, uint32_t ttl, void *context)
{
	int i;
	(void) context;
	printf("DNSServiceRef %ld\n", (long int) sr);
	printf("DNSServiceFlags %d\n", flags);
	printf("interface %d\n", interfaceIndex);
	printf("DNSServiceErrorType %d\n", errorCode);
	printf("name <%s>\n", fullname);
	printf("rrtype %d\n", rrtype);
	printf("rrclass %d\n", rrclass);
	printf("rdlen %d\n", rdlen);
	printf("rdata ");
	for (i = 0; i < rdlen; ++i)
		printf(" %.2X", ((const BYTE *) rdata)[i]);
	printf("\nttl %d\n", ttl);
}

		/* Resolved service -- add to connection_in */
#endif							/* 0 */

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
			CONNIN_WLOCK;
			in = NewIn(NULL);
			if (in != NULL) {
				BUSLOCKIN(in);
				in->name = strdup(name);
				in->busmode = bus_server;
			}
			CONNIN_WUNLOCK;
		}
		if (in != NULL) {
			if (Zero_detect(in)) {
				BadAdapter_detect(in);
			} else {
				in->connin.tcp.type = strdup(bs->type);
				in->connin.tcp.domain = strdup(bs->domain);
				in->connin.tcp.fqdn = strdup(n);
			}
			BUSUNLOCKIN(in);
		}
	}
#if 0
	// test out RecordRequest and RecordReconfirm
	do {
		DNSServiceRef sr;
		printf("\nRecord Request (of %s) = %d\n", n,
			   DNSServiceQueryRecord(&sr, 0, 0, n, kDNSServiceType_SRV, kDNSServiceClass_IN, RequestBack, NULL));
		printf("Process Request = %d\n", DNSServiceProcessResult(sr));
		DNSServiceRefDeallocate(sr);
		printf("Reconfirm %s\n", n);
		DNSServiceReconfirmRecord(0, 0, n, kDNSServiceType_SRV, kDNSServiceClass_IN, 0, NULL);
		printf("Record Request (of %s) = %d\n", n, DNSServiceQueryRecord(&sr, 0, 0, n, kDNSServiceType_SRV, kDNSServiceClass_IN, RequestBack, NULL));
		printf("Process Request = %d\n\n", DNSServiceProcessResult(sr));
		DNSServiceRefDeallocate(sr);
	} while (0);
#endif							/* 0 */
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
		if (bs->name) {
			free(bs->name);
		}
		if (bs->type) {
			free(bs->type);
		}
		if (bs->domain) {
			free(bs->domain);
		}
		free(bs);
	}
}

static struct connection_in *FindIn(struct BrowseStruct *bs)
{
	struct connection_in *now;
	CONNIN_RLOCK;
	now = head_inbound_list;
	CONNIN_RUNLOCK;
	for (; now != NULL; now = now->next) {
		if (now->busmode != bus_zero || strcasecmp(now->name, bs->name)
			|| strcasecmp(now->connin.tcp.type, bs->type)
			|| strcasecmp(now->connin.tcp.domain, bs->domain)
			) {
			continue;
		}
		BUSLOCKIN(now);
		break;
	}
	return now;
}
// Wait for a resolve, then return. Timeout after 2 minutes
static void ResolveWait( DNSServiceRef sref )
{
	int file_descriptor = DNSServiceRefSockFD(sref);

	if (file_descriptor >= 0) {
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
				ERROR_CONNECT("Resolve timeout error\n");
			}
			break;
		}
	}
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context)
{
	(void) context;
	struct BrowseStruct * bs;
	//printf("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,name,type,domain) ;
	LEVEL_DETAIL("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n", (long int) s, f, i, e, name, type, domain);

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
		LEVEL_CONNECT("Bonjour: DNSServiceBrowse error = %d\n", dnserr);
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
	pthread_t thread;
	int err ;
	
	if ( Globals.zero == zero_avahi ) {
		err = pthread_create(&thread, NULL, OW_Avahi_Browse, NULL);
		if (err) {
			LEVEL_CONNECT("Avahi Browse thread error %d.\n", err);
		}
	} else if ( Globals.zero == zero_bonjour ) {
		err = pthread_create(&thread, NULL, OW_Browse_Bonjour, NULL);
		if (err) {
			LEVEL_CONNECT("Bonjour Browse thread error %d.\n", err);
		}
	}
}

#else							/* OW_MT */

void OW_Browse(void)
{
	LEVEL_CONNECT("Avahi and Bonjour requires multithreading support (a compile-time configuration setting).\n");
}

#endif							/* OW_MT */

#else							/* OW_ZERO */

#include "ow_debug.h"

void OW_Browse(void)
{
	LEVEL_CONNECT("OWFS was compiled without Avahi or Bonjour support.\n");
}

#endif							/* OW_ZERO */
