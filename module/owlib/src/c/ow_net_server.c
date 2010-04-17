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

#if OW_MT
	/* Locking for thread work */
	/* Variables only used in this particular file */
	/* i.e. "locally global" */
	my_rwlock_t shutdown_mutex_rw ;
	pthread_mutex_t handler_thread_mutex ;
	int handler_thread_count ;
	int shutdown_in_progress ;
	FILE_DESCRIPTOR_OR_ERROR shutdown_pipe[2] ;
#endif /* OW_MT */


/* Prototypes */
static GOOD_OR_BAD ServerAddr(const char * default_port, struct connection_out *out);
static FILE_DESCRIPTOR_OR_ERROR ServerListen(struct connection_out *out);

static FILE_DESCRIPTOR_OR_ERROR SetupListenSet( fd_set * listenset ) ;
static GOOD_OR_BAD SetupListenSockets( void (*HandlerRoutine) (int file_descriptor) ) ;
static void CloseListenSockets( void ) ;
static void ProcessListenSocket( struct connection_out * out ) ;
static void *ProcessAcceptSocket(void *arg) ;
static void ProcessListenSet( fd_set * listenset ) ;
static GOOD_OR_BAD ListenCycle( void ) ;

static GOOD_OR_BAD ServerAddr(const char * default_port, struct connection_out *out)
{
	struct addrinfo hint;
	char *p;

	if (out->name == NULL) {	// use defaults
		out->host = owstrdup("0.0.0.0");
		out->service = owstrdup(default_port);
	} else if ((p = strrchr(out->name, ':')) == NULL) {
		if (strchr(out->name, '.')) {	//probably an address
			out->host = owstrdup(out->name);
			out->service = owstrdup(default_port);
		} else {				// assume a port
			out->host = owstrdup("0.0.0.0");
			out->service = owstrdup(out->name);
		}
	} else {
		p[0] = '\0';
		out->host = owstrdup(out->name);
		p[0] = ':';
		out->service = owstrdup(&p[1]);
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

	if ( getaddrinfo(out->host, out->service, &hint, &out->ai) != 0 ) {
		ERROR_CONNECT("GetAddrInfo error [%s]=%s:%s", SAFESTRING(out->name), SAFESTRING(out->host), SAFESTRING(out->service));
		return gbBAD;
	}
	return gbGOOD;
}

static FILE_DESCRIPTOR_OR_ERROR ServerListen(struct connection_out *out)
{
	if (out->ai == NULL) {
		LEVEL_CONNECT("Server address not yet parsed [%s]", SAFESTRING(out->name));
		return FILE_DESCRIPTOR_BAD;
	}

	if (out->ai_ok == NULL) {
		out->ai_ok = out->ai;
	}

	do {
		int on = 1;
		FILE_DESCRIPTOR_OR_ERROR file_descriptor = socket(out->ai_ok->ai_family, out->ai_ok->ai_socktype, out->ai_ok->ai_protocol);

		//printf("ServerListen file_descriptor=%d\n",file_descriptor);
		if (file_descriptor < 0) {
			ERROR_CONNECT("Socket problem [%s]", SAFESTRING(out->name));
		} else if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) != 0) {
			ERROR_CONNECT("SetSockOpt problem [%s]", SAFESTRING(out->name));
		} else if (bind(file_descriptor, out->ai_ok->ai_addr, out->ai_ok->ai_addrlen) != 0) {
			// this is where the default linking to a busy port shows up
			ERROR_CONNECT("Bind problem [%s]", SAFESTRING(out->name));
		} else if (listen(file_descriptor, 10) != 0) {
			ERROR_CONNECT("Listen problem [%s]", SAFESTRING(out->name));
		} else {
			out->file_descriptor = file_descriptor;
			return file_descriptor;
		}
		if ( file_descriptor >= 0 ) {
			close( file_descriptor );
		}
	} while ((out->ai_ok = out->ai_ok->ai_next));
	LEVEL_CONNECT("No good listen network sockets [%s]", SAFESTRING(out->name));
	return FILE_DESCRIPTOR_BAD;
}

