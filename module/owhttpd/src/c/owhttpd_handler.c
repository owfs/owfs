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
#define _GNU_SOURCE
#include <stdio.h> // for getline
#undef _GNU_SOURCE

#include "owhttpd.h"

// #include <libgen.h>  /* for dirname() */

/* ------------ Protoypes ---------------- */
struct urlparse {
	char *line;
	size_t line_length ;
	char *cmd;
	char *file;
	char *version;
	char *request;
	char *value;
};

enum http_return { http_ok, http_dir, http_icon, http_400, http_404 } ;

	/* Error page functions */
static void Bad400(FILE * out);
static void Bad404(FILE * out);

/* URL parsing function */
static void URLparse(struct urlparse *up);
static enum http_return handle_GET(FILE * out, struct urlparse * up) ;
static enum http_return handle_POST(FILE * out, struct urlparse * up) ;
static int ReadToCRLF( FILE * out ) ;
static void TrimBoundary( char ** boundary ) ;
static int GetPostData( char * boundary, struct memblob * mb, FILE * out ) ;
static char * GetPostPath( FILE * out ) ;

/* --------------- Functions ---------------- */

/* Main handler for a web page */
int handle_socket(FILE * out)
{
	enum http_return http_code ;

	struct urlparse up;

	struct parsedname s_pn;
	struct parsedname * pn = &s_pn ;
	
	up.line = NULL ; // prep for getline with null. Will be allocated by getline.
	if ( getline(&(up.line), &(up.line_length), out) >= 0 ) {
		LEVEL_CALL("PreParse line=%s", up.line);
		URLparse(&up);				/* Break up URL */
		httpunescape((BYTE *) up.file    );
		httpunescape((BYTE *) up.request );
		httpunescape((BYTE *) up.value   );

		LEVEL_CALL
		("WLcmd: %s\tfile: %s\trequest: %s\tvalue: %s\tversion: %s",
		SAFESTRING(up.cmd), SAFESTRING(up.file), SAFESTRING(up.request), SAFESTRING(up.value), SAFESTRING(up.version));

		if (up.cmd == NULL) {
			// No command line in request
			pn = NULL ;
			http_code = http_400 ;
		} else if (up.file == NULL) {
			// could not parse a path from the request
			pn = NULL ;
			http_code = http_404 ;
		} else if (strcasecmp(up.file, "/favicon.ico") == 0) {
			// secial case for the icon
			LEVEL_DEBUG("http icon request.");
			ReadToCRLF(out) ;
			pn = NULL ;
			http_code = http_icon ;
		} else 	if (FS_ParsedName(up.file, pn)) {
			// Can't understand the file name = URL
			LEVEL_DEBUG("http %s not understood.",up.file);
			ReadToCRLF(out) ;
			pn = NULL ;
			http_code = http_404 ;
		} else if (pn->selected_device == NULL) {
			// directory!
			LEVEL_DEBUG("http directory request.");
			ReadToCRLF(out) ;
			http_code = http_dir ;
		} else if (strcmp(up.cmd, "POST") == 0) {
			LEVEL_DEBUG("http POST request.");
			http_code = handle_POST( out, &up ) ;
		} else if (strcmp(up.cmd, "GET") == 0) {
			LEVEL_DEBUG("http GET request.");
			http_code = handle_GET( out, &up ) ;
		} else {
			ReadToCRLF(out) ;
			http_code = http_400 ;
		}
		free(up.line) ;
	} else {
		LEVEL_DEBUG("No http data.");
		pn = NULL ;
		http_code = http_400 ;
	}

	/*
	* This is necessary for some stupid *
	* * operating system such as SunOS
	*/
	fflush(out);
	switch ( http_code ) {
		case http_icon:
			Favicon(out);
			break ;
		case http_400:
			Bad400(out);
			break ;
		case http_404:
			Bad404(out);
			break ;
		case http_dir:
			ShowDir(out, pn);
			break ;
		case http_ok:
			ShowDevice(out, pn);
			break ;
	}
	if ( pn ) {
		FS_ParsedName_destroy(pn);
	}
	
	return 0 ;
}	

