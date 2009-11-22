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
static int hex_convert(char *str);
static int httpunescape(BYTE * httpstr);
static void hex_only(char *str);

/* --------------- Functions ---------------- */


// POST data -- a file upload
void PostData(struct one_wire_query *owq)
{
	/* Do command processing and make changes to 1-wire devices */
	LEVEL_DETAIL("Uploaded Data path=%s size=%s\n", PN(owq)->path, OWQ_size(owq));
	FS_write_postparse(owq);
}

// Standard Data via GET
void ChangeData(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	ASCII *value_string = OWQ_buffer(owq);
	
	/* Do command processing and make changes to 1-wire devices */
	httpunescape((BYTE *) value_string);
	LEVEL_DETAIL("New data path=%s value=%s\n", pn->path, value_string);
	switch (pn->selected_filetype->format) {
		case ft_binary:
			hex_only(value_string);
			OWQ_size(owq) = hex_convert(value_string);
			break;
		default:
			OWQ_size(owq) = strlen(value_string);
			break;
	}
	FS_write_postparse(owq);
}

/* Change web-escaped string back to straight ascii */
static int httpunescape(BYTE * httpstr)
{
	BYTE *in = httpstr;			/* input string pointer */
	BYTE *out = httpstr;		/* output string pointer */

	while (*in) {
		switch (*in) {
		case '%':
			++in;
			if (in[0] == '\0' || in[1] == '\0') {
				*out = '\0';
				return 1;
			}
			*out++ = string2num((char *) in);
			++in;
			break;
		case '+':
			*out++ = ' ';
			break;
		default:
			*out++ = *in;
			break;
		}
		in++;
	}
	*out = '\0';
	return 0;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_only(char *str)
{
	char *uc = str;
	char *hx = str;
	while (*uc) {
		if (isxdigit(*uc))
			*hx++ = *uc;
		uc++;
	}
	*hx = '\0';
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
/* returns length */
static int hex_convert(char *str)
{
	char *uc = str;
	BYTE *hx = (BYTE *) str;
	int return_length = 0;
	for (; *uc; uc += 2) {
		*hx++ = string2num(uc);
		++return_length;
	}
	*hx = '\0';
	return return_length;
}