GOOD_OR_BAD ServerOutSetup(struct connection_out *out)
{
	if ( out->name == NULL ) { // NULL name means default attempt
		char * default_port ;
		// First time through, try default port
		switch (Globals.opt) {
			case opt_server:
				default_port = DEFAULT_PORT ;
				break ;
			case opt_ftpd:
				default_port = "21" ;
				break ;
			default:
				default_port = NULL ;
				break ;
		}
		if ( default_port != NULL ) { // one of the 2 cases above
			RETURN_BAD_IF_BAD( ServerAddr( default_port, out ) ) ;
			if ( ServerListen(out)  != FILE_DESCRIPTOR_BAD ) {
				return gbGOOD ;
			}
			ERROR_CONNECT("Default port not successful. Try an ephemeral port");
		}
	}

	// second time through, use ephemeral port
	RETURN_BAD_IF_BAD( ServerAddr( "0", out ) ) ;

	return (ServerListen(out) == FILE_DESCRIPTOR_BAD) ? gbBAD : gbGOOD ;
}

static FILE_DESCRIPTOR_OR_ERROR SetupListenSet( fd_set * listenset )
{
	int maxfd = -1 ;
	struct connection_out * out ;

	FD_ZERO( listenset ) ;
	for (out = Outbound_Control.head; out; out = out->next) {
		FILE_DESCRIPTOR_OR_ERROR fd = out->file_descriptor ;
		if ( fd != FILE_DESCRIPTOR_BAD ) {
			FD_SET( fd, listenset ) ;
			if ( fd > maxfd ) {
				maxfd = fd ;
			}
		}
	}
	return maxfd ;
}

static GOOD_OR_BAD SetupListenSockets( void (*HandlerRoutine) (int file_descriptor) )
{
	struct connection_out * out ;
	GOOD_OR_BAD any_sockets = gbBAD ;

	for (out = Outbound_Control.head; out; out = out->next) {
		if ( GOOD( ServerOutSetup( out ) ) ) {
			any_sockets = gbGOOD;
			ZeroConf_Announce(out);
		}
		out-> HandlerRoutine = HandlerRoutine ;
	}
	return any_sockets ;
}

static void CloseListenSockets( void )
{
	struct connection_out * out ;

	for (out = Outbound_Control.head; out; out = out->next) {
		Test_and_Close( &(out->file_descriptor) ) ;
	}
}

static void ProcessListenSet( fd_set * listenset )
{
	struct connection_out * out ;

	for (out = Outbound_Control.head; out; out = out->next) {
		printf("Listen loop\n") ;
		if ( FD_ISSET( out->file_descriptor, listenset ) ) {
			printf("%d is set\n",out->file_descriptor) ;
			ProcessListenSocket( out ) ;
		}
	}
}

/* structure */
struct Accept_Socket_Data {
	FILE_DESCRIPTOR_OR_ERROR acceptfd;
	struct connection_out * out;
};

static void *ProcessAcceptSocket(void *arg)
{
	struct Accept_Socket_Data * asd = (struct Accept_Socket_Data *) arg;
	pthread_detach(pthread_self());

	asd->out->HandlerRoutine( asd->acceptfd );

	// cleanup
	Test_and_Close( &(asd->acceptfd) );
	owfree(asd);
	LEVEL_DEBUG("Normal exit.");
#if OW_MT
	my_rwlock_read_lock( &shutdown_mutex_rw ) ;
	pthread_mutex_lock( &handler_thread_mutex ) ;
	--handler_thread_count ;
	if ( shutdown_in_progress && handler_thread_count==0) {
		if ( shutdown_pipe[fd_pipe_write] != FILE_DESCRIPTOR_BAD ) {
			write( shutdown_pipe[fd_pipe_write],"X",1) ; //dummy payload
		}		
	}
	pthread_mutex_unlock( &handler_thread_mutex ) ;
	my_rwlock_read_unlock( &shutdown_mutex_rw ) ;
#endif /* OW_MT */
	return NULL;
}