/* The HTTP request is a GET message */
static enum http_return handle_GET(FILE * out, struct urlparse * up)
{
	/* read lines until blank */
	if (up->version) {
		ReadToCRLF( out ) ;
	}
	
	if (up->request == NULL) {
		// NO request -- just a read or dir, not a write
		LEVEL_DEBUG("Simple GET request -- read a value or directory");
		return http_ok ;
	} else if (up->value==NULL) {
		// write without a value
		LEVEL_DEBUG("Null value for write command -- Bad URL.");
		return http_400 ;
	} else {				/* First write new values, then show */
		OWQ_allocate_struct_and_pointer(owq_write);
		size_t req_leng = strlen(up->request) ;
		size_t fil_leng = strlen(up->file) ;

		// see if the URL includes the property twice -- happens if only a property is shown
		if ( req_leng <= fil_leng && strcasecmp(up->request,&up->file[fil_leng-req_leng])==0 ) {
			struct parsedname s_pn ;
			if ( FS_ParsedName(up->file,&s_pn)==0 ) {
				if ( s_pn.selected_filetype != NULL ) {
					LEVEL_DEBUG("Property name %s duplicated on command line. Not a problem.",up->request) ;
					up->file[fil_leng-req_leng-1] = '\0';
				}
				FS_ParsedName_destroy(&s_pn);
			}
		}
		
		// Create the owq to write to.
		if ( BAD( OWQ_create_plus(up->file, up->request, owq_write) ) ) { // for write
			return http_404 ;
		}
		OWQ_assign_write_buffer( up->value, strlen(up->value), 0, owq_write ) ;
		
		/* Execute the write */
		ChangeData(owq_write);
		
		OWQ_destroy(owq_write);
		return http_ok ;
	}
}

/* The HTTP request is a POST message */
static enum http_return handle_POST(FILE * out, struct urlparse * up)
{
	enum http_return http_code ;

	char * boundary = NULL ;
	size_t boundary_length ;
	
	/* read lines until blank */
	if (up->version) {
		ReadToCRLF( out ) ;
	}
	
	// use getline because it handles null chars
	if ( getline(&boundary,&boundary_length,out) > 2 ) {
		char * post_path  = GetPostPath( out ) ;

		TrimBoundary( &boundary) ;
		LEVEL_CALL("POST boundary=%s",boundary);

		if ( post_path ) {
			struct memblob mb ;
			if ( GetPostData( boundary, &mb, out ) == 0 ) {
				struct one_wire_query * owq = OWQ_create_from_path( post_path ) ; // for write
				if ( owq ) {
					LEVEL_DEBUG("File upload %s for %ld bytes",post_path,MemblobLength(&mb));
					if ( OWQ_allocate_write_buffer( (char *) MemblobData(&mb), MemblobLength(&mb), 0, owq) == 0 ) {
						PostData(owq);
						http_code = http_ok ;
					} else {
						http_code = http_404 ;
					}
					OWQ_destroy(owq) ;
				} else {
					LEVEL_DEBUG("Can't create %s",post_path);
					http_code = http_404 ;
				}
			} else {
				LEVEL_DEBUG("Can't read full binary data from file upload");
				http_code = http_404 ;
			}
			MemblobClear( &mb ) ;
			owfree(post_path ) ;
		} else {
			LEVEL_DEBUG("Can't read property name from file upload");
			http_code = http_404 ;
		}
	}
	if ( boundary ) {
		free(boundary) ; // allocated in getline with malloc, not owmalloc
	}

	return http_code ;
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
	LEVEL_DEBUG("URL parse file=%s, request=%s, value=%s", SAFESTRING(up->file), SAFESTRING(up->request), SAFESTRING(up->value));
}


