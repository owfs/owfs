/*
 * $Id$
 *
 * Restrictions:
 *  - Only stream MODE is supported.
 *  - Only "ftp" or "anonymous" accepted as a user.
 */

#ifndef FTP_SESSION_H
#define FTP_SESSION_H

#include <netinet/in.h>
#include <sys/types.h>
#include <limits.h>
#include "af_portability.h"
#include "watchdog.h"
#include "ftp_error.h"

/* data representation types supported */
#define TYPE_ASCII  0
#define TYPE_IMAGE  1

/* file structure types supported */
#define STRU_FILE   0
#define STRU_RECORD 1

/* data path chosen */
#define DATA_PORT     0
#define DATA_PASSIVE  1

/* space required for text representation of address and port,
   e.g. "192.168.0.1 port 1024" or
        "2001:3333:DEAD:BEEF:0666:0013:0069:0042 port 65535" */
#define ADDRPORT_STRLEN 58

/* structure encapsulating an FTP session's information */
struct ftp_session_t {
    /* flag whether session is active */
    int session_active;

    /* incremented for each command */
    unsigned long command_number;

    /* options about transfer set by user */
    int data_type;
    int file_structure;

    /* offset to begin sending file from */
    off_t file_offset;
    unsigned long file_offset_command_number;

    /* flag set if client requests ESPV ALL - this prevents subsequent
       use of PORT, PASV, LPRT, LPSV, or EPRT */
    int epsv_all_set;

    /* address of client */
    sockaddr_storage_t client_addr;
    char client_addr_str[ADDRPORT_STRLEN];

    /* address of server (including IPv4 version) */
    sockaddr_storage_t server_addr;
    struct sockaddr_in server_ipv4_addr;

    /* telnet session to encapsulate control channel logic */
    struct telnet_session_t *telnet_session;

    /* current working directory of this connection */
    char dir[PATH_MAX+1];

    /* data channel information, including type,
      and client address or server port depending on type */
    int data_channel;
    sockaddr_storage_t data_port;
    int server_fd;

    /* watchdog to handle timeout */
    watched_t *watched;
} ;

int ftp_session_init(struct ftp_session_t *f,
                     const sockaddr_storage_t *client_addr,
                     const sockaddr_storage_t *server_addr, 
                     struct telnet_session_t *t,
                     const char *dir,
                     error_code_t *err);
void ftp_session_drop(struct ftp_session_t *f, const char *reason);
void ftp_session_run(struct ftp_session_t *f, watched_t *watched);
void ftp_session_destroy(struct ftp_session_t *f);

#endif /* FTP_SESSION_H */

