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

#define SVERSION "owhttpd"

#include "owhttpd.h"

/* ------------ Protoypes ---------------- */

	/* Utility HTML page display functions */
void HTTPstart(struct OutputControl * oc, const char *status, const enum content_type ct)
{
	FILE * out = oc->out ;
	char d[44];
	time_t t = NOW_TIME;
	size_t l = strftime(d, sizeof(d), "%a, %d %b %Y %T GMT", gmtime(&t));

	fprintf(out, "HTTP/1.0 %s\r\n", status);
	fprintf(out, "Date: %*s\r\n", (int) l, d);
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
	case ct_json:
		fprintf(out, "Access-Control-Allow-Origin: *\r\n");
		fprintf(out, "Content-Type: application/json\r\n");
		break ;
	}
	fprintf(out, "\r\n");
}

void HTTPtitle(struct OutputControl * oc, const char *title)
{
	FILE * out = oc->out ;
	
	fprintf(out, "<HTML><HEAD><TITLE>1-Wire Web: %s</TITLE></HEAD>\n", title);
}

void HTTPheader(struct OutputControl * oc, const char *head)
{
	FILE * out = oc->out ;
	
	fprintf(out,
			"<BODY " BODYCOLOR "><TABLE " TOPTABLE
			"><TR><TD>OWFS</TD><TD><A HREF='/'>Bus listing</A></TD><TD><A HREF='http://www.owfs.org'>OWFS homepage</A></TD><TD><A HREF='http://www.maxim-ic.com'>Dallas/Maxim</A></TD><TD>by <A HREF='mailto://paul.alfille@gmail.com'>Paul H Alfille</A></TD></TR></TABLE>\n");
	fprintf(out, "<H1>%s</H1><HR>\n", head);
}

void HTTPfoot(struct OutputControl * oc)
{
	FILE * out = oc->out;
	
	fprintf(out, "</BODY></HTML>");
}
