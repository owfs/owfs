/*

$Id$
 
This is the code that waits for client connections and creates the
threads that handle them.  When ftp_listener_init() is called, it binds
to the appropriate socket and sets up the other values for the
structure.  Then, when ftp_listener_start() is called, a thread is
started dedicatd to accepting connections.

This thread waits for input on two file descriptors.  If input arrives
on the socket, then it accepts the connection and creates a thread to
handle it.  If input arrives on the shutdown_request pipe, then the
thread terminates.  This is how ftp_listener_stop() signals the listener
to end.

*/

#include "owftpd.h"
#include <arpa/inet.h>
#include <limits.h>

/* maximum number of consecutive errors in accept()
   before we terminate listener                     */
#define MAX_ACCEPT_ERROR 10

/* buffer to hold address string */
#define ADDR_BUF_LEN 100

/* information for a specific connection */
struct connection_info_s {
	struct ftp_listener_s *ftp_listener;
	struct telnet_session_s telnet_session;
	struct ftp_session_s ftp_session;
	struct watched_s watched;

	struct connection_info *next;
};

/* prototypes */
static int invariant(const struct ftp_listener_s *f);
static void *connection_acceptor(void *v);
static char *addr2string(const sockaddr_storage_t * s);
static void *connection_handler(void *v);
static void connection_handler_cleanup(void *v);

/* initialize an FTP listener */
/* ftp uses 0 as an error return */
int ftp_listener_init(struct ftp_listener_s *f)
{
	int flags;
	int pipefds[2];

	daemon_assert(f != NULL);

	// For sone reason, there must be an IP address included
	if (strchr(head_outbound_list->name, ':') == NULL) {
		char *newname;
		char *oldname = head_outbound_list->name;
		newname = malloc(8 + strlen(oldname));
		if (newname == NULL) {
			LEVEL_DEFAULT("Cannot allocate menory for port name\n");
			return 0;
		}
		strcpy(newname, "0.0.0.0:");
		strcat(newname, oldname);
		//printf("OWSERVER composite name <%s> -> <%s>\n",oldname,newname);
		head_outbound_list->name = newname;
		free(oldname);
	}

	if (ServerOutSetup(head_outbound_list))
		return 0;

	// Zeroconf/Bonjour start (since owftpd doesn't use ServerProcess yet)
	OW_Announce(head_outbound_list);

	/* prevent socket from blocking on accept() */
	flags = fcntl(head_outbound_list->file_descriptor, F_GETFL);
	if (flags == -1) {
		ERROR_CONNECT("Error getting flags on socket\n");
		return 0;
	}
	if (fcntl(head_outbound_list->file_descriptor, F_SETFL, flags | O_NONBLOCK) != 0) {
		ERROR_CONNECT("Error setting socket to non-blocking\n");
		return 0;
	}

	/* create a pipe to wake up our listening thread */
	if (pipe(pipefds) != 0) {
		ERROR_CONNECT("Error creating pipe for internal use\n");
		return 0;
	}

	/* now load the values into the structure, since we can't fail from
	   here */
	f->file_descriptor = head_outbound_list->file_descriptor;
	f->max_connections = Globals.max_clients;
	f->num_connections = 0;
	f->inactivity_timeout = Globals.timeout_ftp;
	pthread_mutex_init(&f->mutex, NULL);

	strcpy(f->dir, "/");
	f->listener_running = 0;

	f->shutdown_request_send_fd = pipefds[1];
	f->shutdown_request_recv_fd = pipefds[0];
	pthread_cond_init(&f->shutdown_cond, NULL);

	daemon_assert(invariant(f));
	return 1;
}

/* receive connections */
int ftp_listener_start(struct ftp_listener_s *f)
{
	pthread_t thread_id;
	int ret_val;
	int error_code;

	daemon_assert(invariant(f));

	error_code = pthread_create(&thread_id, NULL, connection_acceptor, f);

	if (error_code == 0) {
		f->listener_running = 1;
		f->listener_thread = thread_id;
		ret_val = 1;
	} else {
		errno = error_code;
		ERROR_CONNECT("Unable to create thread\n");
		ret_val = 0;
	}

	daemon_assert(invariant(f));

	return ret_val;
}


