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

#include <owftpd.h>
//#include <features.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <string.h>
//#include <errno.h>
//#include <unistd.h>
//#include <pthread.h>
//#include <stdlib.h>
//#include <limits.h>
//#include <syslog.h>
//#include <stdio.h>
//#include <netdb.h>
//#include <netinet/tcp.h>
//#include <fcntl.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

//#include "daemon_assert.h"
//#include "telnet_session.h"
//#include "ftp_session.h"
//#include "ftp_listener.h"
//#include "watchdog.h"
//#include "af_portability.h"



/* maximum number of consecutive errors in accept()
   before we terminate listener                     */
#define MAX_ACCEPT_ERROR 10

/* buffer to hold address string */
#define ADDR_BUF_LEN 100

/* information for a specific connection */
typedef struct connection_info {
    struct ftp_listener_t *ftp_listener;
    struct telnet_session_t telnet_session;
    struct ftp_session_t ftp_session;
    watched_t watched;

    struct connection_info *next;
} connection_info_t;

/* prototypes */
static int invariant(const struct ftp_listener_t *f);
static void *connection_handler(void * v);
static void connection_handler_cleanup(void * v);

/* initialize an FTP listener */
int ftp_listener_init(struct ftp_listener_t *f, 
                      char *address, 
                      int port, 
                      int max_connections,
                      int inactivity_timeout,
                      error_code_t *err)
{
    sockaddr_storage_t sock_addr;
    int fd;
    int flags;
    int pipefds[2];
    int reuseaddr;
    char dir[PATH_MAX+1];
    char buf[ADDR_BUF_LEN+1];
    const char *inet_ntop_ret;

    daemon_assert(f != NULL);
    daemon_assert(port >= 0);
    daemon_assert(port < 65536);
    daemon_assert(max_connections > 0);
    daemon_assert(err != NULL);

    dir[0]  = '/'; /* first dir is always root dir */
    dir[1]  = '\000';

    /* set up our socket address */
    memset(&sock_addr, 0, sizeof(sockaddr_storage_t));

    if (address == NULL) {
#ifdef INET6
        SAFAM(&sock_addr) = AF_INET6;
        memcpy(&SIN6ADDR(&sock_addr), &in6addr_any, sizeof(struct in6_addr));
#else
        SAFAM(&sock_addr) = AF_INET;
        sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
    } else {
        int gai_err;
        struct addrinfo hints;
        struct addrinfo *res;

        memset(&hints, 0, sizeof(hints));

/* - This code should be able to parse both IPv4 and IPv6 internet
 * addresses and put them in the sock_addr variable.
 * - Much neater now.
 * - Bug: Can't handle hostnames, only IP addresses.  Not sure
 * exactly why.  But then again, I'm not sure exactly why
 * there is a man page for getipnodebyname (which getaddrinfo
 * says it uses) but no corresponding function in libc.
 * -- Matthew Danish [3/20/2001]
 */

#ifdef INET6
        hints.ai_family = AF_INET6;
#else
        hints.ai_family = AF_INET;
#endif

        hints.ai_flags  = AI_PASSIVE;

        gai_err = getaddrinfo(address, NULL, &hints, &res);
        if (gai_err != 0) {
            error_init(err, gai_err, "error parsing server socket address; %s", gai_strerror(gai_err));
            return 0;
        }

        memcpy(&sock_addr, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
    } 

    if (port == 0) {
        SINPORT(&sock_addr) = htons(DEFAULT_FTP_PORT);
    } else {
        SINPORT(&sock_addr) = htons(port);
    }


    inet_ntop_ret = inet_ntop(SAFAM(&sock_addr), 
                              (void *)&SINADDR(&sock_addr), 
                              buf, 
                              sizeof(buf));
    if (inet_ntop_ret == NULL) {
        error_init(err, errno, "error converting server address to ASCII; %s", strerror(errno));
        return 0;
    }
    
    /* Put some information in syslog */
    LEVEL_CONNECT("Binding interface '%s', port %d, max clients %d\n", buf, ntohs(SINPORT(&sock_addr)), max_connections)
    

    /* okay, finally do some socket manipulation */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        error_init(err, errno, "error creating socket; %s", strerror(errno));
        return 0;
    }

    reuseaddr = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseaddr, sizeof(int)) !=0) {
        close(fd);
        error_init(err, errno, "error setting socket to reuse address; %s", strerror(errno));
        return 0;
    }

    if (bind(fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in)) != 0) {
        close(fd);
        error_init(err, errno, "error binding address; %s", strerror(errno));
        return 0;
    }

    if (listen(fd, SOMAXCONN) != 0) {
        close(fd);
        error_init(err, errno, "error setting socket to listen; %s", strerror(errno));
        return 0;
    }

    /* prevent socket from blocking on accept() */
    flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        close(fd);
        error_init(err, errno, "error getting flags on socket; %s", strerror(errno));
        return 0;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
        close(fd);
        error_init(err, errno, "error setting socket to non-blocking; %s", strerror(errno));
        return 0;
    }

    /* create a pipe to wake up our listening thread */
    if (pipe(pipefds) != 0) {
        close(fd);
        error_init(err, errno, "error creating pipe for internal use; %s", strerror(errno));
        return 0;
    }

    /* now load the values into the structure, since we can't fail from
       here */
    f->fd = fd;
    f->max_connections = max_connections;
    f->num_connections = 0;
    f->inactivity_timeout = inactivity_timeout;
    pthread_mutex_init(&f->mutex, pmattr);

    daemon_assert(strlen(dir) < sizeof(f->dir));
    strcpy(f->dir, dir);
    f->listener_running = 0;

    f->shutdown_request_send_fd = pipefds[1];
    f->shutdown_request_recv_fd = pipefds[0];
    pthread_cond_init(&f->shutdown_cond, NULL);

    daemon_assert(invariant(f));
    return 1;
}

