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

#define SVERSION "owhttpd"

#include "owhttpd.h"

/* ------------ Protoypes ---------------- */

    /* Utility HTML page display functions */
void HTTPstart(FILE * out, const char * status, const unsigned int text ) {
    char d[44] ;
    time_t t = time(NULL) ;
    size_t l = strftime(d,sizeof(d),"%a, %d %b %Y %T GMT",gmtime(&t)) ;

    fprintf(out, "HTTP/1.0 %s\r\n", status);
    fprintf(out, "Date: %*s\n", (int)l, d );
    fprintf(out, "Server: %s\r\n", SVERSION);
    fprintf(out, "Last-Modified: %*s\r\n", (int)l, d );
    /*
     * fprintf( out, "MIME-version: 1.0\r\n" );
     */
    if(text)
      fprintf(out, "Content-Type: text/plain\r\n");
    else
      fprintf(out, "Content-Type: text/html\r\n");
    fprintf(out, "\r\n");
}

void HTTPtitle(FILE * out, const char * title ) {
    fprintf(out, "<HTML><HEAD><TITLE>1-Wire Web: %s</TITLE></HEAD>\n", title);
}

void HTTPheader(FILE * out, const char * head ) {
    fprintf(out, "<BODY "BODYCOLOR"><TABLE "TOPTABLE"><TR><TD>OWFS on %s</TD><TD><A HREF='/'>Bus listing</A></TD><TD><A HREF='http://owfs.sourceforge.net'>OWFS homepage</A></TD><TD><A HREF='http://www.maxim-ic.com'>Dallas/Maxim</A></TD><TD>by <A HREF='mailto://palfille@earthlink.net'>Paul H Alfille</A></TD></TR></TABLE>\n", SimpleBusName);
    fprintf(out, "<H1>%s</H1><HR>\n", head);
}

void HTTPfoot(FILE * out ) {
    fprintf(out, "</BODY></HTML>") ;
}


/* reads an as ascii hex string, strips out non-hex, converts in place */
void hex_convert( char * str ) {
    char * uc = str ;
    unsigned char * hx = str ;
    for( ; *uc ; uc+=2 ) *hx++ = string2num( uc ) ;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
void hex_only( char * str ) {
    char * uc = str ;
    char * hx = str ;
    while( *uc ) {
        if ( isxdigit(*uc) ) *hx++ = *uc ;
        uc ++ ;
    }
    *hx = '\0' ;
}

/* Change web-escaped string back to straight ascii */
int httpunescape(unsigned char *httpstr) {
    unsigned char *in = httpstr ;          /* input string pointer */
    unsigned char *out = httpstr ;          /* output string pointer */

    while(*in)
    {
        switch (*in) {
        case '%':
            ++ in ;
            if ( in[0]=='\0' || in[1]=='\0' ) return 1 ;
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
