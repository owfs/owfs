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
enum content_type PoorMansParser( char * bad_url ) ;
static void Bad400( struct OutputControl * oc, const enum content_type ct);
static void Bad404( struct OutputControl * oc, const enum content_type ct);

/* URL parsing function */
static void URLparse(struct urlparse *up);
static enum http_return handle_GET( struct OutputControl * oc, struct urlparse * up) ;
static enum http_return handle_POST( struct OutputControl * oc, struct urlparse * up) ;
static void ReadToCRLF( struct OutputControl * oc ) ;
static void TrimBoundary( char ** boundary ) ;
static int GetPostData( char * boundary, struct memblob * mb, struct OutputControl * oct ) ;
static char * GetPostPath(  struct OutputControl * oc ) ;
static GOOD_OR_BAD GetHostURL( struct OutputControl * oc ) ;

/* --------------- Functions ---------------- */

/* Main handler for a web page */
int handle_socket(FILE * out)
{
	enum http_return http_code ;
	enum content_type pmp = ct_html;

	struct urlparse up;
	
	struct OutputControl s_oc = { out, 0, NULL, NULL, } ;
	struct OutputControl * oc = &s_oc ;

	struct parsedname s_pn;
	struct parsedname * pn = &s_pn ;
	
	up.line = NULL ; // prep for getline with null. Will be allocated by getline.
	if ( getline(&(up.line), &(up.line_length), out) >= 0 ) {
		LEVEL_CALL("PreParse line=%s", up.line);
		URLparse(&up);				/* Break up URL */
		httpunescape((BYTE *) up.file    );
		httpunescape((BYTE *) up.request );
		httpunescape((BYTE *) up.value   );

		LEVEL_CALL(
		"WLcmd: %s\tfile: %s\trequest: %s\tvalue: %s\tversion: %s",
		SAFESTRING(up.cmd), SAFESTRING(up.file), SAFESTRING(up.request), SAFESTRING(up.value), SAFESTRING(up.version)
		);

		oc->base_url = owstrdup( up.file==NULL ? "" : up.file ) ;

		if ( BAD( GetHostURL(oc) ) ) {
			// No command line in request
			pn = NO_PARSEDNAME ;
			http_code = http_400 ;
		} else if (up.cmd == NULL) {
			// No command line in request
			pn = NO_PARSEDNAME ;
			http_code = http_400 ;
		} else if (up.file == NULL) {
			// could not parse a path from the request
			pn = NO_PARSEDNAME ;
			http_code = http_404 ;
		} else if (strcasecmp(up.file, "/favicon.ico") == 0) {
			// special case for the icon
			LEVEL_DEBUG("http icon request.");
			ReadToCRLF(oc) ;
			pn = NO_PARSEDNAME ;
			http_code = http_icon ;
		} else 	if (FS_ParsedName(up.file, pn) != 0) {
			// Can't understand the file name = URL
			LEVEL_DEBUG("http %s not understood.",up.file);
			ReadToCRLF(oc) ;
			pn = NO_PARSEDNAME ;
			http_code = http_404 ;
		} else if (pn->selected_device == NO_DEVICE) {
			// directory!
			LEVEL_DEBUG("http directory request.");
			ReadToCRLF(oc) ;
			http_code = http_dir ;
		} else if (strcmp(up.cmd, "POST") == 0) {
			LEVEL_DEBUG("http POST request.");
			http_code = handle_POST( oc, &up ) ;
		} else if (strcmp(up.cmd, "GET") == 0) {
			LEVEL_DEBUG("http GET request.");
			http_code = handle_GET( oc, &up ) ;
			// special case for possible alias changes
			// Parsedname structure may point to a device that no longer exists
			if ( http_code == http_ok ) { // was able to write
				FS_ParsedName_destroy(pn);
				if (FS_ParsedName(up.file, pn)) {
					// Can't understand the device name any more
					LEVEL_DEBUG("Alias change for %s",up.file);
					if (FS_ParsedName("/", pn)) {
						// Try to set to root
						LEVEL_DEBUG("Set to root");
						pn = NO_PARSEDNAME ;
						http_code = http_404 ;
					}
				}
			}
		} else {
			ReadToCRLF(oc) ;
			http_code = http_400 ;
		}
		switch ( http_code ) {
			case http_400:
			case http_404:
				// need to call this before freeing up.file
				pmp = PoorMansParser(up.file) ;
				break ;
			default:
				// not an error
				break ;
		}
		// allocated by getline
		free(up.line) ;
	} else {
		LEVEL_DEBUG("No http data.");
		pn = NO_PARSEDNAME ;
		http_code = http_400 ;
	}

	// This is necessary for SunOS
	fflush(out);
	
	switch ( http_code ) {
		case http_icon:
			Favicon(oc);
			break ;
		case http_400:
			Bad400(oc,pmp);
			break ;
		case http_404:
			Bad404(oc,pmp);
			break ;
		case http_dir:
			ShowDir(oc, pn);
			break ;
		case http_ok:
			ShowDevice(oc, pn);
			break ;
	}
	if ( pn != NO_PARSEDNAME ) {
		FS_ParsedName_destroy(pn);
	}
	
	if ( oc->base_url != NULL ) {
		owfree( oc->base_url ) ;
	}
	if ( oc->host != NULL ) {
		owfree( oc->host ) ;
	}
	
	return 0 ;
}	