static void ProcessListenSocket( struct connection_out * out )
{
	FILE_DESCRIPTOR_OR_ERROR acceptfd;
	struct Accept_Socket_Data * asd ;

	acceptfd = accept(out->file_descriptor, NULL, NULL);

	if (acceptfd == FILE_DESCRIPTOR_BAD ) {
		return ;
	}

	asd = owmalloc( sizeof(struct Accept_Socket_Data) ) ;
	if ( asd == NULL ) {
		close( acceptfd ) ;
		return ;
	}
	asd->acceptfd = acceptfd ;
	asd->out = out ;
	printf("Prethread\n") ;
	printf("asd=%p\n",asd) ;
#if OW_MT
	my_rwlock_read_lock( &shutdown_mutex_rw ) ;
	if ( ! shutdown_in_progress ) {
		pthread_t tid;
		pthread_mutex_lock( &handler_thread_mutex ) ;
		++handler_thread_count ;
		pthread_mutex_unlock( &handler_thread_mutex ) ;
		if ( pthread_create(&tid, NULL, ProcessAcceptSocket, asd ) != 0 ) {
			// Do it in the main routine rather than a thread
			ProcessAcceptSocket(asd) ;
		}
	}
	my_rwlock_read_unlock( &shutdown_mutex_rw ) ;
#else /* OW_MT */
	ProcessAcceptSocket(asd) ;
#endif
}

static GOOD_OR_BAD ListenCycle( void )
{
	fd_set listenset ;
	FILE_DESCRIPTOR_OR_ERROR maxfd = SetupListenSet( &listenset) ;
	if ( maxfd != FILE_DESCRIPTOR_BAD) {
		if ( select( maxfd+1, &listenset, NULL, NULL, NULL) > 0 ) {
			printf("postselect\n");
			ProcessListenSet( &listenset) ;
			return gbGOOD ;
		}
	}
	return gbBAD ;
}

/* Setup Servers -- select on each port */
void ServerProcess(void (*HandlerRoutine) (int file_descriptor))
{

#if OW_MT
	/* Locking for thread work */
	int dont_need_to_read_pipe ;

	handler_thread_count = 0 ;
	shutdown_in_progress = 0 ;
	shutdown_pipe[fd_pipe_read] = FILE_DESCRIPTOR_BAD ;
	shutdown_pipe[fd_pipe_write] = FILE_DESCRIPTOR_BAD ;

	my_rwlock_init( &shutdown_mutex_rw ) ;
	my_pthread_mutex_init(&handler_thread_mutex, Mutex.pmattr);
	if ( pipe( shutdown_pipe ) != 0 ) {
		ERROR_DEFAULT("Cannot allocate a shutdown pipe. The program shutdown may be messy");
	}
#endif /* OW_MT */

	if ( GOOD( SetupListenSockets( HandlerRoutine ) ) ) {
		while (	GOOD( ListenCycle() ) ) {
		}
#if OW_MT
		my_rwlock_write_lock( &shutdown_mutex_rw ) ;
		shutdown_in_progress = 1 ; // Signal time to wrap up
		dont_need_to_read_pipe = handler_thread_count==0 || shutdown_pipe[fd_pipe_read]==FILE_DESCRIPTOR_BAD  ;
		my_rwlock_write_unlock( &shutdown_mutex_rw ) ;
		if ( ! dont_need_to_read_pipe ) {
			char buf[2] ;
			read( shutdown_pipe[fd_pipe_read],buf,1) ;
		}
		Test_and_Close(&shutdown_pipe[fd_pipe_read]) ;
		Test_and_Close(&shutdown_pipe[fd_pipe_write]) ;
		my_rwlock_destroy(&shutdown_mutex_rw) ;
		pthread_mutex_destroy(&handler_thread_mutex) ;
#endif /* OW_MT */
		CloseListenSockets() ;
	} else {
		LEVEL_DEFAULT("Isolated from any control -- exit") ;
	}
	/* Cleanup that may never be reached */
	return;
}
