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
static void Upload( FILE * out, const struct parsedname * pn ) ;

static void ShowText(FILE * out, const struct parsedname *pn_entry);
static void ShowTextDirectory(FILE * out, const struct parsedname *pn_entry);
static void ShowTextReadWrite(FILE * out, struct one_wire_query *owq);
static void ShowTextReadonly(FILE * out, struct one_wire_query *owq);
static void ShowTextWriteonly(FILE * out, struct one_wire_query *owq);
static void ShowTextStructure(FILE * out, struct one_wire_query *owq);

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show(FILE * out, const struct parsedname *pn_entry)
{
	struct one_wire_query *owq = FS_OWQ_from_pn(pn_entry);

	/* Left column */
	fprintf(out, "<TR><TD><B>%s</B></TD><TD>", FS_DirName(pn_entry));

	if (owq == NULL) {
		fprintf(out, "<B>Memory exhausted</B>");
	} else if (pn_entry->selected_filetype == NULL) {
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
	FS_OWQ_destroy_not_pn(owq);
}


/* Device entry -- table line for a filetype */
static void ShowReadWrite(FILE * out, struct one_wire_query *owq)
{
	const char *file = FS_DirName(PN(owq));
	int read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}
	switch (PN(owq)->selected_filetype->format) {
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
			Upload( out, PN(owq) ) ;
			break;
		}
	case ft_yesno:
	case ft_bitfield:
		if (PN(owq)->extension >= 0) {
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
	int read_return = FS_read_postparse(owq);
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

/* Device entry -- table line for a filetype */
static void ShowStructure(FILE * out, struct one_wire_query *owq)
{
	int read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		fprintf(out, "Error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
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
	struct one_wire_query *owq = FS_OWQ_from_pn(pn_entry);

	/* Left column */
	fprintf(out, "%s ", FS_DirName(pn_entry));

	if (owq == NULL) {
		//fprintf(out, "(memory exhausted)");
	} else if (pn_entry->selected_filetype == NULL) {
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
	FS_OWQ_destroy_not_pn(owq);
}

/* Device entry -- table line for a filetype */
static void ShowTextStructure(FILE * out, struct one_wire_query *owq)
{
	int read_return = FS_read_postparse(owq);
	if (read_return < 0) {
		//fprintf(out, "error: %s", strerror(-read_return));
		return;
	}

	fprintf(out, "%.*s", read_return, OWQ_buffer(owq));
}

/* Device entry -- table line for a filetype */
static void ShowTextReadWrite(FILE * out, struct one_wire_query *owq)
{
	int read_return = FS_read_postparse(owq);
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

	if (pn->selected_filetype) {	/* single item */
		//printf("single item path=%s\n", pn->path);
		ShowText(out, pn);
	} else {					/* whole device */
		//printf("whole directory path=%s \n", pn->path);
		FS_dir(ShowDeviceTextCallback, out, pn);
	}
}

void ShowDevice(FILE * out, struct parsedname *pn)
{
	if (pn->state & ePS_text) {
		ShowDeviceText(out, pn);
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


	if (pn->selected_filetype) {	/* single item */
		Show(out, pn);
	} else {					/* whole device */
		FS_dir(ShowDeviceCallback, out, pn);
	}
	fprintf(out, "</TABLE>");
	HTTPfoot(out);
}