#ifndef NDEBUG
static int invariant(const struct ftp_listener_s *f)
{
	int dir_len;

	if (f == NULL) {
		return 0;
	}
	if (f->file_descriptor < 0) {
		return 0;
	}
	if (f->max_connections <= 0) {
		return 0;
	}
	if ((f->num_connections < 0)
		|| (f->num_connections > f->max_connections)) {
		return 0;
	}
	dir_len = strlen(f->dir);
	if ((dir_len <= 0) || (dir_len > PATH_MAX)) {
		return 0;
	}
	if (f->shutdown_request_send_fd < 0) {
		return 0;
	}
	if (f->shutdown_request_recv_fd < 0) {
		return 0;
	}
	return 1;
}
#endif							/* NDEBUG */

/* handle incoming connections */
static void *connection_acceptor(void *v)
{
	struct ftp_listener_s *f = (struct ftp_listener_s *) v;
	int num_error;

	int file_descriptor;
	int tcp_nodelay;
	sockaddr_storage_t client_addr;
	sockaddr_storage_t server_addr;
	unsigned addr_len;

	struct connection_info_s *info;
	pthread_t thread_id;
	int error_code;

	fd_set readfds;

	daemon_assert(invariant(f));

	if (!watchdog_init(&f->watchdog, f->inactivity_timeout)) {
		LEVEL_CONNECT("Error initializing watchdog thread\n");
		return NULL;
	}

	num_error = 0;
	for (;;) {

		/* wait for something to happen */
		FD_ZERO(&readfds);
		FD_SET(f->file_descriptor, &readfds);
		FD_SET(f->shutdown_request_recv_fd, &readfds);
		select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

		/* if data arrived on our pipe, we've been asked to exit */
		if (FD_ISSET(f->shutdown_request_recv_fd, &readfds)) {
			close(f->file_descriptor);
			LEVEL_CONNECT("Listener no longer accepting connections\n");
			pthread_exit(NULL);
		}

		/* otherwise accept our pending connection (if any) */
		addr_len = sizeof(sockaddr_storage_t);
		file_descriptor = accept(f->file_descriptor, (struct sockaddr *) &client_addr, &addr_len);
		if (file_descriptor >= 0) {
			tcp_nodelay = 1;
			if (setsockopt(file_descriptor, IPPROTO_TCP, TCP_NODELAY, (void *) &tcp_nodelay, sizeof(int)) != 0) {
				ERROR_CONNECT("Error in setsockopt(), FTP server dropping connection\n");
				close(file_descriptor);
				continue;
			}

			addr_len = sizeof(sockaddr_storage_t);
			if (getsockname(file_descriptor, (struct sockaddr *) &server_addr, &addr_len) == -1) {
				ERROR_CONNECT("Error in getsockname(), FTP server dropping connection\n");
				close(file_descriptor);
				continue;
			}

			info = (struct connection_info_s *)
				malloc(sizeof(struct connection_info_s));
			if (info == NULL) {
				LEVEL_CONNECT("Out of memory, FTP server dropping connection\n");
				close(file_descriptor);
				continue;
			}
			info->ftp_listener = f;

			telnet_session_init(&info->telnet_session, file_descriptor, file_descriptor);

			if (!ftp_session_init(&info->ftp_session, &client_addr, &server_addr, &info->telnet_session, f->dir)) {
				LEVEL_CONNECT("Eerror initializing FTP session, FTP server exiting\n");
				close(file_descriptor);
				telnet_session_destroy(&info->telnet_session);
				free(info);
				continue;
			}


			error_code = pthread_create(&thread_id, NULL, connection_handler, info);

			if (error_code != 0) {
				errno = error_code;
				ERROR_CONNECT("Error creating new thread\n");
				close(file_descriptor);
				telnet_session_destroy(&info->telnet_session);
				free(info);
			}

			num_error = 0;
		} else {
			if ((errno == ECONNABORTED) || (errno == ECONNRESET)) {
				ERROR_CONNECT("Interruption accepting FTP connection\n");
			} else {
				ERROR_CONNECT("Error accepting FTP connection\n");
				++num_error;
			}
			if (num_error >= MAX_ACCEPT_ERROR) {
				LEVEL_CONNECT("Too many consecutive errors, FTP server exiting\n");
				return NULL;
			}
		}
	}
}