/* receive connections */
int ftp_listener_start(struct ftp_listener_t *f, error_code_t *err)
{
    pthread_t thread_id;
    int ret_val;
    int error_code;

    daemon_assert(invariant(f));
    daemon_assert(err != NULL);

//    error_code = pthread_create(&thread_id, NULL, (void *(*)())connection_acceptor, f);
    error_code = pthread_create(&thread_id, NULL, connection_acceptor, f);

    if (error_code == 0) {
        f->listener_running = 1;
        f->listener_thread = thread_id;
        ret_val = 1;
    } else {
        error_init(err, error_code, "unable to create thread");
        ret_val = 0;
    }

    daemon_assert(invariant(f));

    return ret_val;
}


#ifndef NDEBUG
static int invariant(const struct ftp_listener_t *f) {
    int dir_len;

    if ( (f == NULL)
          ||
       (f->fd < 0)
          ||
       (f->max_connections <= 0)
          ||
       (f->num_connections < 0)
          ||
       (f->num_connections > f->max_connections)
          ||
       ((dir_len=strlen(f->dir)) <= 0)
          ||
       (dir_len > PATH_MAX)
          ||
       (f->shutdown_request_send_fd < 0)
          ||
       (f->shutdown_request_recv_fd < 0)
       ) 
        return 0;
    return 1;
}
#endif /* NDEBUG */

/* handle incoming connections */
void *connection_acceptor(void * v) {
    struct ftp_listener_t *f = (struct ftp_listener_t *) v ;
    error_code_t err;
    int num_error;

    int fd;
    int tcp_nodelay;
    sockaddr_storage_t client_addr;
    sockaddr_storage_t server_addr;
    unsigned addr_len;
    
    connection_info_t *info;
    pthread_t thread_id;
    int error_code;

    fd_set readfds;

    daemon_assert(invariant(f));

    if (!watchdog_init(&f->watchdog, f->inactivity_timeout, &err)) {
        LEVEL_CONNECT("Error initializing watchdog thread; %s",error_get_desc(&err));
        return NULL;
    }

    num_error = 0;
    for (;;) {

         /* wait for something to happen */
         FD_ZERO(&readfds);
         FD_SET(f->fd, &readfds);
         FD_SET(f->shutdown_request_recv_fd, &readfds);
         select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

         /* if data arrived on our pipe, we've been asked to exit */
         if (FD_ISSET(f->shutdown_request_recv_fd, &readfds)) {
             close(f->fd);
             LEVEL_CONNECT("listener no longer accepting connections");
             pthread_exit(NULL);
         }

         /* otherwise accept our pending connection (if any) */
         addr_len = sizeof(sockaddr_storage_t);
         fd = accept(f->fd, (struct sockaddr *)&client_addr, &addr_len);
         if (fd >= 0) { // Successful "accept"
            tcp_nodelay = 1;
            if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&tcp_nodelay, sizeof(int)) != 0) {
                LEVEL_CONNECT("error in setsockopt(), FTP server dropping connection; %s",strerror(errno));
                close(fd);
                continue;
            }
    
            addr_len = sizeof(sockaddr_storage_t);
            if (getsockname(fd, (struct sockaddr *)&server_addr, &addr_len) == -1) {
                LEVEL_CONNECT("error in getsockname(), FTP server dropping connection; %s",strerror(errno));
                close(fd);
                continue;
            }
    
            info = (connection_info_t *)malloc(sizeof(connection_info_t));
            if (info == NULL) {
                LEVEL_DEFAULT("out of memory, FTP server dropping connection");
                close(fd);
                continue;
            }
            info->ftp_listener = f;
    
            telnet_session_init(&info->telnet_session, fd, fd);
    
            if (!ftp_session_init(&info->ftp_session,
                                &client_addr,
                                &server_addr,
                                &info->telnet_session,
                                f->dir,
                                &err))
            {
                LEVEL_DEFAULT("error initializing FTP session, FTP server exiting; %s",error_get_desc(&err));
                close(fd);
                telnet_session_destroy(&info->telnet_session);
                free(info);
                continue;
            }


//             error_code = pthread_create(&thread_id, NULL, (void *(*)())connection_handler, info);
            error_code = pthread_create(&thread_id, NULL, connection_handler, info);
    
            if (error_code != 0) {
                LEVEL_DEFAULT("error creating new thread; %d", error_code);
                close(fd);
                telnet_session_destroy(&info->telnet_session);
                free(info);
            }
    
            num_error = 0;
        } else {
            if ((errno == ECONNABORTED) || (errno == ECONNRESET)) {
                LEVEL_CONNECT("interruption accepting FTP connection; %s",strerror(errno));
            } else {
                LEVEL_CONNECT("error accepting FTP connection; %s",strerror(errno));
                ++num_error;
            }
            if (num_error >= MAX_ACCEPT_ERROR) {
                LEVEL_CONNECT("too many consecutive errors, FTP server exiting");
                return NULL;
            }
        }
    }
}