/* The HTTP request is a GET message */
static enum http_return handle_GET( struct OutputControl * oc, struct urlparse * up)
{
	/* read lines until blank */
	if (up->version) {
		ReadToCRLF( oc ) ;
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
		char * remove_filetype = NULL ; // remove the filetype from the file name

		// see if the URL includes the property twice -- happens if only a property is shown
		if ( req_leng <= fil_leng && strcasecmp(up->request,&up->file[fil_leng-req_leng])==0 ) {
			struct parsedname s_pn ;
			if ( FS_ParsedName(up->file,&s_pn)==0 ) {
				if ( s_pn.selected_filetype != NO_FILETYPE ) {
					LEVEL_DEBUG("Property name %s duplicated on command line. Not a problem.",up->request) ;
					remove_filetype = &up->file[fil_leng-req_leng-1] ;
					remove_filetype[0] = '\0';
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

		if ( remove_filetype != NULL ) {
			// restore full URL path
			remove_filetype[0] = '/' ;
		}
		return http_ok ;
	}
}

/* The HTTP request is a POST message */
static enum http_return handle_POST( struct OutputControl * oc, struct urlparse * up)
{
	FILE* out = oc->out ;
	enum http_return http_code = http_404 ; // default error mode

	char * boundary = NULL ;
	size_t boundary_length ;
	
	/* read lines until blank */
	if (up->version) {
		ReadToCRLF( oc ) ;
	}
	
	// use getline because it handles null chars
	if ( getline(&boundary,&boundary_length,out) > 2 ) {
		char * post_path  = GetPostPath( oc ) ;

		TrimBoundary( &boundary) ;
		LEVEL_CALL("POST boundary=%s",boundary);

		if ( post_path ) {
			struct memblob mb ;
			if ( GetPostData( boundary, &mb, oc ) == 0 ) {
				struct one_wire_query * owq = OWQ_create_from_path( post_path ) ; // for write
				if ( owq ) {
					LEVEL_DEBUG("File upload %s for %ld bytes",post_path,MemblobLength(&mb));
					if ( GOOD( OWQ_allocate_write_buffer( (char *) MemblobData(&mb), MemblobLength(&mb), 0, owq)) ) {
						PostData(owq);
						http_code = http_ok ;
					}
					OWQ_destroy(owq) ;
				} else {
					LEVEL_DEBUG("Can't create %s",post_path);
				}
			} else {
				LEVEL_DEBUG("Can't read full binary data from file upload");
			}
			MemblobClear( &mb ) ;
			owfree(post_path ) ;
		} else {
			LEVEL_DEBUG("Can't read property name from file upload");
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
	char * extension ;

	up->cmd = up->version = up->file = up->request = up->value = NULL;
	
	/* Special case for sparse array
	 * Substitute "/" for "?EXTENSION=" 
	 * */
	extension = strchr( up->line, '?' ) ;
	if ( extension != NULL ) {
		if ( strncmp( "?EXTENSION=" , extension, 11 ) == 0 ) {
			int l = strlen( extension ) ;
			extension[0] = '/' ;
			memmove(  &extension[1], &extension[11], l-10 ) ;
		}
	}
	 
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


static void Bad400(struct OutputControl * oc, const enum content_type ct)
{
	FILE * out = oc->out ;
	
	LEVEL_CALL("Return a 400 HTTP error code");
	switch( ct ) {
		case ct_text:
			HTTPstart(oc, "400 Bad Request", ct_text);
			fprintf(out, "400 Bad request");
			break ;
		case ct_json:
			HTTPstart(oc, "400 Bad Request", ct_json);
			fprintf(out, "{}");
			break ;
		default:
			HTTPstart(oc, "400 Bad Request", ct_html);
			HTTPtitle(oc, "Error 400 -- Bad request");
			HTTPheader(oc, "Unrecognized Request");
			fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested web page is not recognized.</P>");
			fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
			HTTPfoot(oc);
	}
}

static void Bad404(struct OutputControl * oc, const enum content_type ct)
{
	FILE * out = oc->out ;
	LEVEL_CALL("Return a 404 HTTP error code");
	switch( ct ) {
		case ct_text:
			HTTPstart(oc, "404 Not Found", ct_text);
			fprintf(out, "404 Not Found");
			break ;
		case ct_json:
			HTTPstart(oc, "404 Not Found", ct_json);
			fprintf(out, "{}");
			break ;
		default:
			HTTPstart(oc, "404 Not Found", ct_html);
			HTTPtitle(oc, "Error 404 -- Item doesn't exist");
			HTTPheader(oc, "Nonexistent Device");
			fprintf(out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested device is not recognized.</P>");
			fprintf(out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>");
			HTTPfoot(oc);
}	}


static void ReadToCRLF( struct OutputControl * oc )
{
	FILE * out = oc->out ;
	char * text_in = NULL ;
	size_t length_in = 0 ;
	ssize_t getline_length ;

	/* read lines until blank */
	while ( (getline_length = getline(&text_in, &length_in, out)) > 0 )  {
		LEVEL_DEBUG("More (%d) data:%s",(int)getline_length,text_in);
		if ( strcmp(text_in, "\r\n")==0 || strcmp(text_in, "\n")==0 ) {
			break ;
		}
	}
	
	
	if ( text_in != NULL ) {
		free( text_in) ;
	}
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

static char * GetPostPath(struct OutputControl * oc )
{
	FILE * out = oc->out ;
	char * text_in = NULL ;
	size_t length_in = 0 ;
	char * path_found = NO_PATH ;
	
	/* read lines until blank */
	while (getline(&text_in, &length_in, out)>-1)  {
		char * namestart ;
		LEVEL_DEBUG("Post data:%s",SAFESTRING(text_in));
		if ( strcmp(text_in, "\r\n")==0 || strcmp(text_in, "\n")==0 ) {
			free( text_in) ;
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
static int GetPostData( char * boundary, struct memblob * mb, struct OutputControl * oc )
{
	FILE * out = oc->out ;
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

// Parse the line for just the text or json key since there was an error and Parsedname is null
enum content_type PoorMansParser( char * bad_url )
{
	if ( bad_url == NULL ) {
		LEVEL_DEBUG("Error on http request assume html");
		return ct_html ;
	} else if ( strstr( bad_url, "json" ) != NULL ) {
		LEVEL_DEBUG("Error on http request <%s> assume json",bad_url);
		return ct_json ;
	} else if ( strstr( bad_url, "JSON" ) != NULL ) {
		LEVEL_DEBUG("Error on http request <%s> assume json",bad_url);
		return ct_json ;
	} else if ( strstr( bad_url, "text" ) != NULL ) {
		LEVEL_DEBUG("Error on http request <%s> assume text",bad_url);
		return ct_text ;
	} else if ( strstr( bad_url, "TEXT" ) != NULL ) {
		LEVEL_DEBUG("Error on http request <%s> assume text",bad_url);
		return ct_text ;
	}
	LEVEL_DEBUG("Error on http request <%s> assume html",bad_url);
	return ct_html ;
}
			
static GOOD_OR_BAD GetHostURL( struct OutputControl * oc )
{
	FILE * out = oc->out ;
	char * line = NULL ;
	char * pline ;
	do {
		size_t s ;
		if ( getline( &line, &s, out ) < 0 ) {
			free( line ) ;
			LEVEL_DEBUG("Couldn't find Host: line in HTTP header") ;
			return gbBAD ;
		}
		if ( s < 5 ) {
			continue ; // too short for Host:
		}
		for ( pline = line ; *pline != '\0' ; ++pline ) {
			if ( pline[0] != ' ' ) {
				if ( strncasecmp( pline, "host", 4 ) == 0 ) {
					// Found host
					strsep( &pline, ":" ) ; // look for ':'
					if ( pline == NULL ) {
						LEVEL_DEBUG("No : in Host HTTP line") ;
						free(line) ;
						return gbBAD ;
					}
					for ( ; *pline==' ' ; ++pline ) {
						continue ;
					}
					oc->host = owstrdup(strsep( &pline, " \r\n" )) ;
					LEVEL_DEBUG("Found host <%s>",oc->host) ;
					free(line) ;
					return gbGOOD ;				
				} else {
					continue ;
				}
			}
		}
	} while (1) ;
}


				
	
