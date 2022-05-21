/*
 * http.c for owhttpd (1-wire web server)
 * By Paul Alfille 2003, using libow
 * offshoot of the owfs ( 1wire file system )
 *
 * GPL license ( Gnu Public Lincense )
 *
 * Based on chttpd. copyright(c) 0x7d0 greg olszewski <noop@nwonknu.org>
 *
 */

#include "owhttpd.h"

/* Change web-escaped string back to straight ascii */
/* Works in-place since the final string is never longer */
void httpunescape(BYTE * httpstr)
{
	BYTE *in = httpstr;			/* input string pointer */
	BYTE *out = httpstr;		/* output string pointer */

	while (in) {
		switch (*in) {
		case '+':
			*out++ = ' ';
			break;
		case '\0':
			*out++ = '\0';
			return;
		case '%':
			if ( isxdigit(in[1]) && isxdigit(in[2]) ) {
				++in;
				*out++ = string2num((char *) in);
				++in;
				break ;
			}
			// fall through
		default:
			*out++ = *in;
			break;
		}
		in++;
	}
}

/* HTTP encode */
char * httpescape( const char * original_string )
{
	const char * original_p = original_string ;
	char * escaped_string ;
	char * escaped_p ;

	if ( original_string == NULL ) {
		return NULL ;
	}

	escaped_p = escaped_string = owmalloc( strlen(original_string)*3 + 1 ) ;

	if ( escaped_string == NULL ) {
		return NULL ;
	}

	while (1) {
		switch ( *original_p ) {
			case '\0':
				*escaped_p = '\0' ;
				return escaped_string ;
			case ' ':
			case '!':
			case '"':
			case '#':
			case '$':
			case '%':
			case '@':
			case '\'':
			case '(':
			case ')':
			case '<':
			case '>':
			case ';':
			case ':':
			case '+':
			case ',':
			case '=':
				UCLIBCLOCK;
				escaped_p += sprintf( escaped_p, "%%%.2X", *original_p++ ) ;
				UCLIBCUNLOCK;
				break ;
			default:
				*escaped_p++ = *original_p++ ;
				break ;
		}
	}
}

				
