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

/* --------------- Prototypes---------------- */
//static void Show(FILE * out, const char *const path, const char *const file);
//static void ShowText(FILE * out, const char *path, const char *file);

static void Show(FILE * out, const struct parsedname *pn_entry);
static void ShowDirectory(FILE * out, const struct parsedname *pn_entry);
static void ShowReadWrite(FILE * out, struct one_wire_query *owq);
static void ShowReadonly(FILE * out, struct one_wire_query *owq);
static void ShowWriteonly(FILE * out, struct one_wire_query *owq);
static void ShowStructure(FILE * out, struct one_wire_query *owq);
static void StructureDetail(FILE * out, const char * structure_details );
static void Upload( FILE * out, const struct parsedname * pn ) ;

static void ShowText(FILE * out, const struct parsedname *pn_entry);
static void ShowTextDirectory(FILE * out, const struct parsedname *pn_entry);
static void ShowTextReadWrite(FILE * out, struct one_wire_query *owq);
static void ShowTextReadonly(FILE * out, struct one_wire_query *owq);
static void ShowTextWriteonly(FILE * out, struct one_wire_query *owq);
static void ShowTextStructure(FILE * out, struct one_wire_query *owq);

static void ShowJson(FILE * out, const struct parsedname *pn_entry);
static void ShowJsonDirectory(FILE * out, const struct parsedname *pn_entry);
static void ShowJsonReadWrite(FILE * out, struct one_wire_query *owq);
static void ShowJsonReadonly(FILE * out, struct one_wire_query *owq);
static void ShowJsonWriteonly(FILE * out, struct one_wire_query *owq);
static void ShowJsonStructure(FILE * out, struct one_wire_query *owq);
static void StructureDetailJson(FILE * out, const char * structure_details );

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show(FILE * out, const struct parsedname *pn_entry)
{
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir
	
	/* Left column */
	fprintf(out, "<TR><TD><B>%s</B></TD><TD>", FS_DirName(pn_entry));

	if (owq == NO_ONE_WIRE_QUERY) {
		fprintf(out, "<B>Memory exhausted</B>");
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		fprintf(out, "<B>Memory exhausted</B>");
	} else if (pn_entry->selected_filetype == NO_FILETYPE) {
		ShowDirectory(out, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowStructure(out, owq);
	} else if (pn_entry->selected_filetype->format == ft_directory || pn_entry->selected_filetype->format == ft_subdir) {
		// Directory
		ShowDirectory(out, pn_entry);
	} else if (pn_entry->selected_filetype->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (pn_entry->selected_filetype->read != NO_READ_FUNCTION) {
			ShowReadonly(out, owq);
		}
	} else {					// Writeable
		if (pn_entry->selected_filetype->read == NO_READ_FUNCTION) {
			ShowWriteonly(out, owq);
		} else {
			ShowReadWrite(out, owq);
		}
	}
	fprintf(out, "</TD></TR>\r\n");
	OWQ_destroy(owq);
}


/* Device entry -- table line for a filetype */
static void ShowReadWrite(FILE * out, struct one_wire_query *owq)
{
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
			fprintf(out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'>", file, read_return >> 5);
			while (i < read_return) {
				fprintf(out, "%.2hhX", OWQ_buffer(owq)[i]);
				if (((++i) < read_return) && (i & 0x1F) == 0) {
					fprintf(out, "\r\n");
				}
			}
			fprintf(out, "</TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>");
			Upload( out, pn ) ;
			break;
		}
	case ft_yesno:
	case ft_bitfield:
		if (pn->extension >= 0) {
			fprintf(out,
					"<FORM METHOD=\"GET\"><INPUT TYPE='CHECKBOX' NAME='%s' %s><INPUT TYPE='SUBMIT' VALUE='CHANGE' NAME='%s'></FORM></FORM>",
					file, (OWQ_buffer(owq)[0] == '0') ? "" : "CHECKED", file);
			break;
		}
		// fall through
	default:
		fprintf(out,
				"<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%.*s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",
				file, read_return, OWQ_buffer(owq));
		break;
	}
}

static void Upload( FILE * out, const struct parsedname * pn )
{
	fprintf(out,"<FORM METHOD='POST' ENCTYPE='multipart/form-data'>Load from file: <INPUT TYPE=FILE NAME='%s' SIZE=30><INPUT TYPE=SUBMIT VALUE='UPLOAD'></FORM>",pn->path);
}