/* convert an address to a printable string */
/* NOT THREADSAFE - wrap with a mutex before calling! */
static char *addr2string(const sockaddr_storage_t *s)
{
#ifdef INET6
    static char addr[IP_ADDRSTRLEN+1];
    int error;
#endif
    char *ret_val;

    daemon_assert(s != NULL);

#ifdef INET6
    error = getnameinfo((struct sockaddr *)s, 
                         sizeof(sockaddr_storage_t),
                         addr, 
                         sizeof(addr), 
                         NULL,
                         0, 
                         NI_NUMERICHOST);
    if (error != 0) {
        LEVEL_CONNECT("getnameinfo error; %s", gai_strerror(error));
        ret_val = "Unknown IP";
    } else {
        ret_val = addr;
    }
#else
    ret_val = inet_ntoa(s->sin_addr);
#endif

    return ret_val;
}


static void *connection_handler(void * v) {
    connection_info_t *info = (connection_info_t *) v ;
    struct ftp_listener_t *f;
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
    LEVEL_CONNECT("%s port %d connection",addr2string(&info->ftp_session.client_addr),ntohs(SINPORT(&info->ftp_session.client_addr)));
    pthread_mutex_unlock(&f->mutex);

    /* handle the session */
    if (num_connections <= f->max_connections) {

        ftp_session_run(&info->ftp_session, &info->watched);

    } else {

        /* too many users */
        UCLIBCLOCK
            sprintf(drop_reason,"Too many users logged in, dropping connection (%d logins maximum)",f->max_connections);
        UCLIBCUNLOCK
        ftp_session_drop(&info->ftp_session, drop_reason);

        /* log the rejection */
        pthread_mutex_lock(&f->mutex);
        LEVEL_CONNECT("%s port %d exceeds max users (%d), dropping connection",addr2string(&info->ftp_session.client_addr),ntohs(SINPORT(&info->ftp_session.client_addr)),num_connections);
        pthread_mutex_unlock(&f->mutex);

    }

    /* exunt (pop calls cleanup function) */
    pthread_cleanup_pop(1);

    /* return for form :) */
    return NULL;
}

/* clean up a connection */
static void connection_handler_cleanup(void * v) {
    connection_info_t *info = (connection_info_t *) v ;
    struct ftp_listener_t *f;

    f = info->ftp_listener;

    watchdog_remove_watched(&info->watched);

    pthread_mutex_lock(&f->mutex);

    f->num_connections--;
    pthread_cond_signal(&f->shutdown_cond);

    LEVEL_CONNECT("%s port %d disconnected",addr2string(&info->ftp_session.client_addr),ntohs(SINPORT(&info->ftp_session.client_addr)));

    pthread_mutex_unlock(&f->mutex);

    ftp_session_destroy(&info->ftp_session);
    telnet_session_destroy(&info->telnet_session);

    free(info);
}

void ftp_listener_stop(struct ftp_listener_t *f)
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

