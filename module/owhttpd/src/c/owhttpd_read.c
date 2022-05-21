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

/* --------------- Prototypes---------------- */
static void Show(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowDirectory(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowReadWrite(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowReadonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowWriteonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowStructure(struct OutputControl * oc, struct one_wire_query *owq);
static void StructureDetail(struct OutputControl * oc, const char * structure_details );
static void Upload( struct OutputControl * oc, const struct parsedname * pn ) ;
static void Extension( struct OutputControl * oc, const struct parsedname * pn ) ;

static void ShowText(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowTextDirectory(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowTextReadWrite(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowTextReadonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowTextWriteonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowTextStructure(struct OutputControl * oc, struct one_wire_query *owq);

static void ShowJson(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowJsonDirectory(struct OutputControl * oc, const struct parsedname *pn_entry);
static void ShowJsonReadWrite(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowJsonReadonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowJsonWriteonly(struct OutputControl * oc, struct one_wire_query *owq);
static void ShowJsonStructure(struct OutputControl * oc, struct one_wire_query *owq);
static void StructureDetailJson(struct OutputControl * oc, const char * structure_details );

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	FILE * out = oc->out ;
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir
	struct filetype * ft = pn_entry->selected_filetype ;
	/* Left column */
	fprintf(out, "<TR><TD><B>%s</B></TD><TD>", FS_DirName(pn_entry));

	if (owq == NO_ONE_WIRE_QUERY) {
		fprintf(out, "<B>Memory exhausted</B>");
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		fprintf(out, "<B>Memory exhausted</B>");
	} else if (ft == NO_FILETYPE) {
		ShowDirectory(oc, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowStructure(oc, owq);
	} else if (ft->format == ft_directory || ft->format == ft_subdir) {
		// Directory
		ShowDirectory(oc, pn_entry);
	} else if ( pn_entry->extension == EXTENSION_UNKNOWN  && ft->ag != NON_AGGREGATE && ft->ag->combined == ag_sparse ) {
		Extension(oc, pn_entry ) ; // flag as generic needing an extension chosen
	} else if (ft->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (ft->read != NO_READ_FUNCTION) {
			ShowReadonly(oc, owq);
		} else {
			// Property has no read or write ability
		}
	} else {					// Writeable
		if (ft->read == NO_READ_FUNCTION) {
			ShowWriteonly(oc, owq);
		} else {
			ShowReadWrite(oc, owq);
		}
	}
	fprintf(out, "</TD></TR>\r\n");
	OWQ_destroy(owq);
}


/* Device entry -- table line for a filetype */
static void ShowReadWrite(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	struct parsedname * pn = PN(owq) ;
	const char *file = FS_DirName(pn);
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	switch (pn->selected_filetype->format) {
		case ft_binary:
			{
				int i = 0;
				fprintf(out, "<CODE><FORM METHOD='GET' ACTION='http://%s%s'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'>", oc->host, oc->base_url, file, read_return >> 5);
				while (i < read_return) {
					fprintf(out, "%.2hhX", OWQ_buffer(owq)[i]);
					if (((++i) < read_return) && (i & 0x1F) == 0) {
						fprintf(out, "\r\n");
					}
				}
				fprintf(out, "</TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>");
				Upload( oc, pn ) ;
				break;
			}
		case ft_yesno:
		case ft_bitfield:
			if (pn->extension >= 0) {
				fprintf(out,
						"<FORM METHOD='GET' ACTION='http://%s%s'><INPUT TYPE='CHECKBOX' NAME='%s' %s><INPUT TYPE='SUBMIT' VALUE='CHANGE' NAME='%s'></FORM>",oc->host, oc->base_url,
						file, (OWQ_buffer(owq)[0] == '0') ? "" : "CHECKED", file);
				break;
			}
			// fall through
		default:
			fprintf(out,
					"<FORM METHOD='GET' ACTION='http://%s%s'><INPUT TYPE='TEXT' NAME='%s' VALUE='%.*s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",oc->host, oc->base_url,
					file, read_return, OWQ_buffer(owq));
			break;
	}
}

static void Upload( struct OutputControl * oc, const struct parsedname * pn )
{
	FILE * out = oc->out ;
	fprintf(out,"<FORM METHOD='POST'  ACTION='http://%s%s' ENCTYPE='multipart/form-data'>Load from file: <INPUT TYPE=FILE NAME='%s' SIZE=30><INPUT TYPE=SUBMIT VALUE='UPLOAD'></FORM>",oc->host, oc->base_url, pn->path);
}

static void Extension( struct OutputControl * oc, const struct parsedname * pn )
{
	static regex_t rx_extension ;
	FILE * out = oc->out ;
	const char * file = FS_DirName(pn);
	struct ow_regmatch orm ;
	orm.number = 0 ;

	ow_regcomp( &rx_extension, "\\.", 0 ) ;
	if ( ow_regexec( &rx_extension, file, &orm ) == 0 ) {
		fprintf(out, "<CODE><FORM METHOD='GET' ACTION='http://%s%s'><INPUT NAME='EXTENSION' TYPE='TEXT' SIZE='30' VALUE='%s.' ID='EXTENSION'><INPUT TYPE='SUBMIT' VALUE='EXTENSION'></FORM>", oc->host, oc->base_url, orm.pre[0] );
		ow_regexec_free( &orm ) ;
	}
}

/* Device entry -- table line for a filetype */
static void ShowReadonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	struct parsedname * pn = PN(owq) ;
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	switch (pn->selected_filetype->format) {
	case ft_binary:
		{
			int i = 0;
			fprintf(out, "<PRE>");
			while (i < read_return) {
				fprintf(out, "%.2hhX", OWQ_buffer(owq)[i]);
				if (((++i) < read_return) && (i & 0x1F) == 0) {
					fprintf(out, "\r\n");
				}
			}
			fprintf(out, "</PRE>");
			break;
		}
	case ft_yesno:
	case ft_bitfield:
		if (pn->extension >= 0) {
			switch (OWQ_buffer(owq)[0]) {
				case '0':
					fprintf(out, "NO  (0)");
					break;
				case '1':
					fprintf(out, "YES (1)");
					break;
			}
			break;
		}
		// fall through
	default:
		fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
		break;
	}
}

/* Structure entry */
static void ShowStructure(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));

	// Optional structure details
	StructureDetail(oc,OWQ_buffer(owq)) ;
}

/* Detailed (parsed) structure entry */
static void StructureDetail(struct OutputControl * oc, const char * structure_details )
{
	FILE * out = oc->out ;
	char format_type ;
	int extension ;
	int elements ;
	char rw[4] ;
	int size ;

	if ( sscanf( structure_details, "%c,%d,%d,%2s,%d,", &format_type, &extension, &elements, rw, &size ) < 5 ) {
		return ;
	}

	fprintf(out, "<br>");
	switch( format_type ) {
		case 'b':
			fprintf(out, "Binary string");
			break ;
		case 'a':
			fprintf(out, "Ascii string");
			break ;
		case 'D':
			fprintf(out, "Directory");
			break ;
		case 'i':
			fprintf(out, "Integer value");
			break ;
		case 'u':
			fprintf(out, "Unsigned integer value");
			break ;
		case 'f':
			fprintf(out, "Floating point value");
			break ;
		case 'l':
			fprintf(out, "Alias");
			break ;
		case 'y':
			fprintf(out, "Yes/No value");
			break ;
		case 'd':
			fprintf(out, "Date value");
			break ;
		case 't':
			fprintf(out, "Temperature value");
			break ;
		case 'g':
			fprintf(out, "Delta temperature value");
			break ;
		case 'p':
			fprintf(out, "Pressure value");
			break ;
		default:
			fprintf(out, "Unknown value type");
	}

	fprintf(out, ", ");

	if ( elements == 1 ) {
		fprintf(out, "Singleton");
	} else if ( extension == EXTENSION_BYTE ) {
		fprintf(out, "Array of %d bits as a BYTE",elements);
	} else if ( extension == EXTENSION_ALL ) {
		fprintf(out, "Array of %d elements combined",elements);
	} else {
		fprintf(out, "Element %d (of %d)",extension,elements);
	}

	fprintf(out, ", ");

	if ( strncasecmp( rw, "rw", 2 ) == 0 ) {
		fprintf(out, "Read/Write");
	} else if ( strncasecmp( rw, "ro", 2 ) == 0 ) {
		fprintf(out, "Read only");
	} else if ( strncasecmp( rw, "wo", 2 ) == 0 ) {
		fprintf(out, "Write only");
	} else{
		fprintf(out, "No access");
	}

	fprintf(out, ", ");

	if ( format_type == 'b' ) {
		fprintf(out, "%d bytes",size);
	} else {
		fprintf(out, "%d characters",size);
	}
}

/* Device entry -- table line for a filetype */
static void ShowWriteonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	struct parsedname * pn = PN(owq) ;
	const char *file = FS_DirName(pn);

	switch (pn->selected_filetype->format) {
		case ft_binary:
			fprintf(out,
					"<CODE><FORM METHOD='GET' ACTION='http://%s%s'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",oc->host, oc->base_url,
					file, (int) (OWQ_size(owq) >> 5));
			Upload(oc,pn) ;
			break;
		case ft_yesno:
		case ft_bitfield:
			if (pn->extension >= 0) {
				fprintf(out,
						"<FORM METHOD='GET' ACTION='http://%s%s'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>", oc->host, oc->base_url, file, file);
				break;
			}
			// fall through
		default:
			fprintf(out, "<FORM METHOD='GET' ACTION='http://%s%s'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>", oc->host, oc->base_url, file);
			break;
	}
}

static void ShowDirectory(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	FILE * out = oc->out ;
	fprintf(out, "<A HREF='%s'>%s</A>", pn_entry->path, FS_DirName(pn_entry));
}

/* Device entry -- table line for a filetype */
static void ShowText(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	FILE * out = oc->out ;
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir
	struct filetype * ft = pn_entry->selected_filetype ;

	/* Left column */
	fprintf(out, "%s ", FS_DirName(pn_entry));

	if (owq == NO_ONE_WIRE_QUERY) {
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		//fprintf(out, "(memory exhausted)");
	} else if (ft == NO_FILETYPE) {
		ShowTextDirectory(oc, pn_entry);
	} else if (ft->format == ft_directory || ft->format == ft_subdir) {
		ShowTextDirectory(oc, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowTextStructure(oc, owq);
	} else if (ft->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (ft->read != NO_READ_FUNCTION) {
			ShowTextReadonly(oc, owq);
		}
	} else {					// Writeable
		if (ft->read == NO_READ_FUNCTION) {
			ShowTextWriteonly(oc, owq);
		} else {
			ShowTextReadWrite(oc, owq);
		}
	}
	fprintf(out, "\r\n");
	OWQ_destroy(owq);
}

/* Device entry -- table line for a filetype */
static void ShowTextStructure(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		//fprintf(out, "error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
}

/* Device entry -- table line for a filetype */
static void ShowTextReadWrite(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		//fprintf(out, "error: %s", strerror(-read_return));
		return;
	}

	switch (PN(owq)->selected_filetype->format) {
	case ft_binary:
		{
			int i;
			for (i = 0; i < read_return; ++i) {
				fprintf(out, "%.2hhX", OWQ_buffer(owq)[i]);
			}
			break;
		}
	case ft_yesno:
	case ft_bitfield:
		if (PN(owq)->extension >= 0) {
			fprintf(out, "%c", OWQ_buffer(owq)[0]);
			break;
		}
		// fall through
	default:
		fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
		break;
	}
}

/* Device entry -- table line for a filetype */
static void ShowTextReadonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	ShowTextReadWrite(oc, owq);
}

/* Device entry -- table line for a filetype */
static void ShowTextWriteonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	(void) owq;
	fprintf(out, "(writeonly)");
}

static void ShowTextDirectory(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	(void) oc;
	(void) pn_entry;
}

/* Now show the device */
static void ShowDeviceTextCallback(void *v, const struct parsedname * pn_entry)
{
	struct OutputControl * oc = v;
	ShowText(oc, pn_entry);
}

static void ShowDeviceCallback(void *v, const struct parsedname * pn_entry)
{
	struct OutputControl * oc = v;
	Show(oc, pn_entry);
}

static void ShowDeviceText(struct OutputControl * oc, struct parsedname *pn)
{
	HTTPstart(oc, "200 OK", ct_text);

	if (pn->selected_filetype == NO_DEVICE) {	/* whole device */
		//printf("whole directory path=%s \n", pn->path);
		FS_dir(ShowDeviceTextCallback, oc, pn);
	} else {					/* Single item */
		//printf("single item path=%s\n", pn->path);
		ShowText(oc, pn);
	}
}

/* Device entry -- table line for a filetype */
static void ShowJson(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	FILE * out = oc->out ;
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir
	struct filetype * ft = pn_entry->selected_filetype ;

	if (owq == NO_ONE_WIRE_QUERY) {
		fprintf(out, "null");
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		fprintf(out, "null");
	} else if (ft == NO_FILETYPE) {
		ShowJsonDirectory(oc, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowJsonStructure(oc, owq);
	} else if (ft->format == ft_directory || ft->format == ft_subdir) {
		ShowJsonDirectory(oc, pn_entry);
	} else if (ft->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (ft->read != NO_READ_FUNCTION) {
			ShowJsonReadonly(oc, owq);
		}
	} else {					// Writeable
		if (ft->read == NO_READ_FUNCTION) {
			ShowJsonWriteonly(oc, owq);
		} else {
			ShowJsonReadWrite(oc, owq);
		}
	}
	OWQ_destroy(owq);
}

/* Device entry -- table line for a filetype */
static void ShowJsonStructure(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "null");
		return;
	}
	fprintf(out, "\"%.*s\":", read_return, OWQ_buffer(owq));
	StructureDetailJson( oc, OWQ_buffer(owq) ) ;
}

/* Detailed (parsed) structure entry */
static void StructureDetailJson(struct OutputControl * oc, const char * structure_details )
{
	FILE * out = oc->out ;
	char format_type ;
	int extension ;
	int elements ;
	char rw[4] ;
	int size ;

	if ( sscanf( structure_details, "[\"%c\",\"%d\",\"%d\",\"%2s\",\"%d\",", &format_type, &extension, &elements, rw, &size ) < 5 ) {
		return ;
	}

	fprintf(out, "\"");
	switch( format_type ) {
		case 'b':
			fprintf(out, "Binary string");
			break ;
		case 'a':
			fprintf(out, "Ascii string");
			break ;
		case 'D':
			fprintf(out, "Directory");
			break ;
		case 'i':
			fprintf(out, "Integer value");
			break ;
		case 'u':
			fprintf(out, "Unsigned integer value");
			break ;
		case 'f':
			fprintf(out, "Floating point value");
			break ;
		case 'l':
			fprintf(out, "Alias");
			break ;
		case 'y':
			fprintf(out, "Yes/No value");
			break ;
		case 'd':
			fprintf(out, "Date value");
			break ;
		case 't':
			fprintf(out, "Temperature value");
			break ;
		case 'g':
			fprintf(out, "Delta temperature value");
			break ;
		case 'p':
			fprintf(out, "Pressure value");
			break ;
		default:
			fprintf(out, "Unknown value type");
	}

	fprintf(out, "\", \"");

	if ( elements == 1 ) {
		fprintf(out, "Singleton");
	} else if ( extension == EXTENSION_BYTE ) {
		fprintf(out, "Array of %d bits as a BYTE",elements);
	} else if ( extension == EXTENSION_ALL ) {
		fprintf(out, "Array of %d elements combined",elements);
	} else {
		fprintf(out, "Element %d (of %d)",extension,elements);
	}

	fprintf(out, "\", \"");

	if ( strncasecmp( rw, "rw", 2 ) == 0 ) {
		fprintf(out, "Read/Write");
	} else if ( strncasecmp( rw, "ro", 2 ) == 0 ) {
		fprintf(out, "Read only");
	} else if ( strncasecmp( rw, "wo", 2 ) == 0 ) {
		fprintf(out, "Write only");
	} else{
		fprintf(out, "No access");
	}

	fprintf(out, "\", \"");

	if ( format_type == 'b' ) {
		fprintf(out, "%d bytes",size);
	} else {
		fprintf(out, "%d characters",size);
	}
	fprintf( out, "\"]" );
}

/* Device entry -- table line for a filetype */
static void ShowJsonReadWrite(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	struct parsedname * pn = PN(owq) ;
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);

	if (read_return < 0) {
		fprintf(out, "null");
		return;
	}

	switch (pn->selected_filetype->format) {
	case ft_binary:
		{
			int i;
			fprintf(out,"\"");
			for (i = 0; i < read_return; ++i) {
				fprintf(out, "%.2hhX", OWQ_buffer(owq)[i]);
			}
			fprintf(out,"\"");
			break;
		}
	case ft_yesno:
	case ft_bitfield:
		if (pn->extension >= 0) {
			fprintf(out, "\"%s\"", OWQ_buffer(owq)[0]=='0'?"false":"true");
			break;
		}
		// fall through
	default:
		fprintf(out, "\"%.*s\"", read_return, OWQ_buffer(owq));
		break;
	}
}

