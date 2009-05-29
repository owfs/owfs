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

/* ------------ Protoypes ---------------- */
struct urlparse {
	char line[PATH_MAX + 1];
	char *cmd;
	char *file;
	char *version;
	char *request;
	char *value;
};

	/* Error page functions */
static void Bad400(FILE * out);
static void Bad404(FILE * out);

/* URL parsing function */
static void URLparse(struct urlparse *up);


/* --------------- Functions ---------------- */

/* Main handler for a web page */
int handle_socket(FILE * out)
{
	char linecopy[PATH_MAX + 1];
	char *str;
	struct urlparse up;
	struct parsedname s_pn;
	struct parsedname * pn = &s_pn ;

	str = fgets(up.line, PATH_MAX, out);
	LEVEL_CALL("PreParse line=%s\n", up.line);
	URLparse(&up);				/* Braek up URL */

	/* read lines until blank */
	if (up.version) {
		do {
			str = fgets(linecopy, PATH_MAX, out);
		} while (str != NULL && strcmp(linecopy, "\r\n")
				 && strcmp(linecopy, "\n"));
	}

	LEVEL_CALL
		("WLcmd: %s\tfile: %s\trequest: %s\tvalue: %s\tversion: %s \n",
		 SAFESTRING(up.cmd), SAFESTRING(up.file), SAFESTRING(up.request), SAFESTRING(up.value), SAFESTRING(up.version));
	/*
	 * This is necessary for some stupid *
	 * * operating system such as SunOS
	 */
	fflush(out);

	/* Good command line? */
	if (up.cmd == NULL || strcmp(up.cmd, "GET") != 0) {
		Bad400(out);

		/* Can't understand the file name = URL */
	} else if (up.file == NULL) {
		Bad404(out);
	} else if (strcasecmp(up.file, "/favicon.ico") == 0) {
		Favicon(out);
	} else if (FS_ParsedName(up.file, pn)) {
		/* Can't understand the file name = URL */
		Bad404(out);
	} else {
		/* Root directory -- show the bus */
		if (pn->selected_device == NULL) {	/* directory! */
			ShowDir(out, pn);
		} else if (up.request == NULL) {
			ShowDevice(out, pn);
		} else {				/* First write new values, then show */
			OWQ_allocate_struct_and_pointer(owq_write);

			if (FS_OWQ_create_plus(up.file, up.request, up.value, strlen(up.value), 0, owq_write)) {
				Bad404(out);
			} else {
				/* Single device, show it's properties */
				ChangeData(owq_write);
				FS_OWQ_destroy(owq_write);
				ShowDevice(out, pn);
			}
		}
		FS_ParsedName_destroy(pn);
	}
//printf("Done\n");
	return 0;
}

/* URL handler for a web page */
static void URLparse(struct urlparse *up)
{
	char *str;
	int first = 1;

	up->cmd = up->version = up->file = up->request = up->value = NULL;

	/* Separate out the three parameters, all in same line */
	for (str = up->line; *str; str++) {
		switch (*str) {
		case ' ':
		case '\n':
		case '\r':
			*str = '\0';
			first = 1;
			break;
		default:
			if (!first || up->version) {
				break;
			} else if (up->file) {
				up->version = str;
			} else if (up->cmd) {
				up->file = str;
			} else {
				up->cmd = str;
			}
			first = 0;
		}
	}

	/* Separate out the filename and FORM data */
	if (up->file) {
		for (str = up->file; *str; str++) {
			if (*str == '?') {
				*str = '\0';
				up->request = str + 1;
				break;
			}
		}
		/* trim off trailing '/' */
		--str;
		if (*str == '/' && str > up->file)
			*str = '\0';
	}

	/* Separate out the FORM field and value */
	if (up->request) {
		for (str = up->request; *str; str++) {
			if (*str == '=') {
				*str = '\0';
				up->value = str + 1;
				break;
			}
		}
	}

	/* Remove garbage from oned of value */
	if (up->value) {
		for (str = up->value; *str; str++) {
			if (*str == '&') {
				*str = '\0';
				break;
			}
		}
		/* Special case -- checkbox off, CHANGE is value read */
		if (strcmp("CHANGE", up->value) == 0) {
			up->value[0] = '0';
			up->value[1] = '\0';
		}
	}
	LEVEL_DEBUG("URL parse file=%s, request=%s, value=%s\n", SAFESTRING(up->file), SAFESTRING(up->request), SAFESTRING(up->value));
}


static void Bad400(FILE * out)
{
	HTTPstart(out, "400 Bad Request", ct_html);
	HTTPtitle(out, "Error 400 -- Bad request");
	HTTPheader(out, "Unrecognized Request");
	fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested web page is not recognized.</P>");
	fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
	HTTPfoot(out);
}

static void Bad404(FILE * out)
{
	HTTPstart(out, "404 File not found", ct_html);
	HTTPtitle(out, "Error 400 -- Item doesn't exist");
	HTTPheader(out, "Non-existent Device");
	fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested device is not recognized.</P>");
	fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
	HTTPfoot(out);
}
