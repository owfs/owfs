/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
  * 
  * Although it's been considerably modified over time -- don't blame the authors''
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include <stdarg.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

const char mutex_init_failed[] = "mutex_init failed rc=%d [%s]\n";
const char mutex_destroy_failed[] = "mutex_destroy failed rc=%d [%s]\n";
const char mutex_lock_failed[] = "mutex_lock failed rc=%d [%s]\n";
const char mutex_unlock_failed[] = "mutex_unlock failed rc=%d [%s]\n";
const char mutexattr_init_failed[] = "mutexattr_init failed rc=%d [%s]\n";
const char mutexattr_destroy_failed[] = "mutexattr_destroy failed rc=%d [%s]\n";
const char mutexattr_settype_failed[] = "mutexattr_settype failed rc=%d [%s]\n";
const char rwlock_init_failed[] = "rwlock_init failed rc=%d [%s]\n";
const char rwlock_read_lock_failed[] = "rwlock_read_lock failed rc=%d [%s]\n";
const char rwlock_read_unlock_failed[] = "rwlock_read_unlock failed rc=%d [%s]\n";
const char cond_timedwait_failed[] = "cond_timedwait failed rc=%d [%s]\n";
const char cond_signal_failed[] = "cond_signal failed rc=%d [%s]\n";
const char cond_broadcast_failed[] = "cond_broadcast failed rc=%d [%s]\n";
const char cond_wait_failed[] = "cond_wait failed rc=%d [%s]\n";
const char cond_init_failed[] = "cond_init failed rc=%d [%s]\n";
const char cond_destroy_failed[] = "cond_destroy failed rc=%d [%s]\n";
const char sem_init_failed[] = "sem_init failed rc=%d [%s]\n";
const char sem_post_failed[] = "sem_post failed rc=%d [%s]\n";
const char sem_wait_failed[] = "sem_wait failed rc=%d [%s]\n";
const char sem_trywait_failed[] = "sem_trywait failed rc=%d [%s]\n";
const char sem_timedwait_failed[] = "sem_timedwait failed rc=%d [%s]\n";
const char sem_destroy_failed[] = "sem_destroy failed rc=%d [%s]\n";

static void err_format(char * format, int errno_save, const char * level_string, const char * file, int line, const char * func, const char * fmt);
static void hex_print( const char * buf, int length ) ;
static void ascii_print( const char * buf, int length ) ;

/* See man page for explanation */
int log_available = 0;

/* Limits for prettier byte printing */
#define HEX_PRINT_BYTES_PER_LINE 16
#define HEX_PRINT_MAX_LINES       4

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
#define MAXLINE     1023

void print_timestamp_(const char * file, int line, const char * func, const char *fmt, ...)
{
	struct timeval tv;
	char buf[MAXLINE + 3];
	char format[MAXLINE + 3];
	va_list va;

	gettimeofday(&tv, NULL);
	snprintf(format, MAXLINE, "%s:%s(%d) %s", file,func,line,fmt);  /* safe */

	va_start(va, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, format, va);    /* safe */
#else
	vsprintf(buf, format, va);      /* not safe */
#endif
	va_end(va);

	fprintf(stderr, "%ld DEFAULT: %s %ld.%06ld\n", time(NULL), buf, tv.tv_sec, tv.tv_usec);
	fflush(stderr);
}

void err_msg(enum e_err_type errnoflag, enum e_err_level level, const char * file, int line, const char * func, const char *fmt, ...)
{
	int errno_save = (errnoflag==e_err_type_error)?errno:0;		/* value caller might want printed */
	char format[MAXLINE + 3];
	char buf[MAXLINE + 3];
	va_list ap;
	const char * level_string ;

	switch (level) {
		case e_err_default:
			level_string = "DEFAULT: ";
			break;
		case e_err_connect:
			level_string = "CONNECT: ";
			break;
		case e_err_call:
			level_string = "   CALL: ";
			break;
		case e_err_data:
			level_string = "   DATA: ";
			break;
		case e_err_detail:
			level_string = " DETAIL: ";
			break;
		case e_err_debug:
		case e_err_beyond:
		default:
			level_string = "  DEBUG: ";
			break;
	}

	va_start(ap, fmt);
	err_format( format, errno_save, level_string, file, line, func, fmt) ;

	UCLIBCLOCK;
	/* Create output string */
#ifdef    HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, format, ap);	/* safe */
#else
	vsprintf(buf, format, ap);		/* not safe */
#endif
	UCLIBCUNLOCK;
	va_end(ap);
//printf("About to output an error \n");

	fputs(buf, stderr);
	fflush(stderr);
	return;
}

/* Purely a debugging routine -- print an arbitrary buffer of bytes */
void _Debug_Bytes(const char *title, const unsigned char *buf, int length)
{
	/* title line */
	fprintf(stderr,"Byte buffer %s, length=%d", title ? title : "anonymous", (int) length);
	if (length < 0) {
		fprintf(stderr,"\n-- Attempt to write with bad length\n");
		return;
	} else if ( length == 0 ) {
		fprintf(stderr,"\n");
		return ;
	}
	if (buf == NULL) {
		fprintf(stderr,"\n-- NULL buffer\n");
		return;
	}

	hex_print( (const char *) buf, length ) ;
	ascii_print( (const char *) buf, length ) ;
}

