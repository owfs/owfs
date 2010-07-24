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
#include "ow_connection.h"

#if OW_ZERO && OW_MT

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void RegisterBack(DNSServiceRef s, DNSServiceFlags f, DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *v) ;
static void Announce_Post_Register(DNSServiceRef sref, DNSServiceErrorType err) ;
static void *Announce(void *v) ;

/* Sent back from Bonjour -- arbitrarily use it to set the Ref for Deallocation */
static void RegisterBack(DNSServiceRef s, DNSServiceFlags f, DNSServiceErrorType e, const char *name, const char *type, const char *domain, void *v)
{
	struct connection_out * out = v ;
	LEVEL_DETAIL
		("RegisterBack ref=%d flags=%d error=%d name=%s type=%s domain=%s", s, f, e, SAFESTRING(name), SAFESTRING(type), SAFESTRING(domain));
	if (e != kDNSServiceErr_NoError) {
		return ;
	}
	out->sref0 = s;

	SAFEFREE( out->zero.name ) ;
	out->zero.name = owstrdup(name) ;

	SAFEFREE( out->zero.type ) ;
	out->zero.type = owstrdup(type) ;

	SAFEFREE( out->zero.domain ) ;
	out->zero.domain = owstrdup(domain) ;
}

static void Announce_Post_Register(DNSServiceRef sref, DNSServiceErrorType err)
{
	if (err == kDNSServiceErr_NoError) {
		DNSServiceProcessResult(sref);
	} else {
		LEVEL_CONNECT("Unsuccessful call to DNSServiceRegister err = %d", err);
	}
}

/* Register the out port with Bonjour -- might block so done in a separate thread */
static void *Announce(void *v)
{
	struct connection_out *out = v;
	DNSServiceRef sref;
	DNSServiceErrorType err;

	struct sockaddr sa;
	//socklen_t sl = sizeof(sa);
	socklen_t sl = 128;
	uint16_t port ;
	char * service_name ;
	char name[63] ;

	#if OW_MT
	pthread_detach(pthread_self());
	#endif							/* OW_MT */

	if (getsockname(out->file_descriptor, &sa, &sl)) {
		LEVEL_CONNECT("Could not get port number of device.");
		return NULL;
	}
	port = ntohs(((struct sockaddr_in *) (&sa))->sin_port) ;

	/* Add the service */
	switch (Globals.opt) {
		case opt_httpd:
			service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Web" ;
			UCLIBCLOCK;
			snprintf(name,62,"%s <%d>",service_name,(int)port);
			UCLIBCUNLOCK;
			err = DNSServiceRegister(&sref, 0, 0, name,"_http._tcp", NULL, NULL, port, 0, NULL, RegisterBack, out) ;
			Announce_Post_Register(sref, err) ;
			err = DNSServiceRegister(&sref, 0, 0, name,"_owhttpd._tcp", NULL, NULL, port, 0, NULL, RegisterBack, out) ;
			break ;
		case opt_server:
			service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) Server" ;
			UCLIBCLOCK;
			snprintf(name,62,"%s <%d>",service_name,(int)port);
			UCLIBCUNLOCK;
			err = DNSServiceRegister(&sref, 0, 0, name,"_owserver._tcp", NULL, NULL, port, 0, NULL, RegisterBack, out) ;
			break;
		case opt_ftpd:
			service_name = (Globals.announce_name) ? Globals.announce_name : "OWFS (1-wire) FTP" ;
			UCLIBCLOCK;
			snprintf(name,62,"%s <%d>",service_name,(int)port);
			UCLIBCUNLOCK;
			err = DNSServiceRegister(&sref, 0, 0, name,"_owftp._tcp", NULL, NULL, port, 0, NULL, RegisterBack, out) ;
			break;
		default:
			err = kDNSServiceErr_NoError ;
			break ;
	}

	Announce_Post_Register(sref, err) ;
	LEVEL_DEBUG("Normal exit");
	pthread_exit(NULL);

	return NULL;
}

void ZeroConf_Announce(struct connection_out *out)
{
	if ( Globals.announce_off) {
		return;
	} else if ( Globals.zero == zero_avahi ) {
#if !OW_CYGWIN
		// avahi only implemented with multithreading
		pthread_t thread;
		int err = pthread_create(&thread, NULL, OW_Avahi_Announce, (void *) out);
		if (err) {
			LEVEL_CONNECT("Avahi registration thread error %d.", err);
		}
#endif
	} else if ( Globals.zero == zero_bonjour ) {
		pthread_t thread;
		int err = pthread_create(&thread, NULL, Announce, (void *) out);
		if (err) {
			LEVEL_CONNECT("Zeroconf/Bonjour registration thread error %d.", err);
		}
	}
}

#else							/* OW_ZERO && OW_MT  */

void ZeroConf_Announce(struct connection_out *out)
{
	(void) out;
	LEVEL_CONNECT("Zeroconf and/or Multithreading are not enabled");
	return;
}

#endif							/* OW_ZERO */
