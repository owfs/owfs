/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include "owshell.h"

#if OW_ZERO

static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const unsigned char *t, void *c);
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context);
static void HandleCall(DNSServiceRef sref);

static void HandleCall(DNSServiceRef sref)
{
	int file_descriptor;
	DNSServiceErrorType err = kDNSServiceErr_Unknown;
	//file_descriptor = DNSServiceRefSockFD(NULL) ;
	//PRINT_ERROR("HandleCall: file_descriptor=%d (just a test of function-call, should result -1)\n", file_descriptor);
	file_descriptor = DNSServiceRefSockFD(sref);
	//PRINT_ERROR("HandleCall: file_descriptor=%d\n", file_descriptor);
	if (file_descriptor >= 0) {
		while (1) {
			fd_set readfd;
			int rc;
			struct timeval tv = { 10, 0 };
			FD_ZERO(&readfd);
			FD_SET(file_descriptor, &readfd);
			rc = select(file_descriptor + 1, &readfd, NULL, NULL, &tv);
			if (rc == -1) {
				if (errno == EINTR) {
					continue;
				}
				PERROR("Service Discovery select returned error\n");
			} else if (rc > 0) {
				if (FD_ISSET(file_descriptor, &readfd)) {
					err = DNSServiceProcessResult(sref);
				}
			} else {
				PERROR("Service Discovery timed out\n");
			}
			break;
		}
	} else {
		PRINT_ERROR("No Service Discovery socket\n");
	}
	DNSServiceRefDeallocate(sref);
	if (err != kDNSServiceErr_NoError) {
		PRINT_ERROR("Service Discovery Process result error  0x%X\n", (int) err);
		Exit(1);
	}
}


static void ResolveBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
						DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const unsigned char *t, void *c)
{
	ASCII name[121];
	(void) tl;
	(void) t;
	(void) c;
	(void) n;
	(void) s;
	(void) f;
	(void) i;
	(void) e;
	//PRINT_ERROR("ResolveBack ref=%ld flags=%ld index=%ld, error=%d name=%s host=%s port=%d\n",(long int)s,(long int)f,(long int)i,(int)e,SAFESTRING(name),SAFESTRING(host),ntohs(port)) ;
	if (snprintf(name, 120, "%s:%d", SAFESTRING(host), ntohs(port)) < 0) {
		PERROR("Trouble with zeroconf resolve return\n");
	} else if (count_inbound_connections < 1) {
		++count_inbound_connections;
		owserver_connection->name = strdup(name);
		return;
	}
	Exit(1);
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack(DNSServiceRef s, DNSServiceFlags f, uint32_t i,
					   DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *context)
{
	(void) context;
	(void) i ;
	(void) s ;
	//PRINT_ERROR("BrowseBack ref=%ld flags=%ld index=%ld, error=%d name=%s type=%s domain=%s\n",(long int)s,(long int)f,(long int)i,(int)e,SAFESTRING(name),SAFESTRING(type),SAFESTRING(domain)) ;

	if (e == kDNSServiceErr_NoError) {

		if (f & kDNSServiceFlagsAdd) {	// Add
			DNSServiceRef sr;

			if (DNSServiceResolve(&sr, 0, 0, name, type, domain, ResolveBack, NULL) == kDNSServiceErr_NoError) {
				HandleCall(sr);
				return;
			} else {
				PRINT_ERROR("Service Resolve error on %s\n", SAFESTRING(name));
			}
		} else {
			PRINT_ERROR("OWSERVER %s is leaving\n", name);
		}
	} else {
		PRINT_ERROR("Browse callback error = %d\n", (int) e);
	}
	Exit(1);
}

void OW_Browse(void)
{
	DNSServiceErrorType dnserr;
	DNSServiceRef sref;

	dnserr = DNSServiceBrowse(&sref, 0, 0, "_owserver._tcp", NULL, BrowseBack, NULL);
	if (dnserr == kDNSServiceErr_NoError) {
		HandleCall(sref);
	} else {
		PRINT_ERROR("DNSServiceBrowse error = %d\n", (int) dnserr);
		Exit(1);
	}
}

#else							/* OW_ZERO */

void OW_Browse(void)
{
	PRINT_ERROR("OWFS is compiled without Zeroconf/Bonjour support.\n");
}

#endif							/* OW_ZERO */