/* calls exit() so never returns */
void fatal_error(const char * file, int line, const char * func, const char *fmt, ...)
{
	va_list ap;
	char format[MAXLINE + 1];
	char buf[MAXLINE + 1];
	va_start(ap, fmt);

	err_format( format, 0, "FATAL ERROR: ", file, line, func, fmt) ;

#ifdef OWNETC_OW_DEBUG
	{
		fprintf(stderr, "%s:%d ", file, line);
#ifdef HAVE_VSNPRINTF
		vsnprintf(buf, MAXLINE, format, ap);
#else
		vsprintf(buf, fmt, ap);
#endif
		fprintf(stderr, "%s", buf);
	}
#else /* OWNETC_OW_DEBUG */
	if(Globals.fatal_debug) {

#ifdef HAVE_VSNPRINTF
		vsnprintf(buf, MAXLINE, format, ap);
#else
		vsprintf(buf, format, ap);
#endif
		/* Print where? */
		switch (Globals.error_print) {
			case e_err_print_mixed:
				switch (Globals.daemon_status) {
					case e_daemon_want_bg:
					case e_daemon_bg:
						sl = e_err_print_syslog ;
						break ;
					default:
						sl = e_err_print_console;
						break ;
				}		
				break;
			case e_err_print_syslog:
				sl = e_err_print_syslog;
				break;
			case e_err_print_console:
				sl = e_err_print_console;
				break;
			default:
				va_end(ap);
				return;
		}

		if (sl == e_err_print_syslog) {	/* All output to syslog */
			if (!log_available) {
				openlog("OWFS", LOG_PID, LOG_DAEMON);
				log_available = 1;
			}
			syslog(LOG_USER|LOG_INFO, "%s\n", buf);
		} else {
			fflush(stdout);			/* in case stdout and stderr are the same */
			fputs(buf, stderr);
			fprintf(stderr,"\n");
			fflush(stderr);
		}
	}


	if(Globals.fatal_debug_file != NULL) {
		FILE *fp;
		char filename[64];
		sprintf(filename, "%s.%d", Globals.fatal_debug_file, getpid());
		if((fp = fopen(filename, "a")) != NULL) {
			if(!Globals.fatal_debug) {
#ifdef HAVE_VSNPRINTF
				vsnprintf(buf, MAXLINE, format, ap);
#else
				vsprintf(buf, format, ap);
#endif
			}
			fprintf(fp, "%s:%d %s\n", file, line, buf);
			fclose(fp);
		}
	}
#endif /* OWNETC_OW_DEBUG */
	va_end(ap);
	debug_crash(); // Core-dump to make it possible to trace down the problem!
	//exit(EXIT_FAILURE) ;
}

static void err_format(char * format, int errno_save, const char * level_string, const char * file, int line, const char * func, const char * fmt)
{
	UCLIBCLOCK;
	/* Create output string */
#ifdef    HAVE_VSNPRINTF
	if (errno_save) {
		snprintf(format, MAXLINE, "%s%s:%s(%d) [%s] %s", level_string,file,func,line,strerror(errno_save),fmt);	/* safe */
	} else {
		snprintf(format, MAXLINE, "%s%s:%s(%d) %s", level_string,file,func,line,fmt);	/* safe */
	}
#else
	if (errno_save) {
		sprintf(format, "%s%s:%s(%d) [%s] %s", level_string,file,func,line,strerror(errno_save),fmt);		/* not safe */
	} else {
		sprintf(format, "%s%s:%s(%d) %s", level_string,file,func,line,fmt);		/* not safe */
	}
#endif
	UCLIBCUNLOCK;
	/* Add CR at end */
}


static void hex_print( const char * buf, int length )
{
	int i = 0 ;
	/* hex lines */
	for (i = 0; i < length; ++i) {
		if ((i % HEX_PRINT_BYTES_PER_LINE) == 0) {	// switch lines
			fprintf(stderr,"\n--%3.3d:",i);
		}
		fprintf(stderr," %.2X", (unsigned char)buf[i]);

		if( i >= ( HEX_PRINT_BYTES_PER_LINE * HEX_PRINT_MAX_LINES -1 ) ) {
			/* Sorry for this, but I think it's better to strip off all huge 8192 packages in the debug-output. */
			fprintf(stderr,"\n--%3.3d: == abridged ==",i);
			break;
		}
	}
}

static void ascii_print( const char * buf, int length )
{	
	int i ;
	/* char line -- printable or . */
	fprintf(stderr,"\n   <");
	for (i = 0; i < length; ++i) {
		char c = buf[i];
		fprintf(stderr,"%c", isprint(c) ? c : '.');
		if(i >= ( HEX_PRINT_BYTES_PER_LINE * HEX_PRINT_MAX_LINES -1 )) {
			/* Sorry for this, but I think it's better to strip off all huge 8192 packages in the debug-output. */
			break;
		}
	}
	fprintf(stderr,">\n");
}
