/*
 * $Id$
 */

#include <owftpd.h> 
 
//#include "owfs_config.h"
//#include <features.h>
//#include <string.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <fcntl.h>
//#include <sys/socket.h>
//#include <errno.h>
//#include <pthread.h>
//#include <sys/utsname.h>
//#include <netdb.h>
//#include <syslog.h>
//#include <stdlib.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>

//#include "ow.h"

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
//#include "ftp_command.h"
//#include "file_list.h"
//#include "ftp_session.h"
//#include "watchdog.h"
//#include "oftpd.h"
//#include "af_portability.h"

/* space requirements */
#define ADDRPORT_STRLEN 58

/* prototypes */
static int invariant(const struct ftp_session_t *f);
static void reply(struct ftp_session_t *f, int code, const char *fmt, ...);
static void change_dir(struct ftp_session_t *f, const char *new_dir);
static int open_connection(struct ftp_session_t *f);
//static int write_fully(int fd, const char *buf, int buflen);
static void init_passive_port(void);
static int get_passive_port(void);
//static int convert_newlines(char *dst, const char *src, int srclen);
static void get_addr_str(const sockaddr_storage_t *s, char *buf, int bufsiz);
//static void send_readme(const struct ftp_session_t *f, int code);
static void netscape_hack(int fd);
static void set_port(struct ftp_session_t *f, const sockaddr_storage_t *host_port);
static int set_pasv(struct ftp_session_t *f, sockaddr_storage_t *host_port);
static int ip_equal(const sockaddr_storage_t *a, const sockaddr_storage_t *b);
static void get_absolute_fname(char *fname, 
                               int fname_len,
                               const char *dir,
                               const char *file);
static int ShowDir( FILE * out, const struct parsedname * const pn );