/* convert an address to a printable string */
/* NOT THREADSAFE - wrap with a mutex before calling! */
static char *addr2string(const sockaddr_storage_t * s)
{
	char *ret_val;

	daemon_assert(s != NULL);

#ifdef INET6
	{
		static char addr[IP_ADDRSTRLEN + 1];
		int error;
		error = getnameinfo((struct sockaddr *) s, sizeof(sockaddr_storage_t), addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
		if (error != 0) {
			LEVEL_CONNECT("getnameinfo error; %s\n", gai_strerror(error));
			ret_val = "Unknown IP";
		} else {
			ret_val = addr;
		}
	}
#else
	ret_val = inet_ntoa(s->sin_addr);
#endif

	return ret_val;
}


static void *connection_handler(void *v)
{
	struct connection_info_s *info = (struct connection_info_s *) v;
	struct ftp_listener_s *f;
	int num_connections;
	char drop_reason[80];

	/* for ease of use only */
	f = info->ftp_listener;

	/* don't save state for pthread_join() */
	pthread_detach(pthread_self());

	/* set up our watchdog */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	watchdog_add_watched(&f->watchdog, &info->watched);

	/* set up our cleanup handler */
	pthread_cleanup_push(connection_handler_cleanup, info);

	/* process global data */
	pthread_mutex_lock(&f->mutex);
	num_connections = ++f->num_connections;
	ERROR_DEBUG("%s port %d connection\n", addr2string(&info->ftp_session.client_addr), ntohs(SINPORT(&info->ftp_session.client_addr)));
	pthread_mutex_unlock(&f->mutex);

	/* handle the session */
	if (num_connections <= f->max_connections) {

		ftp_session_run(&info->ftp_session, &info->watched);

	} else {

		/* too many users */
		sprintf(drop_reason, "Too many users logged in, dropping connection (%d logins maximum)", f->max_connections);
		ftp_session_drop(&info->ftp_session, drop_reason);

		/* log the rejection */
		pthread_mutex_lock(&f->mutex);
		LEVEL_CONNECT
			("%s port %d exceeds max users (%d), dropping connection\n",
			 addr2string(&info->ftp_session.client_addr), ntohs(SINPORT(&info->ftp_session.client_addr)), num_connections);
		pthread_mutex_unlock(&f->mutex);

	}

	/* exunt (pop calls cleanup function) */
	pthread_cleanup_pop(1);

	/* return for form :) */
	return NULL;
}

/* clean up a connection */
static void connection_handler_cleanup(void *v)
{
	struct connection_info_s *info = (struct connection_info_s *) v;
	struct ftp_listener_s *f;

	f = info->ftp_listener;

	watchdog_remove_watched(&info->watched);

	pthread_mutex_lock(&f->mutex);

	f->num_connections--;
	pthread_cond_signal(&f->shutdown_cond);

	LEVEL_CONNECT("%s port %d disconnected\n", addr2string(&info->ftp_session.client_addr), ntohs(SINPORT(&info->ftp_session.client_addr)));

	pthread_mutex_unlock(&f->mutex);

	ftp_session_destroy(&info->ftp_session);
	telnet_session_destroy(&info->telnet_session);

	free(info);
}

void ftp_listener_stop(struct ftp_listener_s *f)
{
	daemon_assert(invariant(f));

	/* write a byte to the listening thread - this will wake it up */
	write(f->shutdown_request_send_fd, "", 1);

	/* wait for client connections to complete */
	pthread_mutex_lock(&f->mutex);
	while (f->num_connections > 0) {
		pthread_cond_wait(&f->shutdown_cond, &f->mutex);
	}
	pthread_mutex_unlock(&f->mutex);
}
