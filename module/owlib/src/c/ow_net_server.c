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

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* Prototypes */
static int ServerAddr(struct connection_out *out);
static int ServerListen(struct connection_out *out);


static int ServerAddr(struct connection_out *out)
{
	struct addrinfo hint;
	int ret;
	char *p;

	if (out->name == NULL)
		return -1;
	if ((p = strrchr(out->name, ':'))) {	/* : exists */
		p[0] = '\0';
		out->host = strdup(out->name);
		p[0] = ':';
		if (out->host == NULL)
			return -ENOMEM;
		out->service = p + 1;
		//printf("ServerAddr name=<%s>\n",out->name) ;
		//printf("ServerAddr search name=<%s> :=<%s>\n",out->host,out->service) ;
	} else {
#if OW_CYGWIN
		out->host = strdup("0.0.0.0");
#else
		out->host = NULL;
#endif
		out->service = strdup(out->name);
	}

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_flags = AI_PASSIVE;
	hint.ai_socktype = SOCK_STREAM;
#if OW_CYGWIN || defined(__FreeBSD__)
	hint.ai_family = AF_INET;	// FreeBSD will bind IP6 preferentially
#else							/* __FreeBSD__ */
	hint.ai_family = AF_UNSPEC;
#endif							/* __FreeBSD__ */

	//printf("ServerAddr: [%s] [%s]\n", out->host, out->service);

	if ((ret = getaddrinfo(out->host, out->service, &hint, &out->ai))) {
		ERROR_CONNECT("GetAddrInfo error [%s]=%s:%s\n",
					  SAFESTRING(out->name), SAFESTRING(out->host),
					  SAFESTRING(out->service));
		return -1;
	}
	return 0;
}

static int ServerListen(struct connection_out *out)
{
	int fd;
	int on = 1;
	int ret;

	if (out->ai == NULL) {
		LEVEL_CONNECT("Server address not yet parsed [%s]\n",
					  SAFESTRING(out->name));
		return -1;
	}

	if (out->ai_ok == NULL)
		out->ai_ok = out->ai;
	do {
		fd = socket(out->ai_ok->ai_family,
					out->ai_ok->ai_socktype, out->ai_ok->ai_protocol);
		if (fd >= 0) {
			ret =
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,
						   sizeof(on))
				|| bind(fd, out->ai_ok->ai_addr, out->ai_ok->ai_addrlen)
				|| listen(fd, 10);
			if (ret) {
				ERROR_CONNECT("ServerListen: Socket problem [%s]\n",
							  SAFESTRING(out->name));
				close(fd);
			} else {
				out->fd = fd;
				return fd;
			}
		} else {
			ERROR_CONNECT("ServerListen: socket() [%s]",
						  SAFESTRING(out->name));
		}
	} while ((out->ai_ok = out->ai_ok->ai_next));
	ERROR_CONNECT("ServerListen: Socket problem [%s]\n",
				  SAFESTRING(out->name));
	return -1;
}

int ServerOutSetup(struct connection_out *out)
{
	return ServerAddr(out) || (ServerListen(out) < 0);
}

/*
 Loops through outdevices, starting a detached thread for each 
 Each loop spawn threads for accepting connections
 Uses my non-patented "pre-threaded technique"
 */

#if OW_MT

/* Accept thread
   Very clever design (if I may say so)
   Avoids thundering herd, and precreates the next thread before the connection comes in
   so thread creation isn't charged to transaction time

   A mutex is used.

   Try to lock mutex.
        If able to lock, then create son and proceed to accept. On acccept, unlock mutex, and process.
 */
static void * ServerProcessAccept(void *v)
{
	struct connection_out *out = v ;
	pthread_t tid;
	int acceptfd;
	int ret;

    pthread_detach(pthread_self());

LoopIfNoThreading:
    
	LEVEL_DEBUG("ServerProcessAccept %s[%lu] try lock %d\n",
				SAFESTRING(out->name), (unsigned long int) pthread_self(),
				out->index);

	ACCEPTLOCK(out);

    ret = pthread_create( &tid, NULL, ServerProcessAccept, v ) ;

    LEVEL_DEBUG("ServerProcessAccept %s[%lu] locked %d\n",
				SAFESTRING(out->name), (unsigned long int) pthread_self(),
				out->index);

	do {
		acceptfd = accept(out->fd, NULL, NULL);
		//LEVEL_DEBUG("ServerProcessAccept %s[%lu] accept %d\n",SAFESTRING(out->name),(unsigned long int)pthread_self(),out->index) ;
		if (shutdown_in_progress)
			break;
		if (acceptfd < 0) {
			if (errno == EINTR) {
				LEVEL_DEBUG("ow_net.c: accept interrupted\n");
				continue;
			}
			LEVEL_DEBUG("ow_net.c: accept error %d [%s]\n", errno,
						strerror(errno));
		}
		break;
	} while (1);
	ACCEPTUNLOCK(out);
	//LEVEL_DEBUG("ServerProcessAccept %s[%lu] unlock %d\n",SAFESTRING(out->name),(unsigned long int)pthread_self(),out->index) ;

	if (shutdown_in_progress) {
		LEVEL_DEBUG
			("ServerProcessAccept %s[%lu] shutdown_in_progress %d return\n",
			 SAFESTRING(out->name), (unsigned long int) pthread_self(),
			 out->index);
		if (acceptfd >= 0)
			close(acceptfd);
		return NULL;
	}

	if (acceptfd < 0) {
		ERROR_CONNECT("ServerProcessAccept: accept() problem %d (%d)\n",
					  out->fd, out->index);
	} else {
        (out->HandlerRoutine)(acceptfd) ;
        close(acceptfd) ;
    }

    if ( ret ) goto LoopIfNoThreading ;
    
    LEVEL_DEBUG("ServerProcessAccept = %lu CLOSING\n",
				(unsigned long int) pthread_self());
	return NULL ;
}

