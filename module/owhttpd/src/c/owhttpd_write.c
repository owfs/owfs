/*
$Id$
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

// #include <libgen.h>  /* for dirname() */
    /* string format functions */
static void hex_convert( char * str ) ;
static int httpunescape(BYTE *httpstr) ;
static void hex_only( char * str ) ;

/* --------------- Functions ---------------- */


void ChangeData( char * value, const struct parsedname * pn ) {
/* Do command processing and make changes to 1-wire devices */
    if ( pn->ft && value ) {
        httpunescape((BYTE*)value) ;
        LEVEL_DETAIL("CHANGEDATA path=%s value=%s\n",pn->path,value);
        switch ( pn->ft->format ) {
            case ft_binary:
                hex_only(value) ;
                if ( (int)strlen(value) == (pn->ft->suglen<<1) ) {
                    hex_convert(value) ;
                    FS_write_postparse( value, (size_t) pn->ft->suglen, 0, pn ) ;
                }
                break;
            case ft_yesno:
            case ft_bitfield:
                if ( pn->extension>=0 ) { // Single check box handled differently
                    FS_write_postparse( strncasecmp(value,"on",2)?"0":"1", 2, 0, pn ) ;
                    break;
                }
                // fall through
            default:
                FS_write_postparse(value,strlen(value)+1,0,pn) ;
                break;
        }
    }
}

/* Change web-escaped string back to straight ascii */
static int httpunescape(BYTE *httpstr) {
    BYTE *in = httpstr ;          /* input string pointer */
    BYTE *out = httpstr ;          /* output string pointer */

    while(*in)
    {
        switch (*in) {
            case '%':
                ++ in ;
                if ( in[0]=='\0' || in[1]=='\0' ) {
                    *out = '\0';
                    return 1 ;
                }
                *out++ = string2num((char *) in ) ;
                ++ in ;
                break ;
            case '+':
                *out++ = ' ';
                break ;
            default:
                *out++ = *in;
                break ;
        }
        in++;
    }
    *out = '\0';
    return 0;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_only( char * str ) {
    char * uc = str ;
    char * hx = str ;
    while( *uc ) {
        if ( isxdigit(*uc) ) *hx++ = *uc ;
        uc ++ ;
    }
    *hx = '\0' ;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_convert( char * str ) {
    char * uc = str ;
    BYTE * hx = (BYTE *) str ;
    for( ; *uc ; uc+=2 ) *hx++ = string2num( uc ) ;
}
