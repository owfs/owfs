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
void HTTPstart(FILE * out, const char *status, const enum content_type ct)
{
	char d[44];
	time_t t = NOW_TIME;
	size_t l = strftime(d, sizeof(d), "%a, %d %b %Y %T GMT", gmtime(&t));

	fprintf(out, "HTTP/1.0 %s\r\n", status);
	fprintf(out, "Date: %*s\n", (int) l, d);
	fprintf(out, "Server: %s\r\n", SVERSION);
	fprintf(out, "Last-Modified: %*s\r\n", (int) l, d);
	/*
	 * fprintf( out, "MIME-version: 1.0\r\n" );
	 */
	switch (ct) {
	case ct_html:
		fprintf(out, "Content-Type: text/html\r\n");
		break;
	case ct_icon:
		fprintf(out, "Content-Length: 894\r\n");
		fprintf(out, "Connection: close\r\n");
		break ;
	case ct_text:
		fprintf(out, "Content-Type: text/plain\r\n");
		break ;
	}
	fprintf(out, "\r\n");
}

void HTTPtitle(FILE * out, const char *title)
{
	fprintf(out, "<HTML><HEAD><TITLE>1-Wire Web: %s</TITLE></HEAD>\n", title);
}

void HTTPheader(FILE * out, const char *head)
{
	struct connection_in * in ;
	in = Inbound_Control.head ;
	fprintf(out,
			"<BODY " BODYCOLOR "><TABLE " TOPTABLE
			"><TR><TD>OWFS on %s</TD><TD><A HREF='/'>Bus listing</A></TD><TD><A HREF='http://www.owfs.org'>OWFS homepage</A></TD><TD><A HREF='http://www.maxim-ic.com'>Dallas/Maxim</A></TD><TD>by <A HREF='mailto://palfille@earthlink.net'>Paul H Alfille</A></TD></TR></TABLE>\n",
			(in==NO_CONNECTION)?"no buses":SAFESTRING(SOC(in)->devicename));
	fprintf(out, "<H1>%s</H1><HR>\n", head);
}

void HTTPfoot(FILE * out)
{
	fprintf(out, "</BODY></HTML>");
}
