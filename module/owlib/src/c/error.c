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

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

/* See man page for explanation */
int log_available = 0;

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
#define MAXLINE     1023
void err_msg(enum e_err_type errnoflag, enum e_err_level level, const char *fmt, ...)
{
	int errno_save = errno;		/* value caller might want printed */
	int n;
	char buf[MAXLINE + 1];
	enum e_err_print sl;		// 2=console 1=syslog
	va_list ap;

	/* Print where? */
	switch (Globals.error_print) {
	case e_err_print_mixed:
		sl = Globals.now_background ? e_err_print_syslog : e_err_print_console;
		break;
	case e_err_print_syslog:
		sl = e_err_print_syslog;
		break;
	case e_err_print_console:
		sl = e_err_print_console;
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
	vsprintf(buf, fmt, ap);		/* not safe */
#endif
	n = strlen(buf);
	if (errnoflag == e_err_type_error) {
		snprintf(buf + n, MAXLINE - n, " -> %s\n", strerror(errno_save));
	}
	UCLIBCUNLOCK;
	va_end(ap);

	if (sl == e_err_print_syslog) {	/* All output to syslog */
		strcat(buf, "\n");
		if (!log_available) {
			openlog("OWFS", LOG_PID, LOG_DAEMON);
			log_available = 1;
		}
		syslog(level <= e_err_default ? LOG_INFO : LOG_NOTICE, buf);
	} else {
		fflush(stdout);			/* in case stdout and stderr are the same */
		switch (level) {
		case e_err_default:
			fputs("DEFAULT: ", stderr);
			break;
		case e_err_connect:
			fputs("CONNECT: ", stderr);
			break;
		case e_err_call:
			fputs("   CALL: ", stderr);
			break;
		case e_err_data:
			fputs("   DATA: ", stderr);
			break;
		case e_err_detail:
			fputs(" DETAIL: ", stderr);
			break;
		case e_err_debug:
		case e_err_beyond:
		default:
			fputs("  DEBUG: ", stderr);
			break;
		}
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}

/* Purely a debugging routine -- print an arbitrary buffer of bytes */
void _Debug_Bytes(const char *title, const unsigned char *buf, int length)
{
	int i;
	/* title line */
	fprintf(stderr,"Byte buffer %s, length=%d", title ? title : "anonymous", (int) length);
	if (length < 0) {
		fprintf(stderr,"\n-- Attempt to write with bad length\n");
		return;
	}
	if (buf == NULL) {
		fprintf(stderr,"\n-- NULL buffer\n");
		return;
	}
#if 1
	/* hex lines -- 16 bytes each */
	for (i = 0; i < length; ++i) {
		if ((i & 0x0F) == 0) {	// devisible by 16
			fprintf(stderr,"\n--%3.3d:",i);
		}
		fprintf(stderr," %.2X", (unsigned char)buf[i]);

		if(i >= 63) break;
		/* Sorry for this, but I think it's better to strip off all huge 8192 packages in the debug-output. */
	}

#endif
	/* char line -- printable or . */
	fprintf(stderr,"\n   <");
	for (i = 0; i < length; ++i) {
		char c = buf[i];
		fprintf(stderr,"%c", isprint(c) ? c : '.');
		if(i >= 63) break;
		/* Sorry for this, but I think it's better to strip off all huge 8192 packages in the debug-output. */
	}
	fprintf(stderr,">\n");
}

/* Purely a debugging routine -- print an arbitrary buffer of bytes */
void _Debug_Writev(struct iovec *io, int iosz)
{
	/* title line */
	int i, ionr = 0;
	char *buf;
	int length;
	
	while(ionr < iosz) {
		buf = io[ionr].iov_base;
		length = io[ionr].iov_len;
		
		fprintf(stderr,"Writev byte buffer ionr=%d/%d length=%d", ionr+1, iosz, (int) length);
		if (length <= 0) {
			fprintf(stderr,"\n-- Attempt to write with bad length\n");
		}
		if (buf == NULL) {
			fprintf(stderr,"\n-- NULL buffer\n");
		}
#if 1
		/* hex lines -- 16 bytes each */
		for (i = 0; i < length; ++i) {
			if ((i & 0x0F) == 0) {	// devisible by 16
				fprintf(stderr,"\n--%3.3d:",i);
			}
			fprintf(stderr," %.2X", (unsigned char)buf[i]);
		}
		
#endif
		/* char line -- printable or . */
		fprintf(stderr,"\n   <");
		for (i = 0; i < length; ++i) {
			char c = buf[i];
			fprintf(stderr,"%c", isprint(c) ? c : '.');
		}
		fprintf(stderr,">\n");
		ionr++;
	}
}

void fatal_error(char *file, int row, const char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	char filename[64];
	char tmp[256];
	va_start(ap, fmt);

	if(Globals.fatal_debug) {
		LEVEL_DEFAULT("%s:%d ", file, row);
#ifdef HAVE_VSNPRINTF
		vsnprintf(tmp, sizeof(tmp), fmt, ap);
#else
		vsprintf(tmp, fmt, ap);
#endif
		LEVEL_DEFAULT(tmp);
	}


	if(Globals.fatal_debug_file != NULL) {
		sprintf(filename, "%s.%d", Globals.fatal_debug_file, getpid());
		if((fp = fopen(filename, "a")) != NULL) {
			if(!Globals.fatal_debug) {
#ifdef HAVE_VSNPRINTF
				vsnprintf(tmp, sizeof(tmp), fmt, ap);
#else
				vsprintf(tmp, fmt, ap);
#endif
			}
			fprintf(fp, "%s:%d %s\n", file, row, tmp);
			fclose(fp);
		}
	}
}


