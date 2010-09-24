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

/* Locking for thread work */
/* Variables only used in this particular file */
/* i.e. "locally global" */
my_rwlock_t shutdown_mutex_rw ;
pthread_mutex_t handler_thread_mutex ;
int handler_thread_count ;
int shutdown_in_progress ;
FILE_DESCRIPTOR_OR_ERROR shutdown_pipe[2] ;


/* Prototypes */
static GOOD_OR_BAD ServerAddr(const char * default_port, struct connection_out *out);
static FILE_DESCRIPTOR_OR_ERROR ServerListen(struct connection_out *out);

static FILE_DESCRIPTOR_OR_ERROR SetupListenSet( fd_set * listenset ) ;
static GOOD_OR_BAD SetupListenSockets( void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor) ) ;
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
		if ( FILE_DESCRIPTOR_NOT_VALID(file_descriptor) ) {
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
		Test_and_Close(&file_descriptor) ;
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
				default_port = DEFAULT_SERVER_PORT ;
				break ;
			case opt_ftpd:
				default_port = DEFAULT_FTP_PORT ;
				break ;
			default:
				default_port = NULL ;
				break ;
		}
		if ( default_port != NULL ) { // one of the 2 cases above
			RETURN_BAD_IF_BAD( ServerAddr( default_port, out ) ) ;
			if ( FILE_DESCRIPTOR_VALID(ServerListen(out)) ) {
				return gbGOOD ;
			}
			ERROR_CONNECT("Default port not successful. Try an ephemeral port");
		}
	}

	// second time through, use ephemeral port
	RETURN_BAD_IF_BAD( ServerAddr( "0", out ) ) ;

	return FILE_DESCRIPTOR_VALID(ServerListen(out)) ? gbGOOD : gbBAD ;
}

static FILE_DESCRIPTOR_OR_ERROR SetupListenSet( fd_set * listenset )
{
	FILE_DESCRIPTOR_OR_ERROR maxfd = FILE_DESCRIPTOR_BAD ;
	struct connection_out * out ;

	FD_ZERO( listenset ) ;
	for (out = Outbound_Control.head; out; out = out->next) {
		FILE_DESCRIPTOR_OR_ERROR fd = out->file_descriptor ;
		if ( FILE_DESCRIPTOR_VALID( fd ) ) {
			FD_SET( fd, listenset ) ;
			if ( fd > maxfd ) {
				maxfd = fd ;
			}
		}
	}
	return maxfd ;
}

static GOOD_OR_BAD SetupListenSockets( void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor) )
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
		if ( FD_ISSET( out->file_descriptor, listenset ) ) {
			ProcessListenSocket( out ) ;
		}
	}
}

/* structure */
struct Accept_Socket_Data {
	FILE_DESCRIPTOR_OR_ERROR acceptfd;
	struct connection_out * out;
};

static GOOD_OR_BAD ListenCycle( void )
{
	fd_set listenset ;
	FILE_DESCRIPTOR_OR_ERROR maxfd = SetupListenSet( &listenset) ;
	if ( FILE_DESCRIPTOR_VALID( maxfd ) ) {
		if ( select( maxfd+1, &listenset, NULL, NULL, NULL) > 0 ) {
			ProcessListenSet( &listenset) ;
			return gbGOOD ;
		}
	}
	return gbBAD ;
}

#if OW_MT

// Read data from the waiting socket and do the actual work
static void *ProcessAcceptSocket(void *arg)
{
	struct Accept_Socket_Data * asd = (struct Accept_Socket_Data *) arg;
	DETACH_THREAD;

	// Do the actual work
	asd->out->HandlerRoutine( asd->acceptfd );

	// cleanup
	Test_and_Close( &(asd->acceptfd) );
	owfree(asd);
	LEVEL_DEBUG("Normal exit.");

	// All done. If shutdown in progress and this is a last handler thread, send a message to the main thread.
	RWLOCK_RLOCK( shutdown_mutex_rw ) ;
	_MUTEX_LOCK( handler_thread_mutex ) ;
	--handler_thread_count ;
	if ( shutdown_in_progress && handler_thread_count==0) {
		if ( FILE_DESCRIPTOR_VALID( shutdown_pipe[fd_pipe_write] ) ) {
			ignore_result = write( shutdown_pipe[fd_pipe_write],"X",1) ; //dummy payload
		}		
	}
	_MUTEX_UNLOCK( handler_thread_mutex ) ;
	RWLOCK_RUNLOCK( shutdown_mutex_rw ) ;

	return NULL;
}

