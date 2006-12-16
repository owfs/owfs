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

#include <config.h>
#include <owfs_config.h>
#include <ow.h>
#include <stdarg.h>

/* See man page for explanation */
int log_available = 0;

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
#define MAXLINE     1023
void err_msg(int errnoflag, int level, const char *fmt, ...)
{
    int errno_save = errno;	/* value caller might want printed */
    int n;
    char buf[MAXLINE + 1];
    int sl;			// 0=console 1=syslog
    va_list ap;

    /* Print where? */
    switch (Global.error_print) {
    case 0:
	sl = Global.now_background;
	break;
    case 1:
	sl = 1;
	break;
    case 2:
	sl = 0;
	break;
    default:
	return;
    }

    va_start(ap, fmt);
    UCLIBCLOCK;
    /* Create output string */
#ifdef    HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
#else
    vsprintf(buf, fmt, ap);	/* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
	snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    UCLIBCUNLOCK;
    va_end(ap);

    if (sl) {			/* All output to syslog */
	strcat(buf, "\n");
	if (!log_available) {
	    openlog("OWFS", LOG_PID, LOG_DAEMON);
	    log_available = 1;
	}
	syslog(errnoflag ? LOG_INFO : LOG_NOTICE, buf);
    } else {
	fflush(stdout);		/* in case stdout and stderr are the same */
	switch (level) {
	case 0:
	    fputs("DEFAULT: ", stderr);
	    break;
	case 1:
	    fputs("CONNECT: ", stderr);
	    break;
	case 2:
	    fputs("   CALL: ", stderr);
	    break;
	case 3:
	    fputs("   DATA: ", stderr);
	    break;
	case 4:
	    fputs(" DETAIL: ", stderr);
	    break;
	default:
	    fputs("  DEBUG: ", stderr);
	    break;
	}
	fputs(buf, stderr);
	fflush(stderr);
    }
    return;
}
