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

static void    err_doit(int, int, const char *, va_list);

/* Nonfatal error related to system call
 * Print message and return */

void err_ret(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to system call
 * Print message and terminate */

void err_sys(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void err_dump(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort();        /* dump core and terminate */
    exit(1);        /* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void err_msg(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void err_quit(const char *fmt, ...) {
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
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

    UCLIBCLOCK
#ifdef    HAVE_VSNPRINTF
        vsnprintf(buf, MAXLINE, fmt, ap);    /* safe */
#else
        vsprintf(buf, fmt, ap);                    /* not safe */
#endif
    UCLIBCUNLOCK
    n = strlen(buf);
    if (errnoflag) {
        UCLIBCLOCK
            snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
        UCLIBCUNLOCK
    }

    strcat(buf, "\n");

    if (sl) {
        syslog(level, buf);
    } else {
        fflush(stdout);        /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}