/* Device entry -- table line for a filetype */
static void ShowJsonReadonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	ShowJsonReadWrite(oc, owq);
}

/* Device entry -- table line for a filetype */
static void ShowJsonWriteonly(struct OutputControl * oc, struct one_wire_query *owq)
{
	FILE * out = oc->out ;
	(void) owq ;
	fprintf(out,"null") ;
}
static void ShowJsonDirectory(struct OutputControl * oc, const struct parsedname *pn_entry)
{
	FILE * out = oc->out ;
	(void) pn_entry;
	fprintf(out,"[]") ;
}

/* Now show the device */
static void ShowDeviceJsonCallback(void *v, const struct parsedname * pn_entry)
{
	struct OutputControl * oc = v ;
	JSON_dir_entry(oc, "\"%s\":",FS_DirName(pn_entry) ) ;
	ShowJson(oc, pn_entry);
}

static void ShowDeviceJson(struct OutputControl * oc, struct parsedname *pn)
{
	FILE * out = oc->out ;

	HTTPstart(oc, "200 OK", ct_json);

	if (pn->selected_filetype == NO_DEVICE) {	/* whole device */
		JSON_dir_init( oc ) ;
		fprintf(out, "{\n" ) ;
		FS_dir(ShowDeviceJsonCallback, oc, pn);
		JSON_dir_finish(oc) ;
		fprintf(out, "}" );
	} else {					/* Single item */
		//printf("single item path=%s\n", pn->path);
		fprintf(out, "[ " ) ;
		ShowJson(oc, pn);
		fprintf(out, " ]" ) ;
	}
}


void ShowDevice(struct OutputControl * oc, struct parsedname *pn)
{
	FILE * out = oc->out ;
	if (pn->state & ePS_text) {
		ShowDeviceText(oc, pn);
		return;
	} else if (pn->state & ePS_json) {
		ShowDeviceJson(oc, pn);
		return;
	}

	HTTPstart(oc, "200 OK", ct_html);

	HTTPtitle(oc, &pn->path[1]);
	HTTPheader(oc, &pn->path[1]);

	if (NotUncachedDir(pn) && IsRealDir(pn)) {
		fprintf(out, "<BR><small><A href='/uncached%s'>uncached version</A></small>", pn->path);
	}
	fprintf(out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>");
	fprintf(out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>directory</TD></TR>", Backup(pn->path), pn->path);


	if (pn->selected_filetype == NO_FILETYPE) {	/* whole device */
		FS_dir(ShowDeviceCallback, oc, pn);
	} else {					/* single item */
		Show(oc, pn);
	}
	fprintf(out, "</TABLE>");
	HTTPfoot(oc);
}