/* Device entry -- table line for a filetype */
static void ShowReadonly(FILE * out, struct one_wire_query *owq)
{
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	switch (PN(owq)->selected_filetype->format) {
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
		if (PN(owq)->extension >= 0) {
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
static void ShowStructure(FILE * out, struct one_wire_query *owq)
{
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
	
	// Optional structure details
	StructureDetail(out,OWQ_buffer(owq)) ;
}

/* Detailed (parsed) structure entry */
static void StructureDetail(FILE * out, const char * structure_details )
{
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
static void ShowWriteonly(FILE * out, struct one_wire_query *owq)
{
	const char *file = FS_DirName(PN(owq));
	switch (PN(owq)->selected_filetype->format) {
	case ft_binary:
		fprintf(out,
				"<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",
				file, (int) (OWQ_size(owq) >> 5));
		Upload(out,PN(owq)) ;
		break;
	case ft_yesno:
	case ft_bitfield:
		if (PN(owq)->extension >= 0) {
			fprintf(out,
					"<FORM METHOD='GET'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>", file, file);
			break;
		}
		// fall through
	default:
		fprintf(out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>", file);
		break;
	}
}

static void ShowDirectory(FILE * out, const struct parsedname *pn_entry)
{
	fprintf(out, "<A HREF='%s'>%s</A>", pn_entry->path, FS_DirName(pn_entry));
}

/* Device entry -- table line for a filetype */
static void ShowText(FILE * out, const struct parsedname *pn_entry)
{
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir

	/* Left column */
	fprintf(out, "%s ", FS_DirName(pn_entry));

	if (owq == NO_ONE_WIRE_QUERY) {
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		//fprintf(out, "(memory exhausted)");
	} else if (pn_entry->selected_filetype == NO_FILETYPE) {
		ShowTextDirectory(out, pn_entry);
	} else if (pn_entry->selected_filetype->format == ft_directory || pn_entry->selected_filetype->format == ft_subdir) {
		ShowTextDirectory(out, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowTextStructure(out, owq);
	} else if (pn_entry->selected_filetype->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (pn_entry->selected_filetype->read != NO_READ_FUNCTION) {
			ShowTextReadonly(out, owq);
		}
	} else {					// Writeable
		if (pn_entry->selected_filetype->write == NO_READ_FUNCTION) {
			ShowTextWriteonly(out, owq);
		} else {
			ShowTextReadWrite(out, owq);
		}
	}
	fprintf(out, "\r\n");
	OWQ_destroy(owq);
}

/* Device entry -- table line for a filetype */
static void ShowTextStructure(FILE * out, struct one_wire_query *owq)
{
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		//fprintf(out, "error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
}

/* Device entry -- table line for a filetype */
static void ShowTextReadWrite(FILE * out, struct one_wire_query *owq)
{
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
static void ShowTextReadonly(FILE * out, struct one_wire_query *owq)
{
	ShowTextReadWrite(out, owq);
}

/* Device entry -- table line for a filetype */
static void ShowTextWriteonly(FILE * out, struct one_wire_query *owq)
{
	(void) owq;
	fprintf(out, "(writeonly)");
}

static void ShowTextDirectory(FILE * out, const struct parsedname *pn_entry)
{
	(void) out;
	(void) pn_entry;
}

/* Now show the device */
static void ShowDeviceTextCallback(void *v, const struct parsedname * pn_entry)
{
	FILE *out = v;
	ShowText(out, pn_entry);
}

static void ShowDeviceCallback(void *v, const struct parsedname * pn_entry)
{
	FILE *out = v;
//    Show(sds->out, sds->path, FS_DirName(pn_entry));
	Show(out, pn_entry);
}

static void ShowDeviceText(FILE * out, struct parsedname *pn)
{
	HTTPstart(out, "200 OK", ct_text);

	if (pn->selected_filetype == NO_DEVICE) {	/* whole device */
		//printf("whole directory path=%s \n", pn->path);
		FS_dir(ShowDeviceTextCallback, out, pn);
	} else {					/* Single item */
		//printf("single item path=%s\n", pn->path);
		ShowText(out, pn);
	}
}

/* Device entry -- table line for a filetype */
static void ShowJson(FILE * out, const struct parsedname *pn_entry)
{
	struct one_wire_query *owq = OWQ_create_from_path(pn_entry->path); // for read or dir

	if (owq == NO_ONE_WIRE_QUERY) {
		fprintf(out, "null");
	} else if ( BAD( OWQ_allocate_read_buffer(owq)) ) {
		fprintf(out, "null");
	} else if (pn_entry->selected_filetype == NO_FILETYPE) {
		ShowJsonDirectory(out, pn_entry);
	} else if (IsStructureDir(pn_entry)) {
		ShowJsonStructure(out, owq);
	} else if (pn_entry->selected_filetype->format == ft_directory || pn_entry->selected_filetype->format == ft_subdir) {
		ShowJsonDirectory(out, pn_entry);
	} else if (pn_entry->selected_filetype->write == NO_WRITE_FUNCTION || Globals.readonly) {
		// Unwritable
		if (pn_entry->selected_filetype->read != NO_READ_FUNCTION) {
			ShowJsonReadonly(out, owq);
		}
	} else {					// Writeable
		if (pn_entry->selected_filetype->write == NO_READ_FUNCTION) {
			ShowJsonWriteonly(out, owq);
		} else {
			ShowJsonReadWrite(out, owq);
		}
	}
	OWQ_destroy(owq);
}

/* Device entry -- table line for a filetype */
static void ShowJsonStructure(FILE * out, struct one_wire_query *owq)
{
	SIZE_OR_ERROR read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "null");
		return;
	}
	fprintf(out, "\"%.*s\":", read_return, OWQ_buffer(owq));
	StructureDetailJson( out, OWQ_buffer(owq) ) ;
}

/* Detailed (parsed) structure entry */
static void StructureDetailJson(FILE * out, const char * structure_details )
{
	char format_type ;
	int extension ;
	int elements ;
	char rw[4] ;
	int size ;

	if ( sscanf( structure_details, "%c,%d,%d,%2s,%d,", &format_type, &extension, &elements, rw, &size ) < 5 ) {
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
static void ShowJsonReadWrite(FILE * out, struct one_wire_query *owq)
{
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
		if (PN(owq)->extension >= 0) {
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
static void ShowJsonReadonly(FILE * out, struct one_wire_query *owq)
{
	ShowJsonReadWrite(out, owq);
}

/* Device entry -- table line for a filetype */
static void ShowJsonWriteonly(FILE * out, struct one_wire_query *owq)
{
	(void) owq ;
	fprintf(out,"null") ;
}
static void ShowJsonDirectory(FILE * out, const struct parsedname *pn_entry)
{
	(void) pn_entry;
	fprintf(out,"[]") ;
}

/* Now show the device */
static void ShowDeviceJsonCallback(void *v, const struct parsedname * pn_entry)
{
	FILE *out = v;
	fprintf(out, "\"%s\":",FS_DirName(pn_entry) ) ;
	ShowJson(out, pn_entry);
	fprintf(out, ",\n" ) ;
}

static void ShowDeviceJson(FILE * out, struct parsedname *pn)
{
	HTTPstart(out, "200 OK", ct_text);

	if (pn->selected_filetype == NO_DEVICE) {	/* whole device */
		fprintf(out, "{\n" ) ;
		FS_dir(ShowDeviceJsonCallback, out, pn);
		fprintf(out, "}" );
	} else {					/* Single item */
		//printf("single item path=%s\n", pn->path);
		ShowJson(out, pn);
	}
}


void ShowDevice(FILE * out, struct parsedname *pn)
{
	if (pn->state & ePS_text) {
		ShowDeviceText(out, pn);
		return;
	} else if (pn->state & ePS_json) {
		ShowDeviceJson(out, pn);
		return;
	}

	HTTPstart(out, "200 OK", ct_html);

	HTTPtitle(out, &pn->path[1]);
	HTTPheader(out, &pn->path[1]);

	if (NotUncachedDir(pn) && IsRealDir(pn)) {
		fprintf(out, "<BR><small><A href='/uncached%s'>uncached version</A></small>", pn->path);
	}
	fprintf(out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>");
	fprintf(out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>directory</TD></TR>", Backup(pn->path), pn->path);


	if (pn->selected_filetype == NO_FILETYPE) {	/* whole device */
		FS_dir(ShowDeviceCallback, out, pn);
	} else {					/* single item */
		Show(out, pn);
	}
	fprintf(out, "</TABLE>");
	HTTPfoot(out);
}