static void ProcessListenSocket( struct connection_out * out )
{
	FILE_DESCRIPTOR_OR_ERROR acceptfd;
	struct Accept_Socket_Data * asd ;

	acceptfd = accept(out->file_descriptor, NULL, NULL);

	if ( FILE_DESCRIPTOR_NOT_VALID( acceptfd ) ) {
		return ;
	}

	// allocate space to pass variables to thread -- cleaned up in thread handler
	asd = owmalloc( sizeof(struct Accept_Socket_Data) ) ;
	if ( asd == NULL ) {
		close( acceptfd ) ;
		return ;
	}
	asd->acceptfd = acceptfd ;
	asd->out = out ;

	// Launch Handler thread only if shutdown not in progress
	RWLOCK_RLOCK( shutdown_mutex_rw ) ;
	if ( ! shutdown_in_progress ) {
		pthread_t tid;
		_MUTEX_LOCK( handler_thread_mutex ) ;
		++handler_thread_count ;
		_MUTEX_UNLOCK( handler_thread_mutex ) ;
		if ( pthread_create(&tid, DEFAULT_THREAD_ATTR, ProcessAcceptSocket, asd ) != 0 ) {
			// Do it in the main routine rather than a thread
			ProcessAcceptSocket(asd) ;
		}
	}
	RWLOCK_RUNLOCK( shutdown_mutex_rw ) ;
}

/* Setup Servers -- select on each port */
void ServerProcess(void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor))
{
	/* Locking for thread work */
	int need_to_read_pipe ;

	handler_thread_count = 0 ;
	shutdown_in_progress = 0 ;
	shutdown_pipe[fd_pipe_read] = FILE_DESCRIPTOR_BAD ;
	shutdown_pipe[fd_pipe_write] = FILE_DESCRIPTOR_BAD ;

	RWLOCK_INIT( shutdown_mutex_rw ) ;
	_MUTEX_INIT(handler_thread_mutex);

	if ( pipe( shutdown_pipe ) != 0 ) {
		ERROR_DEFAULT("Cannot allocate a shutdown pipe. The program shutdown may be messy");
	}

	if ( GOOD( SetupListenSockets( HandlerRoutine ) ) ) {
		while (	GOOD( ListenCycle() ) ) {
		}

		// Make sure all the handler threads are complete before closing down
		RWLOCK_WLOCK( shutdown_mutex_rw ) ;
		shutdown_in_progress = 1 ; // Signal time to wrap up
		// need to test if there is a handler to wait for before closing
		// note that a new one can't be started while the write lock is held
		need_to_read_pipe = handler_thread_count>0 && FILE_DESCRIPTOR_VALID( shutdown_pipe[fd_pipe_read] )  ;
		RWLOCK_WUNLOCK( shutdown_mutex_rw ) ;
		
		if ( need_to_read_pipe ) {
			// now wait for write to pipe from last handler thread
			char buf[2] ;
			ignore_result = read( shutdown_pipe[fd_pipe_read],buf,1) ;
		}
		Test_and_Close_Pipe(shutdown_pipe) ;
		RWLOCK_DESTROY(shutdown_mutex_rw) ;
		_MUTEX_DESTROY(handler_thread_mutex) ;
		CloseListenSockets() ;

	} else {
		LEVEL_DEFAULT("Isolated from any control -- exit") ;
	}
	/* Cleanup that may never be reached */
	return;
}

#else /* OW_MT */

// Read data from the waiting socket and do the actual work
static void *ProcessAcceptSocket(void *arg)
{
	struct Accept_Socket_Data * asd = (struct Accept_Socket_Data *) arg;

	// Do the actual work
	asd->out->HandlerRoutine( asd->acceptfd );

	// cleanup
	Test_and_Close( &(asd->acceptfd) );
	owfree(asd);
	LEVEL_DEBUG("Normal exit.");

	return NULL;
}

static void ProcessListenSocket( struct connection_out * out )
{
	FILE_DESCRIPTOR_OR_ERROR acceptfd;
	struct Accept_Socket_Data * asd ;

	acceptfd = accept(out->file_descriptor, NULL, NULL);

	if ( FILE_DESCRIPTOR_NOT_VALID( acceptfd ) ) {
		return ;
	}

	// allocate space to pass variables to thread -- cleaned up in thread handler
	asd = owmalloc( sizeof(struct Accept_Socket_Data) ) ;
	if ( asd == NULL ) {
		close( acceptfd ) ;
		return ;
	}
	asd->acceptfd = acceptfd ;
	asd->out = out ;

	ProcessAcceptSocket(asd) ;
}

/* Setup Servers -- select on each port */
void ServerProcess(void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor))
{
	if ( GOOD( SetupListenSockets( HandlerRoutine ) ) ) {
		while (	GOOD( ListenCycle() ) ) {
		}
		CloseListenSockets() ;
	} else {
		LEVEL_DEFAULT("Isolated from any control -- exit") ;
	}
	/* Cleanup that may never be reached */
	return;
}

#endif /* OW_MT */

