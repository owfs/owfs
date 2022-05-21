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

/* --------------- Functions ---------------- */
static void ShowDirText(struct OutputControl * oc, struct parsedname * pn);
static void ShowDirJson(struct OutputControl * oc, struct parsedname * pn);
/* Find he next higher level by search for last slash that doesn't end the string */
/* return length of "higher level" */
/* Assumes good path:
    null delimitted
    starts with /
    non-null
*/
int Backup(const char *path)
{
	int i = strlen(path) - 1;
	if (i == 0)
		return 1;
	if (path[i] == '/')
		--i;
	while (path[i] != '/')
		--i;
	return i + 1;
}

#define name_directory    "directory"
#define name_onewire_chip "1-wire chip"

	/* Callback function to FS_dir */
static void ShowDirCallback(void *v, const struct parsedname *pn_entry)
{
	/* uncached tag */
	/* device name */
	/* Have to allocate all buffers to make it work for Coldfire */
	struct OutputControl * oc = v;
	FILE * out = oc->out ;
	const char *nam;
	const char *typ;
	char * escaped_path = httpescape( pn_entry->path ) ;

	if (IsDir(pn_entry)) {
		nam = FS_DirName(pn_entry);
		typ = name_directory;
	} else {
		nam = pn_entry->selected_device->readable_name;
		typ = name_onewire_chip;
	}

	fprintf(out,
			"<TR><TD><A HREF='%s'><CODE><B><BIG>%s</BIG></B></CODE></A></TD><TD>%s</TD><TD>%s</TD></TR>", escaped_path==NULL ? pn_entry->path : escaped_path, FS_DirName(pn_entry), nam,
			typ);

	if ( escaped_path ) {
		owfree( escaped_path ) ;
	}
}

	/* Misnamed. Actually all directory */
void ShowDir(struct OutputControl * oc, struct parsedname * pn)
{
	FILE * out = oc->out ;
	int b = Backup(pn->path); // length of string to get to higher level
	
	if (pn->state & ePS_text) {
		ShowDirText(oc, pn);
		return;
	} else if (pn->state & ePS_json) {
		ShowDirJson(oc, pn);
		return;
	}

	HTTPstart(oc, "200 OK", ct_html);
	HTTPtitle(oc, "Directory");

	if (NotRealDir(pn)) {
		/* return whole path since tree structure could be much deeper now */
		/* first / is stripped off */
		HTTPheader(oc, &pn->path[1]);
	} else if (pn->state) {
		/* return whole path since tree structure could be much deeper now */
		/* first / is stripped off */
		HTTPheader(oc, &pn->path[1]);
	} else {
		HTTPheader(oc, "directory");
	}


	fprintf(out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>");

	if (b != 1) {
		char * escaped_path = httpescape( pn->path ) ;
		fprintf(out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>higher level</TD><TD>directory</TD></TR>", b, escaped_path==NULL ? pn->path : escaped_path );
		if ( escaped_path ) {
			owfree( escaped_path ) ;
		}
	} else {
		fprintf(out, "<TR><TD><A HREF='/'><CODE><B><BIG>top</BIG></B></CODE></A></TD><TD>highest level</TD><TD>directory</TD></TR>");
	}

	FS_dir(ShowDirCallback, oc, pn);
	fprintf(out, "</TABLE>");
	HTTPfoot(oc);
}

static void ShowDirTextCallback(void *v, const struct parsedname *const pn_entry)
{
	/* uncached tag */
	/* device name */
	/* Have to allocate all buffers to make it work for Coldfire */
	struct OutputControl * oc = v;
	FILE * out = oc->out ;
	const char *nam;
	const char *typ;
	if (IsDir(pn_entry)) {
		nam = FS_DirName(pn_entry);
		typ = name_directory;
	} else {
		nam = pn_entry->selected_device->readable_name;
		typ = name_onewire_chip;
	}
	fprintf(out, "%s %s \"%s\"\r\n", FS_DirName(pn_entry), nam, typ);
}

static void ShowDirText(struct OutputControl * oc, struct parsedname * pn)
{
	HTTPstart(oc, "200 OK", ct_text);

	FS_dir(ShowDirTextCallback, oc, pn);
	return;
}

void JSON_dir_init( struct OutputControl * oc )
{
	oc->not_first = 0 ;
}
	
void JSON_dir_entry( struct OutputControl * oc, const char * format, const char * data )
{
	FILE * out = oc->out ;
	
	// handle comma (so not after last entry)
	if ( oc->not_first ) {
		fprintf(out, ",\n" ) ;
	} else {
		oc->not_first = 1 ;
	}
	fprintf(out, format, data ) ;
}


void JSON_dir_finish( struct OutputControl * oc )
{
	FILE * out = oc->out ;
	
	if ( oc->not_first ) {
		fprintf(out, "\n" ) ;
	}
}	


static void ShowDirJsonCallback(void *v, const struct parsedname *const pn_entry)
{
	/* uncached tag */
	/* device name */
	/* Have to allocate all buffers to make it work for Coldfire */
	struct OutputControl * oc = v ;
	const char *nam;

	if (IsDir(pn_entry)) {
		nam = FS_DirName(pn_entry);
	} else {
		nam = pn_entry->selected_device->readable_name;
	}
	
	JSON_dir_entry( oc, "\"%s\":[]", nam ) ;
}

static void ShowDirJson(struct OutputControl * oc, struct parsedname * pn)
{
	FILE * out = oc->out ;
	
	JSON_dir_init( oc ) ;

	HTTPstart(oc, "200 OK", ct_json);

	fprintf(out, "{" );
	FS_dir(ShowDirJsonCallback, oc, pn);
	
	JSON_dir_finish( oc ) ;
	fprintf(out, "}" );
	return;
}
