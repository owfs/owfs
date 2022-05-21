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

// #include <libgen.h>  /* for dirname() */
	/* string format functions */
static int hex_convert(char *str);

static void hex_only(char *str);

/* --------------- Functions ---------------- */


// POST data -- a file upload
void PostData(struct one_wire_query *owq)
{
	/* Do command processing and make changes to 1-wire devices */
	LEVEL_DETAIL("Uploaded Data path=%s size=%ld", PN(owq)->path, OWQ_size(owq));
	FS_write_postparse(owq);
}

// Standard Data via GET
void ChangeData(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	ASCII *value_string = OWQ_buffer(owq);
	
	/* Do command processing and make changes to 1-wire devices */
	LEVEL_DETAIL("New data path=%s value=%s", pn->path, value_string);
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

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_only(char *str)
{
	char *leader = str;
	char *trailer = str;

	while (*leader) {
		if (isxdigit(*leader)) {
			*trailer++ = *leader;
		}
		leader++;
	}
	*trailer = '\0';
}

/* reads an as ascii hex string, converts in place */
/* returns length */
static int hex_convert(char *str)
{
	char *leader = str;
	BYTE *trailer = (BYTE *) str;

	while (*leader) {
		*trailer++ = string2num(leader);
		leader += 2 ;
	}

	return trailer - ((BYTE *) str);
}