/* For a given port (connecion_out) set up listening */
static void *ServerProcessOut(void *v)
{
	struct connection_out *out = v;

	LEVEL_DEBUG("ServerProcessOut = %lu\n",
				(unsigned long int) pthread_self());

	if (ServerOutSetup(out)) {
		LEVEL_CONNECT("Cannot set up outdevice [%s](%d) -- will exit\n",
					  SAFESTRING(out->name), out->index);
		(out->Exit) (1);
	}

	OW_Announce(out);

	while (!shutdown_in_progress) {
		ServerProcessAccept(v);
	}
	LEVEL_DEBUG("ServerProcessOut = %lu CLOSING (%s)\n",
				(unsigned long int) pthread_self(), SAFESTRING(out->name));
	OUTLOCK(out);
	out->tid = 0;
	OUTUNLOCK(out);
	pthread_exit(NULL);
	return NULL;
}

/* Setup Servers -- a thread for each port */
void ServerProcess(void (*HandlerRoutine) (int fd),
				   void (*Exit) (int errcode))
{
	struct connection_out *out = outdevice;
	int err, signo;
	sigset_t myset;

	if (outdevices == 0) {
		LEVEL_CALL("No output devices defined\n");
		Exit(1);
	}

	/* Start the head of a thread chain for each outdevice */
	for (out = outdevice; out; out = out->next) {
		OUTLOCK(out);
		out->HandlerRoutine = HandlerRoutine;
		out->Exit = Exit;
		if (pthread_create
			(&(out->tid), NULL, ServerProcessOut, (void *) (out))) {
			OUTUNLOCK(out);
			ERROR_CONNECT("Could not create a thread for %s\n",
						  SAFESTRING(out->name));
			Exit(1);
		}
		OUTUNLOCK(out);
	}

	(void) sigemptyset(&myset);
	(void) sigaddset(&myset, SIGHUP);
	(void) sigaddset(&myset, SIGINT);
	(void) sigaddset(&myset, SIGTERM);
	(void) pthread_sigmask(SIG_BLOCK, &myset, NULL);
	while (!shutdown_in_progress) {
		if ((err = sigwait(&myset, &signo)) == 0) {
			if (signo == SIGHUP) {
				LEVEL_DEBUG("ServerProcess: ignore signo=%d\n", signo);
				continue;
			}
			LEVEL_DEBUG("ServerProcess: break signo=%d\n", signo);
			break;
		} else {
			LEVEL_DEBUG("ServerProcess: sigwait error %n\n", err);
		}
	}
	LEVEL_DEBUG("ow_net.c:ServerProcess() shutdown initiated\n");

	for (out = outdevice; out; out = out->next) {
		OUTLOCK(out);
		if (out->tid) {
			LEVEL_DEBUG("Shutting down %d of %d thread %ld\n", out->index,
						outdevices, out->tid);
			if (pthread_cancel(out->tid)) {
				LEVEL_DEBUG("Can't kill %d of %d\n", out->index,
							outdevices);
			}
			out->tid = 0;
		}
		OUTUNLOCK(out);
	}

	LEVEL_DEBUG("ow_net.c:ServerProcess() shutdown done\n");

	/* Cleanup that may never be reached */
	Exit(0);
}

#else							/* OW_MT */

// Non multithreaded
void ServerProcess(void (*HandlerRoutine) (int fd),
				   void (*Exit) (int errcode))
{
	if (outdevices == 0) {
		LEVEL_CONNECT("No output device (port) specified. Exiting.\n");
		Exit(1);
	} else if (outdevices > 1) {
		LEVEL_CONNECT
			("More than one output device specified (%d). Library compiled non-threaded. Exiting.\n",
			 indevices);
		Exit(1);
	} else if (ServerAddr(outdevice) || (ServerListen(outdevice) < 0)) {
		LEVEL_CONNECT("Cannot set up outdevice [%s] -- will exit\n",
					  SAFESTRING(outdevice->name));
		Exit(1);
	} else {
		OW_Announce(outdevice);
		while (1) {
			int acceptfd = accept(outdevice->fd, NULL, NULL);
			if (shutdown_in_progress)
				break;
			if (acceptfd < 0) {
				ERROR_CONNECT("Trouble with accept, will reloop\n");
			} else {
				HandlerRoutine(acceptfd);
				close(acceptfd);
			}
		}
		Exit(0);
	}
}
#endif							/* OW_MT */