static void Bad400(FILE * out)
{
	LEVEL_CALL("Return a 400 HTTP error code");
	HTTPstart(out, "400 Bad Request", ct_html);
	HTTPtitle(out, "Error 400 -- Bad request");
	HTTPheader(out, "Unrecognized Request");
	fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested web page is not recognized.</P>");
	fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
	HTTPfoot(out);
}

static void Bad404(FILE * out)
{
	LEVEL_CALL("Return a 404 HTTP error code");
	HTTPstart(out, "404 Not Found", ct_html);
	HTTPtitle(out, "Error 400 -- Item doesn't exist");
	HTTPheader(out, "Non-existent Device");
	fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested device is not recognized.</P>");
	fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
	HTTPfoot(out);
}

static int ReadToCRLF( FILE * out )
{
	char * text_in = NULL ;
	size_t length_in = 0 ;

	/* read lines until blank */
	while (getline(&text_in, &length_in, out)>0)  {
		LEVEL_DEBUG("More data:%s",text_in);
		if ( strcmp(text_in, "\r\n")==0 || strcmp(text_in, "\n")==0 ) {
			if ( text_in ) {
				free( text_in) ;
			}
			return 0 ;
		}
	}
	if ( text_in ) {
		free( text_in) ;
	}
	return 1 ;
}

static void TrimBoundary( char ** boundary )
{
	char * remove_char ;
	
	remove_char = strrchr( *boundary, '\n' ) ;
	if ( remove_char ) {
		*remove_char = '\0' ;
	}
	
	remove_char = strrchr( *boundary, '\r' ) ;
	if ( remove_char ) {
		*remove_char = '\0' ;
	}
}

static char * GetPostPath( FILE * out )
{
	char * text_in = NULL ;
	size_t length_in = 0 ;
	char * path_found = NULL ;
	
	/* read lines until blank */
	while (getline(&text_in, &length_in, out)>-1)  {
		char * namestart ;
		LEVEL_DEBUG("Post data:%s",SAFESTRING(text_in));
		if ( strcmp(text_in, "\r\n")==0 || strcmp(text_in, "\n")==0 ) {
			if ( text_in ) {
				free( text_in) ;
			}
			return path_found ;
		}

		namestart = strstr(text_in," name=\"") ;
		if (namestart != NULL ) {
			char * nameend ;
			namestart += 7 ;
			nameend = strchr(namestart,'\"') ;
			if ( nameend != NULL ) {
				nameend[0] = '\0' ;
				if( path_found ) {
					owfree(path_found);
				}
				path_found = owstrdup( namestart ) ;
			}
		}
	}
	if ( text_in ) {
		free( text_in) ;
	}
	return path_found ;
}

// read data from file upload
static int GetPostData( char * boundary, struct memblob * mb, FILE * out )
{
	char * data = NULL ;
	size_t data_length ;

	ssize_t read_this_pass ;

	MemblobInit( mb, 1000 ) ; // increqment in 1K amounts (arbitrary)
	while ( (read_this_pass = getline(&data, &data_length, out)) > -1 ) {
		Debug_Bytes(boundary,(BYTE *)data,(size_t)read_this_pass);
		if ( strstr( data, boundary ) != NULL ) {
			free(data) ; // allocated by getline with malloc, not owmalloc
			MemblobTrim(2,mb) ; // trim off final 0x0A 0x0D
			LEVEL_DEBUG("Read in POST file upload of %ld bytes",MemblobLength(mb));
			return 0 ;
		}
		if ( MemblobAdd( (BYTE *)data, (size_t)read_this_pass, mb ) ) {
			LEVEL_DEBUG("Data size too large");
			free(data) ; // allocated by getline with malloc, not owmalloc
			return 1 ;
		}
	}
	if ( data ) {
		free(data) ; // allocated by getline with malloc, not owmalloc
	}
	LEVEL_DEBUG("HTTP error -- no ending MIME boundary");
	return 1 ;
}