/* command handlers */
static void do_user(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_pass(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_cwd(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_cdup(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_quit(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_port(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_pasv(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_type(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_stru(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_mode(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_retr(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_stor(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_pwd(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_nlst(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_list(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_syst(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_noop(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_rest(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_lprt(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_lpsv(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_eprt(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_epsv(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_size(struct ftp_session_t *f, const struct ftp_command_t *cmd);
static void do_mdtm(struct ftp_session_t *f, const struct ftp_command_t *cmd);

static struct {
    char *name;
    void (*func)(struct ftp_session_t *f, const struct ftp_command_t *cmd);
} command_func[] = {
    { "USER", do_user },
    { "PASS", do_pass },
    { "CWD",  do_cwd },
    { "CDUP", do_cdup },
    { "QUIT", do_quit },
    { "PORT", do_port },
    { "PASV", do_pasv },
    { "LPRT", do_lprt },
    { "LPSV", do_lpsv },
    { "EPRT", do_eprt },
    { "EPSV", do_epsv },
    { "TYPE", do_type },
    { "STRU", do_stru },
    { "MODE", do_mode },
    { "RETR", do_retr },
    { "STOR", do_stor },
    { "PWD",  do_pwd },
    { "NLST", do_nlst },
    { "LIST", do_list },
    { "SYST", do_syst },
    { "NOOP", do_noop },
    { "REST", do_rest },
    { "SIZE", do_size },
    { "MDTM", do_mdtm }
};

#define NUM_COMMAND_FUNC (sizeof(command_func) / sizeof(command_func[0]))


int ftp_session_init(struct ftp_session_t *f, 
                     const sockaddr_storage_t *client_addr, 
                     const sockaddr_storage_t *server_addr, 
                     struct telnet_session_t *t, 
                     const char *dir,
                   error_code_t *err)
{
    daemon_assert(f != NULL);
    daemon_assert(client_addr != NULL);
    daemon_assert(server_addr != NULL);
    daemon_assert(t != NULL);
    daemon_assert(dir != NULL);
    daemon_assert(strlen(dir) <= PATH_MAX);
    daemon_assert(err != NULL);

#ifdef INET6
    /* if the control connection is on IPv6, we need to get an IPv4 address */
    /* to bind the socket to */
    if (SSFAM(server_addr) == AF_INET6) {
    struct addrinfo hints;
    struct addrinfo *res;
    int errcode;

        /* getaddrinfo() does the job nicely */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, "ftp", &hints, &res) != 0) {
        error_init(err, 0, "unable to determing IPv4 address; %s",
            gai_strerror(errcode));
        return 0;
    }

        /* let's sanity check */
    daemon_assert(res != NULL);
    daemon_assert(sizeof(f->server_ipv4_addr) >= res->ai_addrlen);
        daemon_assert(SSFAM(host_port) == AF_INET);

        /* copy the result and free memory as necessary */
    memcpy(&f->server_ipv4_addr, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
    } else {
        daemon_assert(SSFAM(host_port) == AF_INET);
        f->server_ipv4_addr = *server_addr;
    }
#else
    f->server_ipv4_addr = *server_addr;
#endif

    f->session_active = 1;
    f->command_number = 0;

    f->data_type = TYPE_ASCII;
    f->file_structure = STRU_FILE;

    f->file_offset = 0;
    f->file_offset_command_number = ULONG_MAX;

    f->epsv_all_set = 0;

    f->client_addr = *client_addr;
    get_addr_str(client_addr, f->client_addr_str, sizeof(f->client_addr_str));

    f->server_addr = *server_addr;

    f->telnet_session = t;
    daemon_assert(strlen(dir) < sizeof(f->dir));
    strcpy(f->dir, dir);

    f->data_channel = DATA_PORT;
    f->data_port = *client_addr;
    f->server_fd = -1;

    daemon_assert(invariant(f));

    return 1;
}

void ftp_session_drop(struct ftp_session_t *f, const char *reason)
{
    daemon_assert(invariant(f));
    daemon_assert(reason != NULL);

    /* say goodbye */
    reply(f, 421, "%s.", reason);

    daemon_assert(invariant(f));
}

void ftp_session_run(struct ftp_session_t *f, watched_t *watched)
{
    char buf[2048];
    int len;
    struct ftp_command_t cmd;
    int i;

    daemon_assert(invariant(f));
    daemon_assert(watched != NULL);

    /* record our watchdog */
    f->watched = watched;

    /* say hello */
    //send_readme(f, 220);
    reply(f, 220, "Service ready for new user.");

    /* process commands */
    while (f->session_active && 
        telnet_session_readln(f->telnet_session, buf, sizeof(buf))) 
    {
     
        /* delay our timeout based on this input */
    watchdog_defer_watched(f->watched);

    /* increase our command count */
    if (f->command_number == ULONG_MAX) {
        f->command_number = 0;
    } else {
        f->command_number++;
    }
    
        /* make sure we read a whole line */
        len = strlen(buf);
    if (buf[len-1] != '\n') {
            reply(f, 500, "Command line too long.");
            while (telnet_session_readln(f->telnet_session, buf, sizeof(buf))) {
                len = strlen(buf);
        if (buf[len-1] == '\n') {
            break;
        }
            }
        goto next_command;
    }

        LEVEL_DEBUG( "%s %s", f->client_addr_str, buf);

    /* parse the line */
    if (!ftp_command_parse(buf, &cmd)) {
        reply(f, 500, "Syntax error, command unrecognized.");
        goto next_command;
    }

    /* dispatch the command */
    for (i=0; i<NUM_COMMAND_FUNC; i++) {
        if (strcmp(cmd.command, command_func[i].name) == 0) {
            (command_func[i].func)(f, &cmd);
            goto next_command;
        }
    }

        /* oops, we don't have this command (shouldn't happen - shrug) */
    reply(f, 502, "Command not implemented.");

next_command: {}
    }

    daemon_assert(invariant(f));
}

void ftp_session_destroy(struct ftp_session_t *f) 
{
    daemon_assert(invariant(f));

    if (f->server_fd != -1) {
        close(f->server_fd);
    f->server_fd = -1;
    }
}

#ifndef NDEBUG
static int invariant(const struct ftp_session_t *f)
{
    int len;

    if (f == NULL) {
        return 0;
    }
    if ((f->session_active != 0) && (f->session_active != 1)) {
        return 0;
    }
    if ((f->data_type != TYPE_ASCII) && (f->data_type != TYPE_IMAGE)) {
        return 0;
    }
    if ((f->file_structure != STRU_FILE) && (f->file_structure != STRU_RECORD)){
        return 0;
    }
    if (f->file_offset < 0) {
        return 0;
    }
    if ((f->epsv_all_set != 0) && (f->epsv_all_set != 1)) {
        return 0;
    }
    len = strlen(f->client_addr_str);
    if ((len <= 0) || (len >= ADDRPORT_STRLEN)) {
        return 0;
    }
    if (f->telnet_session == NULL) {
        return 0;
    }
    switch (f->data_channel) {
    case DATA_PORT:
            /* If the client specifies a port, verify that it is from the   */
        /* host the client connected from.  This prevents a client from */
        /* using the server to open connections to arbritrary hosts.    */
        if (!ip_equal(&f->client_addr, &f->data_port)) {
            return 0;
        }
        if (f->server_fd != -1) {
            return 0;
        }
        break;
    case DATA_PASSIVE:
        if (f->server_fd < 0) {
            return 0;
        }
        break;
        default:
        return 0;
    }
    return 1;
}
#endif /* NDEBUG */

static void reply(struct ftp_session_t *f, int code, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    daemon_assert(invariant(f));
    daemon_assert(code >= 100);
    daemon_assert(code <= 559);
    daemon_assert(fmt != NULL);

    /* prepend our code to the buffer */
    sprintf(buf, "%d", code);
    buf[3] = ' ';

    /* add the formatted output of the caller to the buffer */
    va_start(ap, fmt);
    vsnprintf(buf+4, sizeof(buf)-4, fmt, ap);
    va_end(ap);

    /* log our reply */
    syslog(LOG_DEBUG, "%s %s", f->client_addr_str, buf);

    /* send the output to the other side */
    telnet_session_println(f->telnet_session, buf);

    daemon_assert(invariant(f));
}

static void do_user(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *user;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    user = cmd->arg[0].string;
    LEVEL_CONNECT("%s attempted to log in as \"%s\"",f->client_addr_str, user);
    if (strcasecmp(user, "ftp") && strcasecmp(user, "anonymous")) {
        reply(f, 530, "Only anonymous FTP supported.");
    } else {
        reply(f, 331, "Send e-mail address as password.");
    }
    daemon_assert(invariant(f));
}


static void do_pass(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *password;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    password = cmd->arg[0].string;
    LEVEL_CONNECT("%s reports e-mail address \"%s\"",f->client_addr_str, password);
    reply(f, 230, "User logged in, proceed.");

    daemon_assert(invariant(f));
}

#ifdef INET6
static void get_addr_str(const sockaddr_storage_t *s, char *buf, int bufsiz)
{
    int port;
    int error;
    int len;

    daemon_assert(s != NULL);
    daemon_assert(buf != NULL);

    /* buf must be able to contain (at least) a string representing an
     * ipv4 addr, followed by the string " port " (6 chars) and the port
     * number (which is 5 chars max), plus the '\0' character. */ 
    daemon_assert(bufsiz >= (INET_ADDRSTRLEN + 12));

    error = getnameinfo(client_addr, sizeof(sockaddr_storage_t), buf, 
                bufsiz, NULL, 0, NI_NUMERICHOST);
    /* getnameinfo() should never fail when called with NI_NUMERICHOST */
    daemon_assert(error == 0);

    len = strlen(buf);
    daemon_assert(bufsiz >= len+12);
    snprintf(buf+len, bufsiz-len, " port %d", ntohs(SINPORT(&f->client_addr)));
}
#else
static void get_addr_str(const sockaddr_storage_t *s, char *buf, int bufsiz)
{
    unsigned int addr;
    int port;

    daemon_assert(s != NULL);
    daemon_assert(buf != NULL);

    /* buf must be able to contain (at least) a string representing an
     * ipv4 addr, followed by the string " port " (6 chars) and the port
     * number (which is 5 chars max), plus the '\0' character. */ 
    daemon_assert(bufsiz >= (INET_ADDRSTRLEN + 12));

    addr = ntohl(s->sin_addr.s_addr);
    port = ntohs(s->sin_port);
    snprintf(buf, (unsigned int)bufsiz, "%d.%d.%d.%d port %d", 
        (addr >> 24) & 0xff, 
    (addr >> 16) & 0xff,
    (addr >> 8)  & 0xff,
    addr & 0xff,
        port);
}
#endif

static void do_cwd(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *new_dir;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    new_dir = cmd->arg[0].string;
    change_dir(f, new_dir);

    daemon_assert(invariant(f));
}

static void do_cdup(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    change_dir(f, "..");

    daemon_assert(invariant(f));
}

static void change_dir(struct ftp_session_t *f, const char *new_dir)
{
    char target[PATH_MAX+1];
    const char *p, *n;
    int len;
    char *prev_dir;
    char *target_end;

    struct stat stat_buf;
    int dir_okay;

    daemon_assert(invariant(f));
    daemon_assert(new_dir != NULL);
    daemon_assert(strlen(new_dir) <= PATH_MAX);

    /* set up our "base" directory that we build from */
    p = new_dir;
    if (*p == '/') {
        /* if this starts with a '/' it is an absolute path */
        strcpy(target, "/");
    do {
        p++;
    } while (*p == '/');
    } else {
        /* otherwise it's a relative path */
    daemon_assert(strlen(f->dir) < sizeof(target));
    strcpy(target, f->dir);
    }

    /* add on each directory, handling "." and ".." */
    while (*p != '\0') {

        /* find the end of the next directory (either at '/' or '\0') */
        n = strchr(p, '/');
    if (n == NULL) {
        n = strchr(p, '\0');
    }
    len = n - p;

        if ((len == 1) && (p[0] == '.')) {

        /* do nothing with "." */

    } else if ((len == 2) && (p[0] == '.') && (p[1] == '.')) {

        /* change to previous directory with ".." */
            prev_dir = strrchr(target, '/');
        daemon_assert(prev_dir != NULL);
        *prev_dir = '\0';
        if (prev_dir == target) {
                strcpy(target, "/");
        }

    } else {

        /* otherwise add to current directory */
        if ((strlen(target) + 1 + len) > PATH_MAX) {
            reply(f, 550, "Error changing directory; path is too long.");
        return;
        }

        /* append a '/' unless we were at the root directory */
        target_end = strchr(target, '\0');
        if (target_end != target+1) {
            *target_end++ = '/';
        }

        /* add the directory itself */
        while (p != n) {
            *target_end++ = *p++;
        }
        *target_end = '\0';

    }

    /* advance to next directory to check */
    p = n;

        /* skip '/' characters */
        while (*p == '/') {
        p++;
    }
    }

    /* see if this is a directory we can change into */
    dir_okay = 0;
    if (FS_fstat(target, &stat_buf) == 0) {
#ifndef STAT_MACROS_BROKEN
        if (!S_ISDIR(stat_buf.st_mode)) {
#else
        if (S_ISDIR(stat_buf.st_mode)) {
#endif
        reply(f, 550,"Directory change failed; target is not a directory.");
    } else { 
        if (S_IXOTH & stat_buf.st_mode) {
            dir_okay = 1;
        } else if ((stat_buf.st_gid == getegid()) && 
            (S_IXGRP & stat_buf.st_mode)) 
        {
            dir_okay = 1;
        } else if ((stat_buf.st_uid == geteuid()) && 
            (S_IXUSR & stat_buf.st_mode)) 
        {
            dir_okay = 1;
        } else {
            reply(f, 550, "Directory change failed; permission denied.");
        }
    }
    } else {
        reply(f, 550, "Directory change failed; directory does not exist.");
    }

    /* if everything is okay, change into the directory */
    if (dir_okay) {
    daemon_assert(strlen(target) < sizeof(f->dir));
    /* send a readme unless we changed to our current directory */
    if (strcmp(f->dir, target) != 0) {
        strcpy(f->dir, target);
	//send_readme(f, 250);
        } else {
        strcpy(f->dir, target);
    }
        reply(f, 250, "Directory change successful.");
    }

    daemon_assert(invariant(f));
}

static void do_quit(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    reply(f, 221, "Service closing control connection.");
    f->session_active = 0;

    daemon_assert(invariant(f));
}

/* support for the various port setting functions */
static void set_port(struct ftp_session_t *f, const sockaddr_storage_t *host_port) {
    daemon_assert(invariant(f));
    daemon_assert(host_port != NULL);

    if (f->epsv_all_set) {
        reply(f, 500, "After EPSV ALL, only EPSV allowed.");
    } else if (!ip_equal(&f->client_addr, host_port)) {
        reply(f, 500, "Port must be on command channel IP.");
    } else if (ntohs(SINPORT(host_port)) < IPPORT_RESERVED) {
        reply(f, 500, "Port may not be less than 1024, which is reserved.");
    } else {
        /* close any outstanding PASSIVE port */
        if (f->data_channel == DATA_PASSIVE) {
            close(f->server_fd);
        f->server_fd = -1;
        }

        f->data_channel = DATA_PORT;
    f->data_port = *host_port;
    reply(f, 200, "Command okay.");
    }

    daemon_assert(invariant(f));
}

/* set IP and port for client to receive data on */
static void do_port(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const sockaddr_storage_t *host_port;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    host_port = &cmd->arg[0].host_port;
    daemon_assert(SSFAM(host_port) == AF_INET);

    set_port(f, host_port);

    daemon_assert(invariant(f));
}

/* set IP and port for client to receive data on, transport independent */
static void do_lprt(struct ftp_session_t *f, const struct ftp_command_t *cmd)  
{
    const sockaddr_storage_t *host_port;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);                                   

    host_port = &cmd->arg[0].host_port;

#ifdef INET6
    if ((SSFAM(host_port) != AF_INET) && (SSFAM(host_port) != AF_INET6)) {
        reply(f, 521, "Only IPv4 and IPv6 supported, address families (4,6)");
    }
#else
    if (SSFAM(host_port) != AF_INET) {
        reply(f, 521, "Only IPv4 supported, address family (4)");
    }
#endif

    set_port(f, host_port);

    daemon_assert(invariant(f));
}

/* set IP and port for the client to receive data on, IPv6 extension */
/*                                                                   */
/* RFC 2428 specifies that if the data connection is going to be on  */
/* the same IP as the control connection, EPSV must be used.  Since  */
/* that is the only mode of transfer we support, we reject all EPRT  */
/* requests.                                                         */
static void do_eprt(struct ftp_session_t *f, const struct ftp_command_t *cmd)  
{
    //const sockaddr_storage_t *host_port;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);                                   

    reply(f, 500, "EPRT not supported, use EPSV.");

    daemon_assert(invariant(f));
}

/* support for the various pasv setting functions */
/* returns the file descriptor of the bound port, or -1 on error */
/* note: the "host_port" parameter will be modified, having its port set */
static int set_pasv(struct ftp_session_t *f, sockaddr_storage_t *bind_addr)
{
    int socket_fd;
    int port;

    daemon_assert(invariant(f));
    daemon_assert(bind_addr != NULL);

    socket_fd = socket(SSFAM(bind_addr), SOCK_STREAM, 0);
    if (socket_fd == -1) {
        reply(f, 500, "Error creating server socket; %s.", strerror(errno));
    return -1;
    } 

    for (;;) {
        port = get_passive_port();
        SINPORT(bind_addr) = htons(port);
    if (bind(socket_fd, (struct sockaddr *)bind_addr, 
        sizeof(struct sockaddr)) == 0) 
    {
        break;
    }
    if (errno != EADDRINUSE) {
            reply(f, 500, "Error binding server port; %s.", strerror(errno));
            close(socket_fd);
            return -1;
    }
    }

    if (listen(socket_fd, 1) != 0) {
        reply(f, 500, "Error listening on server port; %s.", strerror(errno));
        close(socket_fd);
    return -1;
    }

    return socket_fd;
}

/* pick a server port to listen for connection on */
static void do_pasv(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    int socket_fd;
    unsigned int addr;
    int port;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    if (f->epsv_all_set) {
        reply(f, 500, "After EPSV ALL, only EPSV allowed.");
        goto exit_pasv;
    }

    socket_fd = set_pasv(f, &f->server_ipv4_addr);
    if (socket_fd == -1) {
        goto exit_pasv;
    }

    /* report port to client */
    addr = ntohl(f->server_ipv4_addr.sin_addr.s_addr);
    port = ntohs(f->server_ipv4_addr.sin_port);
    reply(f, 227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
        addr >> 24, 
    (addr >> 16) & 0xff,
    (addr >> 8)  & 0xff,
    addr & 0xff,
        port >> 8, 
    port & 0xff);

   /* close any outstanding PASSIVE port */
   if (f->data_channel == DATA_PASSIVE) {
       close(f->server_fd);
   }
   f->data_channel = DATA_PASSIVE;
   f->server_fd = socket_fd;

exit_pasv:
    daemon_assert(invariant(f));
}

/* pick a server port to listen for connection on, including IPv6 */
static void do_lpsv(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    int socket_fd;
    char addr[80];
    uint8_t *a;
    uint8_t *p;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    if (f->epsv_all_set) {
        reply(f, 500, "After EPSV ALL, only EPSV allowed.");
        goto exit_lpsv;
    }

    socket_fd = set_pasv(f, &f->server_addr);
    if (socket_fd == -1) {
        goto exit_lpsv;
    }

    /* report address and port to client */
#ifdef INET6
    if (SSFAM(&f->server_addr) == AF_INET6) {
        a = (uint8_t *)&SIN6ADDR(&f->server_addr);
        p = (uint8_t *)&SIN6PORT(&f->server_addr);
    snprintf(addr, sizeof(addr),
        "(6,16,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,2,%d,%d)",
        a[0],  a[1],  a[2],  a[3],  a[4],  a[5],  a[6],  a[7],  a[8],
        a[9], a[10], a[11], a[12], a[13], a[14], a[15],  p[0],  p[1]);
    } else 
#endif
    {
        a = (uint8_t *)&SIN4ADDR(&f->server_addr);
        p = (uint8_t *)&SIN4PORT(&f->server_addr);
    snprintf(addr, sizeof(addr), "(4,4,%d,%d,%d,%d,2,%d,%d)",
        a[0], a[1], a[2], a[3], p[0], p[1]);    
    }

    reply(f, 228, "Entering Long Passive Mode %s", addr);

   /* close any outstanding PASSIVE port */
   if (f->data_channel == DATA_PASSIVE) {
       close(f->server_fd);
   }
   f->data_channel = DATA_PASSIVE;
   f->server_fd = socket_fd;

exit_lpsv:
    daemon_assert(invariant(f));
}

/* pick a server port to listen for connection on, new IPv6 method */
static void do_epsv(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    int socket_fd;
    sockaddr_storage_t *addr;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert((cmd->num_arg == 0) || (cmd->num_arg == 1));

    /* check our argument, if any,  and use the appropriate address */
    if (cmd->num_arg == 0) {
        addr = &f->server_addr;
    } else {
        switch (cmd->arg[0].num) {
        /* EPSV_ALL is a special number indicating the client sent */
        /* the command "EPSV ALL" - this is not a request to assign */
        /* a new passive port, but rather to deny all future port */
        /* assignment requests other than EPSV */
        case EPSV_ALL:
                f->epsv_all_set = 1;
        reply(f, 200, "EPSV ALL command successful.");
        goto exit_epsv;
        case 1:
            addr = (sockaddr_storage_t *)&f->server_ipv4_addr;
        break;
#ifdef INET6
        case 2:
            addr = &f->server_addr;
        break;
        default:
            reply(f, 522, "Only IPv4 and IPv6 supported, use (1,2)");
            goto exit_epsv;
#else
        default:
            reply(f, 522, "Only IPv4 supported, use (1)");
            goto exit_epsv;
#endif
        }
    }

    /* bind port and so on */
    socket_fd = set_pasv(f, addr);
    if (socket_fd == -1) {
        goto exit_epsv;
    }

    /* report port to client */
    reply(f, 229, "Entering Extended Passive Mode (|||%d|)", 
        ntohs(SINPORT(&f->server_addr)));

    /* close any outstanding PASSIVE port */
    if (f->data_channel == DATA_PASSIVE) {
        close(f->server_fd);
    }
    f->data_channel = DATA_PASSIVE;
    f->server_fd = socket_fd;  

exit_epsv:
    daemon_assert(invariant(f));
}

/* seed the random number generator used to pick a port */
static void init_passive_port( void )
{
    struct timeval tv;
#ifdef __UCLIBC__
    unsigned int seed ;
    seed = tv.tv_usec & 0xFFFF ;
    srand(seed) ;
#else /* __UCLIBC__ */
    unsigned short int seed[3];

    gettimeofday(&tv, NULL);
    seed[0] = (tv.tv_sec >> 16) & 0xFFFF;
    seed[1] = tv.tv_sec & 0xFFFF;
    seed[2] = tv.tv_usec & 0xFFFF;
    seed48(seed);
#endif /* __UCLIBC__ */
}

pthread_mutex_t passive_lock = PTHREAD_MUTEX_INITIALIZER;

/* pick a port to try to bind() for passive FTP connections */
static int get_passive_port( void )
{
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    int port;

    /* initialize the random number generator the first time we're called */
    pthread_once(&once_control, init_passive_port);

    /* pick a random port between 1024 and 65535, inclusive */
    pthread_mutex_lock(&passive_lock);
#ifdef __UCLIBC__
    port = (rand() % 64512 ) + 1024 ;
#else /* __UCLIBC__ */
    port = (lrand48() % 64512) + 1024;
#endif /* __UCLIBC__ */
    pthread_mutex_unlock(&passive_lock);

    return port;
}

static void do_type(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    char type;
    char form;
    int cmd_okay;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg >= 1);
    daemon_assert(cmd->num_arg <= 2);

    type = cmd->arg[0].string[0];
    if (cmd->num_arg == 2) {
        form = cmd->arg[1].string[0];
    } else {
        form = 0;
    }

    cmd_okay = 0;
    if (type == 'A') {
        if ((cmd->num_arg == 1) || ((cmd->num_arg == 2) && (form == 'N'))) {
            f->data_type = TYPE_ASCII;
        cmd_okay = 1;
    }
    } else if (type == 'I') {
        f->data_type = TYPE_IMAGE;
    cmd_okay = 1;
    }

    if (cmd_okay) {
        reply(f, 200, "Command okay.");
    } else { 
        reply(f, 504, "Command not implemented for that parameter.");
    }

    daemon_assert(invariant(f));
}

static void do_stru(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    char structure;
    int cmd_okay;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    structure = cmd->arg[0].string[0];
    cmd_okay = 0;
    if (structure == 'F') {
        f->file_structure = STRU_FILE;
        cmd_okay = 1;
    } else if (structure == 'R') {
        f->file_structure = STRU_RECORD;
        cmd_okay = 1;
    }

    if (cmd_okay) {
        reply(f, 200, "Command okay.");
    } else {
        reply(f, 504, "Command not implemented for that parameter.");
    }

    daemon_assert(invariant(f));
}

static void do_mode(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    char mode;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    mode = cmd->arg[0].string[0];
    if (mode == 'S') {
        reply(f, 200, "Command okay.");
    } else {
        reply(f, 504, "Command not implemented for that parameter.");
    }

    daemon_assert(invariant(f));
}

/* convert the user-entered file name into a full path on our local drive */
static void get_absolute_fname(char *fname, 
                               int fname_len,
                               const char *dir,
                               const char *file)
{
    daemon_assert(fname != NULL);
    daemon_assert(dir != NULL);
    daemon_assert(file != NULL);

    if (*file == '/') {

        /* absolute path, use as input */
        daemon_assert(strlen(file) < fname_len);
    strcpy(fname, file);

    } else {

        /* construct a file name based on our current directory */
        daemon_assert(strlen(dir) + 1 + strlen(file) < fname_len);
        strcpy(fname, dir);

    /* add a seperating '/' if we're not at the root */
    if (fname[1] != '\0') {
            strcat(fname, "/");
    }

        /* and of course the actual file name */
        strcat(fname, file);

    }
}

/* Device entry -- table line for a filetype  -- text mode*/
static int ShowText( FILE * out, const char * const basename, const char * const fullpath, const struct parsedname * const pn, unsigned int suglen, char *buf ) {
    int len ;
    int ret = -1 ;
    enum ft_format format ;
    int canwrite = 0 ;
    //printf("ShowText: basename=%s, fullpath=%s\n",basename,fullpath) ;
    (void)basename ;

    if(!pn->ft) {
      format = ft_subdir ;      /* it seems to be a subdir */
    } else {
      format = pn->ft->format ;
      canwrite = !readonly && pn->ft->write.v ;
    }

    /* Special processing for structure -- ascii text, not native format */
    if ( (pn->type == pn_structure) || ( format==ft_directory || format==ft_subdir ) ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

    //fprintf( out, "%s ", basename ) ;

    /* buffer for field value */
    if ( pn->ft && pn->ft->ag && format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath,buf,suglen,(off_t)0))>0 ) {
                buf[len] = '\0' ;
                ret = fprintf( out, "%s",buf ) ;
            }
        } else if ( canwrite ) { /* rare write-only */
	    ret = fprintf( out, "(writeonly)" ) ;
        }
    } else {
        switch( format ) {
        case ft_directory:
        case ft_subdir:
            break ;
        case ft_yesno:
            if ( pn->ft->read.v ) { /* at least readable */
                if ( (len=FS_read(fullpath,buf,suglen,(off_t)0))>0 ) {
		    ret = fprintf( out, "%c", buf[0] ) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                ret = fprintf( out, "(writeonly)" ) ;
            }
            break ;
        case ft_binary:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,(off_t)0))>0 ) {
                    int i ;
		    ret = 0;
                    for( i=0 ; i<len ; ++i ) {
                        ret += fprintf( out, "%.2hhX", buf[i] ) ;
                    }
                }
            } else if ( canwrite ) { /* rare write-only */
                ret = fprintf( out, "(writeonly)" ) ;
            }
            break ;
        default:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,(off_t)0))>0 ) {
                    buf[len] = '\0' ;
                    ret = fprintf( out, "%s",buf ) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                ret = fprintf( out, "(writeonly)") ;
            }
        }
    }
    //fprintf( out, "\r\n" ) ;
    return ret ;
}

/* Device entry -- table line for a filetype */
static int Show( FILE * out, const char * const path, const char * const file, const struct parsedname * const pn ) {
    struct parsedname pn2 ;
    struct stateinfo si ;
    const char * basename ;
    char fullpath[PATH_MAX+1] ;
    unsigned int suglen = 0 ;
    char *buf = NULL ;
    enum ft_format format ;
    int canwrite = 0 ;
    int ret = -1;

    //printf("Show: path=%s, file=%s\n",path,file) ;

    if(!pn->ft) {
      format = ft_subdir ;      /* it seems to be a subdir */
    } else {
      format = pn->ft->format ;
      canwrite = !readonly && pn->ft->write.v ;
    }
    //printf("Show path=%s, file=%s, suglen=%d pn_struct?%d, ft_directory?%d, ft_subdir?%d\n",path,file,suglen,pn->type == pn_structure,format==ft_directory,format==ft_subdir);

    /* Parse out subdir */
    basename = strrchr(file,'/') ;
    if ( basename ) {
        ++basename ; /* after slash */
    } else {
        basename = file ;
    }
    strcpy(fullpath, path) ;
    if ( fullpath[strlen(fullpath)-1] != '/' ) strcat( fullpath, "/" ) ;
    strcat(fullpath,basename ) ;

    pn2.si = &si;
    if ( (FS_ParsedName(fullpath, &pn2) == 0) ) {
      if ((pn2.state & pn_bus) && (get_busmode(pn2.in) == bus_remote)) {
	//printf("call FS_size(%s)\n", fullpath);
	suglen = FS_size(fullpath) ;
      } else {
	//printf("call FS_size_postparse\n");
	suglen = FS_size_postparse(pn) ;
      }
    } else {
      suglen = 0 ;
      //printf("FAILED parsename %s\n", fullpath);
    }
    FS_ParsedName_destroy( &pn2 ) ;

    if(suglen <= 0) {
      //printf("Show: can't find file-size of %s ???\n", pn->path);
      suglen = 0 ;
    }
    if( ! (buf = malloc((size_t)suglen+1)) ) {
      return -1 ;
    }
    buf[suglen] = '\0' ;

    //printf("Show path=%s, file=%s, suglen=%d pn_struct?%d, ft_directory?%d, ft_subdir?%d\n",path,file,suglen,pn->type == pn_structure,format==ft_directory,format==ft_subdir);

    /* Special processing for structure -- ascii text, not native format */
    if ( pn->type==pn_structure && format!=ft_directory && format!=ft_subdir ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

//printf("pn->path=%s, pn->path_busless=%s\n",pn->path, pn->path_busless) ;
//printf("path=%s, file=%s, fullpath=%s\n",path,file, fullpath) ;

    ret = ShowText( out, basename, fullpath, pn, suglen, buf ) ;
    free(buf);
    return ret ;
}


/* Now show the device */
static int ShowDevice( FILE * out, const struct parsedname * const pn ) {
    struct parsedname pncopy ;
    char * slash;
    char * path2;
    int ret = 0 ;
    /* Embedded function */
    void directory( const struct parsedname * const pn2 ) {
        char *file;
	if( !(file = malloc(OW_FULLNAME_MAX+1)) ) { /* buffer for name */
	  //printf("ShowDevice error malloc %d bytes\n",OW_FULLNAME_MAX+1) ;
	  return;
	}
	FS_DirName(file,OW_FULLNAME_MAX,pn2);
	//printf("ShowDevice: emb: pn2->ft=%p pn2->subdir=%p pn2->dev=%p path2=%s file=%s\n", pn2->ft, pn2->subdir, pn2->dev, path2, file);
	Show( out, path2, file, pn2 ) ;
	free(file);
    }

    //printf("ShowDevice = %s  bus_nr=%d pn->dev=%p\n",pn->path, pn->bus_nr, pn->dev) ;
    if(! (path2 = strdup(pn->path)) ) return -1 ;
    memcpy(&pncopy, pn, sizeof(struct parsedname));

    if ( pn->ft ) { /* single item */
        char *file;
	//printf("single item path=%s pn->path=%s\n", path2, pn->path);
        slash = strrchr(path2,'/') ;
        /* Nested function */
        if ( slash ) slash[0] = '\0' ; /* pare off device name */
	//printf("single item path=%s\n", path2);
	if( !(file = malloc(OW_FULLNAME_MAX+1)) ) { /* buffer for name */
	  //printf("ShowDevice error malloc %d bytes\n",OW_FULLNAME_MAX+1) ;
	  return -1 ;
	}
	FS_DirName(file,OW_FULLNAME_MAX,&pncopy);
	//printf("ShowDevice: emb: pncopy.ft=%p pncopy.subdir=%p pncopy.dev=%p path2=%s file=%s\n", pncopy.ft, pncopy.subdir, pncopy.dev, path2, file);
	ret = Show( out, path2, file, &pncopy ) ;
	free(file);
    } else { /* whole device */
      //printf("whole directory path=%s pn->path=%s\n", path2, pn->path);
        //printf("pn->dev=%p pn->ft=%p pn->subdir=%p\n", pn->dev, pn->ft, pn->subdir);
      ret = FS_dir( directory, &pncopy ) ;
    }
    free(path2) ;
    return ret ;
}


static void do_retr(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *file_name;
    char full_path[PATH_MAX+1+MAX_STRING_LEN];
    int file_fd;
#if 0
    struct stat stat_buf;
    int read_ret;
    char buf[4096];
    char converted_buf[8192];
    int converted_buflen;
    off_t offset;
    off_t amt_to_send;
    int sendfile_ret;
    off_t amt_sent;
#endif
    off_t file_size = 0;
    int socket_fd;
    struct timeval transfer_time;
    struct timeval start_timestamp;
    struct timeval end_timestamp;
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    /* set up for exit */
    file_fd = -1;
    socket_fd = -1;

    /* create an absolute name for our file */
    file_name = cmd->arg[0].string;
    get_absolute_fname(full_path, sizeof(full_path), f->dir, file_name);

#if 1
    pn.si = &si ;
    ret = FS_ParsedName( full_path, &pn ) ;
    // first root always return Bus-list and settings/system/statistics
    pn.si->sg |= (1<<BUSRET_BIT) ;

    if (ret) {
        reply(f, 550, "Error opening file; %s.", strerror(errno));
        goto exit_retr;
    } else if ( (pn.dev == NULL) || (pn.ft == NULL) ) { /* directory! */
        reply(f, 550, "Error, file is a directory.");
        goto exit_retr;
    } else {
        FILE *out ;

        if(f->file_offset > 0) {
            /* Don't support offset right now.. */
            reply(f, 550, "Error seeking to restart position") ;
            goto exit_retr;
        }

        /* ready to transfer */
        reply(f, 150, "About to open data connection.");
        
        /* mark start time */
        gettimeofday(&start_timestamp, NULL);

        /* open data path */
        socket_fd = open_connection(f);
        if (socket_fd == -1) goto exit_retr;
        out = fdopen(socket_fd, "w") ;
        if (out == NULL) goto exit_retr;
        ret = ShowDevice( out, &pn ) ;
        watchdog_defer_watched(f->watched);
        //printf("showdevice returned ret=%d\n", ret);
        fclose(out);
        if (ret < 0) {
            reply(f, 550, "Error opening file; %s.", strerror(errno));
            goto exit_retr;
        }
        file_size += ret;
    }
#else
    /* open file */
    file_fd = open(full_path, O_RDONLY);
    if (file_fd == -1) {
        reply(f, 550, "Error opening file; %s.", strerror(errno));
        goto exit_retr;
    }
    if (fstat(file_fd, &stat_buf) != 0) {
        reply(f, 550, "Error getting file information; %s.", strerror(errno));
        goto exit_retr;
    }
#ifndef STATS_MACRO_BROKEN
    if (S_ISDIR(stat_buf.st_mode)) {
#else
    if (!S_ISDIR(stat_buf.st_mode)) {
#endif
        reply(f, 550, "Error, file is a directory.");
        goto exit_retr;
    }

    /* if the last command was a REST command, restart at the */
    /* requested position in the file                         */
    if ((f->file_offset_command_number == (f->command_number - 1)) && (f->file_offset > 0)) {
        if (lseek(file_fd, f->file_offset, SEEK_SET) == -1) {
            reply(f, 550, "Error seeking to restart position; %s.", 
                strerror(errno));
                goto exit_retr;
        }
    }

    /* ready to transfer */
    reply(f, 150, "About to open data connection.");

    /* mark start time */
    gettimeofday(&start_timestamp, NULL);

    /* open data path */
    socket_fd = open_connection(f);
    if (socket_fd == -1) {
    goto exit_retr;
    }

    /* we're golden, send the file */
    file_size = 0;
    if (f->data_type == TYPE_ASCII) {
        for (;;) {
            read_ret = read(file_fd, buf, sizeof(buf));
            if (read_ret == -1) {
                reply(f, 550, "Error reading from file; %s.", strerror(errno));
                goto exit_retr;
            }
            if (read_ret == 0) break;
            converted_buflen = convert_newlines(converted_buf, buf, read_ret);

            if (write_fully(socket_fd, converted_buf, converted_buflen) == -1) {
                reply(f, 550, "Error writing to data connection; %s.", 
              strerror(errno));
                goto exit_retr;
            }

            file_size += converted_buflen;
        } 
    } else {
        daemon_assert(f->data_type == TYPE_IMAGE);
        
        /* for sendfile(), we still have to use a loop to avoid 
           having our watchdog time us out on large files - it does
           allow us to avoid an extra copy to/from user space */
#ifdef HAVE_SENDFILE
        offset = f->file_offset;
        file_size = stat_buf.st_size - offset;
        while (offset < stat_buf.st_size) {

            amt_to_send = stat_buf.st_size - offset;
            if (amt_to_send > 65536) amt_to_send = 65536;
#ifdef HAVE_LINUX_SENDFILE
            sendfile_ret = sendfile(socket_fd, 
                                    file_fd, 
                                    &offset, 
                                    amt_to_send);
            if (sendfile_ret != amt_to_send) {
                reply(f, 550, "Error sending file; %s.", strerror(errno));
                goto exit_retr;
            }
#elif HAVE_FREEBSD_SENDFILE
            sendfile_ret = sendfile(file_fd, 
                                    socket_fd, 
                                    offset,
                                    amt_to_send,
                                    NULL,
                                    &amt_sent,
                                    0);
            if (sendfile_ret != 0) {
                reply(f, 550, "Error sending file; %s.", strerror(errno));
                goto exit_retr;
            }
            offset += amt_sent;
#endif

            watchdog_defer_watched(f->watched);
        }
#else
        for (;;) {
            read_ret = read(file_fd, buf, sizeof(buf));
            if (read_ret == -1) {
                reply(f, 550, "Error reading from file; %s.", strerror(errno));
                goto exit_retr;
            }
            if (read_ret == 0) {
                break;
            }
            if (write_fully(socket_fd, buf, read_ret) == -1) {
                reply(f, 550, "Error writing to data connection; %s.", 
            strerror(errno));
                goto exit_retr;
            }
            file_size += read_ret;

            watchdog_defer_watched(f->watched);
        }
#endif  /* HAVE_SENDFILE */
    }
#endif

    /* disconnect */
    close(socket_fd);
    socket_fd = -1;

    /* hey, it worked, let the other side know */
    reply(f, 226, "File transfer complete.");

    /* mark end time */
    gettimeofday(&end_timestamp, NULL);

    /* calculate transfer rate */
    transfer_time.tv_sec = end_timestamp.tv_sec - start_timestamp.tv_sec;
    transfer_time.tv_usec = end_timestamp.tv_usec - start_timestamp.tv_usec;
    while (transfer_time.tv_usec >= 1000000) {
        transfer_time.tv_sec++;
        transfer_time.tv_usec -= 1000000;
    }
    while (transfer_time.tv_usec < 0) {
        transfer_time.tv_sec--;
        transfer_time.tv_usec += 1000000;
    }

    /* note the transfer */
    LEVEL_CONNECT(
      "%s retrieved \"%s\", %ld bytes in %d.%06d seconds", 
      f->client_addr_str, 
      full_path,
      file_size,
      transfer_time.tv_sec,
      transfer_time.tv_usec);

exit_retr:
    FS_ParsedName_destroy( &pn ) ;
    f->file_offset = 0;
    if (socket_fd != -1) close(socket_fd);
    if (file_fd != -1) close(file_fd);
    daemon_assert(invariant(f));
}

static void do_stor(struct ftp_session_t *f, const struct ftp_command_t *cmd)  {
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    reply(f, 553, "Server will not store files.");

    daemon_assert(invariant(f));
}

static int open_connection(struct ftp_session_t *f) {
    int socket_fd;
    struct sockaddr_in addr;
    unsigned addr_len;

    daemon_assert((f->data_channel == DATA_PORT) || 
                  (f->data_channel == DATA_PASSIVE));

    if (f->data_channel == DATA_PORT) {
        socket_fd = socket(SSFAM(&f->data_port), SOCK_STREAM, 0);
        if (socket_fd == -1) {
            reply(f, 425, "Error creating socket; %s.", strerror(errno));
            return -1;
        }
        if (connect(socket_fd, (struct sockaddr *)&f->data_port, 
            sizeof(sockaddr_storage_t)) != 0)
        {
            reply(f, 425, "Error connecting; %s.", strerror(errno));
            close(socket_fd);
            return -1;
        }
    } else {
        daemon_assert(f->data_channel == DATA_PASSIVE);

        addr_len = sizeof(struct sockaddr_in);
        socket_fd = accept(f->server_fd, (struct sockaddr *)&addr, &addr_len);
        if (socket_fd == -1) {
            reply(f, 425, "Error accepting connection; %s.", strerror(errno));
            return -1;
        }
#ifdef INET6
        /* in IPv6, the client can connect to a channel using a different */
        /* protocol - in that case, we'll just blindly let the connection */
        /* through, otherwise verify addresses match */
        if (SAFAM(addr) == SSFAM(&f->client_addr)) {
        if (memcmp(&SINADDR(&f->client_addr), &SINADDR(&addr), 
                   sizeof(SINADDR(&addr))))
        {
            reply(f, 425, 
              "Error accepting connection; connection from invalid IP.");
            close(socket_fd);
            return -1;
        }
    }
#else
    if (memcmp(&f->client_addr.sin_addr, 
        &addr.sin_addr, sizeof(struct in_addr))) 
    {
        reply(f, 425, 
          "Error accepting connection; connection from invalid IP.");
        close(socket_fd);
        return -1;
    }
#endif
    }

    return socket_fd;
}

#if 0
/* convert any '\n' to '\r\n' */
/* destination should be twice the size of the source for safety */
static int convert_newlines(char *dst, const char *src, int srclen)
{
    int i;
    int dstlen;

    daemon_assert(dst != NULL);
    daemon_assert(src != NULL);

    dstlen = 0;
    for (i=0; i<srclen; i++) {
        if (src[i] == '\n') {
        dst[dstlen++] = '\r';
    }
    dst[dstlen++] = src[i];
    }
    return dstlen;
}

static int write_fully(int fd, const char *buf, int buflen)
{
    int amt_written;
    int write_ret;

    amt_written = 0;
    while (amt_written < buflen) {
        write_ret = write(fd, buf+amt_written, buflen-amt_written);
    if (write_ret <= 0) {
        return -1;
    }
    amt_written += write_ret;
    }
    return amt_written;
}
#endif

static void do_pwd(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    reply(f, 257, "\"%s\" is current directory.", f->dir);

    daemon_assert(invariant(f));
}

#if 0
/* 
  because oftpd uses glob(), it is possible for users to launch a 
  denial-of-service attack by sending certain wildcard expressions that
  create extremely large lists of files, e.g. "*/../*/../*/../*/../*/../*"

  in order to prevent this, a user may pass wildcards, or paths, but not
  both as arguments to LIST or NLST - at most all the files from a single 
  directory will be returned
*/
#endif

/* check if a filespec has a wildcard in it */
static int filespec_has_wildcard(const char *filespec)
{
    daemon_assert(filespec != NULL);

    /* check each character for wildcard */
    while (*filespec != '\0') {
        switch (*filespec) {
        /* wildcards */
        case '*':
        case '?':
        case '[':
            return 1;

            /* backslash escapes next character unless at end of string */
            case '\\':
            if (*(filespec+1) != '\0') {
                filespec++;
        }
        break;
    }
    filespec++;
    }

    return 0;
}

/* filespec includes path separator, i.e. '/' */
static int filespec_has_path_separator(const char *filespec)
{
    daemon_assert(filespec != NULL);

    /* check each character for path separator */
    if (strchr(filespec, '/') != NULL) {
        return 1;
    } else {
        return 0;
    }
}

/* returns whether filespec is legal or not */
static int filespec_is_legal(const char *filespec)
{
    daemon_assert(filespec != NULL);

    if (filespec_has_wildcard(filespec)) {
        if (filespec_has_path_separator(filespec)) {
        return 0;
    }
    }
    return 1;
}

/* Misnamed. Actually all directory */
static int ShowDir( FILE * out, const struct parsedname * const pn )
{
    struct parsedname pncopy;
    int ret ;
    time_t now;
    /* Embedded function */
    /* Callback function to FS_dir */
    void directory( const struct parsedname * const pn2 ) {
        /* Have to allocate all buffers to make it work for Coldfire */
        char full_path[PATH_MAX+1+MAX_STRING_LEN];
	mode_t mode;
	struct tm tm_now;
	double age;
	char date_buf[13];
	struct stat st;
	char *extname ;
        if( !(extname = malloc(OW_FULLNAME_MAX+1)) ) { /* buffer for name */
	  return;
        }
        FS_DirName( extname, OW_FULLNAME_MAX+1, pn2 ) ;
	//fprintf( out, "%s\r\n", extname ) ;

	memset(&st, 0, sizeof(struct stat));
	get_absolute_fname(full_path, sizeof(full_path), pn->path, extname);

	// FS_fstat requires BUS_lock ! Deadlock here since FS_parsedname
	// called checkpresence()! Fixed I think by adding bus_nr to
	// cache in FS_dir at once!
	ret = FS_fstat(full_path, &st);
	if(ret < 0) {
	  memset(&st, 0, sizeof(struct stat));
	}
	mode = st.st_mode ;
	if ( (pn->type == pn_structure) ) {
	  mode &= ~0222;
	  mode |= 0444;
	}

        /* output file type */
	switch (mode & S_IFMT) {
        case S_IFSOCK:  fprintf( out, "s"); break;
        case S_IFLNK:   fprintf( out, "l"); break;
        case S_IFBLK:   fprintf( out, "b"); break;
        case S_IFDIR:   fprintf( out, "d"); break;
        case S_IFCHR:   fprintf( out, "c"); break;
        case S_IFIFO:   fprintf( out, "p"); break;
        default:        fprintf( out, "-"); 
	}

	/* output permissions */
	fprintf( out, (mode & S_IRUSR) ? "r" : "-");
	fprintf( out, (mode & S_IWUSR) ? "w" : "-");
	if (mode & S_ISUID) { 
	  fprintf( out, (mode & S_IXUSR) ? "s" : "S");
	} else {
	  fprintf( out, (mode & S_IXUSR) ? "x" : "-");
	}
	fprintf( out, (mode & S_IRGRP) ? "r" : "-");
	fprintf( out, (mode & S_IWGRP) ? "w" : "-");
	if (mode & S_ISGID) { 
	  fprintf( out, (mode & S_IXGRP) ? "s" : "S");
	} else {
	  fprintf( out, (mode & S_IXGRP) ? "x" : "-");
	}
	fprintf( out, (mode & S_IROTH) ? "r" : "-");
	fprintf( out, (mode & S_IWOTH) ? "w" : "-");
	if (mode & S_ISVTX) {
	  fprintf( out, (mode & S_IXOTH) ? "t" : "T");
	} else {
	  fprintf( out, (mode & S_IXOTH) ? "x" : "-");
	}
	
        /* output link & ownership information */
	fprintf( out, " %3d %-8d %-8d ", 
		 st.st_nlink, 
		 st.st_uid, 
		 st.st_gid
		 );

	fprintf( out, "%8lu ", (unsigned long)st.st_size);
        
        /* output date */
	localtime_r(&st.st_mtime, &tm_now);
        age = difftime(now, st.st_mtime);
	if ((age > 60 * 60 * 24 * 30 * 6) || (age < -(60 * 60 * 24 * 30 * 6))) {
	  strftime(date_buf, sizeof(date_buf), "%b %e  %Y", &tm_now);
	} else {
	  strftime(date_buf, sizeof(date_buf), "%b %e %H:%M", &tm_now);
	}
        fprintf( out, "%s ", date_buf);
	
	/* output filename */
	fprintf( out, "%s", extname);
  
	/* advance to next line */
	fprintf( out, "\r\n");
	free(extname);
    }

    memcpy(&pncopy, pn, sizeof(struct parsedname));
    time(&now);
    ret = FS_dir( directory, &pncopy ) ;
    return ret ;
}

static void do_nlst(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    int fd;
    const char *param;
    int send_ok;
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert((cmd->num_arg == 0) || (cmd->num_arg == 1));

    /* set up for exit */
    fd = -1;

    /* figure out what parameters to use */
    if (cmd->num_arg == 0) {
        param = "*";
    } else {
        daemon_assert(cmd->num_arg == 1);

    /* ignore attempts to send options to "ls" by silently dropping */
    if (cmd->arg[0].string[0] == '-') {
            param = "*";
    } else {
            param = cmd->arg[0].string;
    }
    }

    /* check spec passed */
    if (!filespec_is_legal(param)) {
        reply(f, 550, "Illegal filename passed.");
    goto exit_nlst;
    }


#if 1
    //printf("do_nlst\n");

    pn.si = &si ;
    ret = FS_ParsedName( f->dir, &pn ) ;
    // first root always return Bus-list and settings/system/statistics
    pn.si->sg |= (1<<BUSRET_BIT) ;

    if (ret || pn.ft) {
        reply(f, 550, "Error reading directory");
	goto exit_nlst;
    } else {
        FILE *out ;
	/* ready to transfer */
	reply(f, 150, "About to open data connection.");
	
        /* open data path */
        fd = open_connection(f);
	if (fd == -1) {
            goto exit_nlst;
	}
	out = fdopen(fd, "w") ;
	if (out == NULL) {
            goto exit_nlst;
	}
        ret = ShowDir( out, &pn ) ;

	//printf("showdir returned ret=%d\n", ret);
	fclose(out);
	send_ok = (ret >= 0) ;
    }
#else
    /* ready to list */
    reply(f, 150, "About to send name list.");

    /* open our data connection */
    fd = open_connection(f);
    if (fd == -1) {
        goto exit_nlst;
    }

    /* send any files */
    send_ok = file_nlst(fd, f->dir, param);
#endif

    /* strange handshake for Netscape's benefit */
    netscape_hack(fd);

    if (send_ok) {
        reply(f, 226, "Transfer complete.");
    } else {
        reply(f, 451, "Error sending name list.");
    }

    /* clean up and exit */
exit_nlst:
    if (fd != -1) {
        close(fd);
    }
    daemon_assert(invariant(f));
}

static void do_list(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    int fd;
    const char *param;
    int send_ok;
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;

    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert((cmd->num_arg == 0) || (cmd->num_arg == 1));

    /* set up for exit */
    fd = -1;

    /* figure out what parameters to use */
    if (cmd->num_arg == 0) {
        param = "*";
    } else {
        daemon_assert(cmd->num_arg == 1);

    /* ignore attempts to send options to "ls" by silently dropping */
    if (cmd->arg[0].string[0] == '-') {
            param = "*";
    } else {
            param = cmd->arg[0].string;
    }
    }

    /* check spec passed */
    if (!filespec_is_legal(param)) {
        reply(f, 550, "Illegal filename passed.");
    goto exit_list;
    }

#if 1
    //printf("do_list\n");
	
    pn.si = &si ;
    ret = FS_ParsedName( f->dir, &pn ) ;
    // first root always return Bus-list and settings/system/statistics
    pn.si->sg |= (1<<BUSRET_BIT) ;

    if (ret || pn.ft) {
        reply(f, 550, "Error reading directory");
	goto exit_list;
    } else {
        FILE *out ;
	/* ready to transfer */
	reply(f, 150, "About to open data connection.");
	
        /* open data path */
        fd = open_connection(f);
	if (fd == -1) {
            goto exit_list;
	}
	out = fdopen(fd, "w") ;
	if (out == NULL) {
            goto exit_list;
	}
        ret = ShowDir( out, &pn ) ;

	//printf("showdir returned ret=%d\n", ret);
	fclose(out);
	send_ok = (ret >= 0) ;
    }
#else
    /* ready to list */
    reply(f, 150, "About to send file list.");

    /* open our data connection */
    fd = open_connection(f);
    if (fd == -1) {
        goto exit_list;
    }

    /* send any files */
    send_ok = file_list(fd, f->dir, param);
#endif

    /* strange handshake for Netscape's benefit */
    netscape_hack(fd);

    if (send_ok) {
        reply(f, 226, "Transfer complete.");
    } else {
        reply(f, 451, "Error sending file list.");
    }

    /* clean up and exit */
exit_list:
    if (fd != -1) {
        close(fd);
    }
    daemon_assert(invariant(f));
}

static void do_syst(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    reply(f, 215, "UNIX");

    daemon_assert(invariant(f));
}


static void do_noop(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 0);

    reply(f, 200, "Command okay.");

    daemon_assert(invariant(f));
}

static void do_rest(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    if (f->data_type != TYPE_IMAGE) {
        reply(f, 555, "Restart not possible in ASCII mode.");
    } else if (f->file_structure != STRU_FILE) {
        reply(f, 555, "Restart only possible with FILE structure.");
    } else {
        f->file_offset = cmd->arg[0].offset;
        f->file_offset_command_number = f->command_number;
        reply(f, 350, "Restart okay, awaiting file retrieval request.");
    }

    daemon_assert(invariant(f));
}

static void do_size(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *file_name;
    char full_path[PATH_MAX+1+MAX_STRING_LEN];
    struct stat stat_buf;
    
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    if (f->data_type != TYPE_IMAGE) {
        reply(f, 550, "Size cannot be determined in ASCII mode.");
    } else if (f->file_structure != STRU_FILE) {
        reply(f, 550, "Size cannot be determined with FILE structure.");
    } else {

        /* create an absolute name for our file */
        file_name = cmd->arg[0].string;
        get_absolute_fname(full_path, sizeof(full_path), f->dir, file_name);

        /* get the file information */
        if (FS_fstat(full_path, &stat_buf) != 0) {
            reply(f, 550, "Error getting file status; %s.", strerror(errno));
        } else {

            /* verify that the file is not a directory */
            if (S_ISDIR(stat_buf.st_mode)) {
                reply(f, 550, "File is a directory, SIZE command not valid.");
            } else {

                /* output the size */
                if (sizeof(off_t) == 8) {
                    reply(f, 213, "%llu", stat_buf.st_size);
                } else {
                    reply(f, 213, "%lu", stat_buf.st_size);
                }
            }

        }

    }

    daemon_assert(invariant(f));
}

/* if no gmtime_r() is available, provide one */
#ifndef HAVE_GMTIME_R
struct tm *gmtime_r(const time_t *timep, struct tm *timeptr) 
{
    static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&time_lock);
    *timeptr = *(gmtime(timep));
    pthread_mutex_unlock(&time_lock);
    return timeptr;
}
#endif /* HAVE_GMTIME_R */

static void do_mdtm(struct ftp_session_t *f, const struct ftp_command_t *cmd) 
{
    const char *file_name;
    char full_path[PATH_MAX+1+MAX_STRING_LEN];
    struct stat stat_buf;
    struct tm mtime;
    char time_buf[16];
    
    daemon_assert(invariant(f));
    daemon_assert(cmd != NULL);
    daemon_assert(cmd->num_arg == 1);

    /* create an absolute name for our file */
    file_name = cmd->arg[0].string;
    get_absolute_fname(full_path, sizeof(full_path), f->dir, file_name);

    /* get the file information */
    if (FS_fstat(full_path, &stat_buf) != 0) {
        reply(f, 550, "Error getting file status; %s.", strerror(errno));
    } else {
        gmtime_r(&stat_buf.st_mtime, &mtime);
        strftime(time_buf, sizeof(time_buf), "%Y%m%d%H%M%S", &mtime);
        reply(f, 213, time_buf);
    }

    daemon_assert(invariant(f));
}

#if 0
static void send_readme(const struct ftp_session_t *f, int code)
{
    char file_name[PATH_MAX+1];
    int dir_len;
    struct stat stat_buf;
    int fd;
    int read_ret;
    char buf[4096];
    char code_str[8];
    char *p;
    int len;
    char *nl;
    int line_len;

    daemon_assert(invariant(f));
    daemon_assert(code >= 100);
    daemon_assert(code <= 559);

    /* set up for early exit */
    fd = -1;

    /* verify our README wouldn't be too long */
    dir_len = strlen(f->dir);
    if ((dir_len + 1 + sizeof(README_FILE_NAME)) > sizeof(file_name)) {
        goto exit_send_readme;
    }

    /* create a README file name */
    strcpy(file_name, f->dir);
    strcat(file_name, "/");
    strcat(file_name, README_FILE_NAME);

    /* open our file */
    fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        goto exit_send_readme;
    }

    /* verify this isn't a directory */
    if (FS_fstat(file_name, &stat_buf) != 0) {
        goto exit_send_readme;
    }
#ifndef STATS_MACRO_BROKEN
    if (S_ISDIR(stat_buf.st_mode)) {
#else
    if (!S_ISDIR(stat_buf.st_mode)) {
#endif
    goto exit_send_readme;
    }

    /* convert our code to a buffer */
    daemon_assert(code >= 100);
    daemon_assert(code <= 999);
    sprintf(code_str, "%03d-", code);

    /* read and send */
    read_ret = read(fd, buf, sizeof(buf));
    if (read_ret > 0) {
        telnet_session_print(f->telnet_session, code_str);
        while (read_ret > 0) {
            p = buf;
        len = read_ret;
            nl = memchr(p, '\n', len);
        while ((len > 0) && (nl != NULL)) {
            *nl = '\0';
            telnet_session_println(f->telnet_session, p);
            line_len = nl - p;
            len -= line_len + 1;
            if (len > 0) {
                telnet_session_print(f->telnet_session, code_str);
                }
        p = nl+1;
                nl = memchr(p, '\n', len);
        }
        if (len > 0) {
            telnet_session_print(f->telnet_session, p);
        }

            read_ret = read(fd, buf, sizeof(buf));
        }
    }

    /* cleanup and exit */
exit_send_readme:
    if (fd != -1) {
        close(fd);
    }
    daemon_assert(invariant(f));
}
#endif

/* hack which prevents Netscape error in file list */
static void netscape_hack(int fd)
{
    fd_set readfds;
    struct timeval _timeout;
    int select_ret;
    char c;

    daemon_assert(fd >= 0);

    shutdown(fd, 1);
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    _timeout.tv_sec = 15;
    _timeout.tv_usec = 0;
    select_ret = select(fd+1, &readfds, NULL, NULL, &_timeout);
    if (select_ret > 0) {
        read(fd, &c, 1);
    }
}

/* compare two addresses to see if they contain the same IP address */
static int ip_equal(const sockaddr_storage_t *a, const sockaddr_storage_t *b)
{
    daemon_assert(a != NULL);
    daemon_assert(b != NULL);
    daemon_assert((SSFAM(a) == AF_INET) || (SSFAM(a) == AF_INET6));
    daemon_assert((SSFAM(b) == AF_INET) || (SSFAM(b) == AF_INET6));

    if (SSFAM(a) != SSFAM(b)) {
        return 0;
    }
    if (memcmp(&SINADDR(a), &SINADDR(b), sizeof(SINADDR(a))) != 0) {
        return 0;
    }
    return 1;
}

