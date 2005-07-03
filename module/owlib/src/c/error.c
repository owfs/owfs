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

/* error.c stolen nearly verbatim from
    "Unix Network Programming Volume 1 The Sockets Networking API (3rd Ed)"
   by W. Richard Stevens, Bill Fenner, Andrew M Rudoff
  Addison-Wesley Professional Computing Series
  Addison-Wesley, Boston 2003
  http://www.unpbook.com
*/ 

#include    <owfs_config.h>
#include    <ow.h>
#include    <stdarg.h>

/* See man page for explanation */
int error_print = 0 ;
int error_level = 0 ;
int log_available = 0 ;
static void    err_doit(int, int, const char *, va_list);

/* Nonfatal error related to system call
 * Print message and return */
void err_msg(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to system call
 * Print message and return */
void err_bad(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
}

/* Fatal error related to system call
 * Print message and return */
void err_debug(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
#define MAXLINE     120
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap) {
    int     errno_save = errno ;  /* value caller might want printed */
    int     n ;
    char    buf[MAXLINE + 1];
    int     sl ;

    switch ( error_print ) {
    case 0 :
        sl = now_background ;
        break ;
    case 1 :
        sl = 1 ;
        break ;
    case 2 :
        sl = 0 ;
        break ;
    case 3 :
        return ;
    }

    UCLIBCLOCK;
#ifdef    HAVE_VSNPRINTF
        vsnprintf(buf, MAXLINE, fmt, ap);    /* safe */
#else
        vsprintf(buf, fmt, ap);              /* not safe */
#endif
    UCLIBCUNLOCK;
    n = strlen(buf);
    if (errnoflag) {
        UCLIBCLOCK;
            snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
        UCLIBCUNLOCK;
    }

    strcat(buf, "\n");

    if (sl) {    /* All output to syslog */
        if ( ! log_available) {
            openlog( "OWFS" , LOG_PID , LOG_DAEMON ) ;
            log_available = 1 ;
        }
        syslog(level, buf);
    } else {
        fflush(stdout);        /* in case stdout and stderr are the same */
        fputs((level==LOG_INFO)?"INFO: ":"ERR: ", stderr);
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}
