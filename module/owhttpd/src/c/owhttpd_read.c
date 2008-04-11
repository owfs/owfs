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
static void Show(FILE * out, const char *const path, const char *const file);
static void ShowText(FILE * out, const char *path, const char *file);

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show(FILE * out, const char *path, const char *file)
{
	struct one_wire_query owq;
	struct parsedname *pn;		// for convenience

	//printf("Show: path=%s, file=%s\n",path,file) ;
	if (FS_OWQ_create_plus(path, file, NULL, 0, 0, &owq)) {
		fprintf(out, "<TR><TD><B>%s</B></TD><TD>", file);
		fprintf(out, "<B>Unparsable name</B></TD></TR>");
		return;
	}
	pn = PN(&owq);

	/* Left column */
	fprintf(out, "<TR><TD><B>%s</B></TD><TD>", file);

	if (IsDir(pn)) {			/* Directory jump */
		fprintf(out, "<A HREF='%s'>%s</A>", pn->path, file);
	} else {
		int canwrite = !Global.readonly && (pn->selected_filetype->write != NO_WRITE_FUNCTION);
		int canread = (pn->selected_filetype->read != NO_READ_FUNCTION);
		enum ft_format format = pn->selected_filetype->format;
		int len = 0;			// initialize to avoid compiler warning

		if (IsStructureDir(pn)) {
			format = ft_ascii;
			canread = 1;
			canwrite = 0;
		}

		OWQ_size(&owq) = FullFileLength(PN(&owq));

		/* buffer for field value */
		if ((OWQ_buffer(&owq) = malloc(OWQ_size(&owq) + 1))) {
			OWQ_buffer(&owq)[OWQ_size(&owq)] = '\0';

			if (canread) {		/* At least readable */
				if ((len = FS_read_postparse(&owq)) >= 0) {
					OWQ_buffer(&owq)[len] = '\0';
					//printf("SHOW read of %s len = %d, value=%s\n",pn.path,len,SAFESTRING(buf)) ;
				}
			}

			if (format == ft_binary) {	/* bianry uses HEX mode */
				if (canread) {	/* At least readable */
					if (len >= 0) {
						if (canwrite) {	/* read-write */
							int i = 0;
							fprintf(out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'>", file, len >> 5);
							while (i < len) {
								fprintf(out, "%.2hhX", OWQ_buffer(&owq)[i]);
								if (((++i) < len) && (i & 0x1F) == 0)
									fprintf(out, "\r\n");
							}
							fprintf(out, "</TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>");
						} else {	/* read only */
							int i = 0;
							fprintf(out, "<PRE>");
							while (i < len) {
								fprintf(out, "%.2hhX", OWQ_buffer(&owq)[i]);
								if (((++i) < len) && (i & 0x1F) == 0)
									fprintf(out, "\r\n");
							}
							fprintf(out, "</PRE>");
						}
					} else {
						//printf("Error read %s %d\n", fullpath, suglen);
						fprintf(out, "Error: %s", strerror(-len));
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out,
							"<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",
							file, (pn->selected_filetype->suglen) >> 5);
				}
			} else if (pn->extension >= 0 && (format == ft_yesno || format == ft_bitfield)) {
				if (canread) {	/* at least readable */
					if (len >= 0) {
						if (canwrite) {	/* read-write */
							fprintf(out,
									"<FORM METHOD=\"GET\"><INPUT TYPE='CHECKBOX' NAME='%s' %s><INPUT TYPE='SUBMIT' VALUE='CHANGE' NAME='%s'></FORM></FORM>",
									file, (OWQ_buffer(&owq)[0] == '0') ? "" : "CHECKED", file);
						} else {	/* read-only */
							switch (OWQ_buffer(&owq)[0]) {
							case '0':
								fprintf(out, "NO");
								break;
							case '1':
								fprintf(out, "YES");
								break;
							}
						}
					} else {
						fprintf(out, "Error: %s", strerror(-len));
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out,
							"<FORM METHOD='GET'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>",
							file, file);
				}
			} else {
				if (canread) {	/* At least readable */
					if (len >= 0) {
						if (canwrite) {	/* read-write */
							fprintf(out,
									"<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",
									file, OWQ_buffer(&owq));
						} else {	/* read only */
							fprintf(out, "%s", OWQ_buffer(&owq));
						}
					} else {
						fprintf(out, "Error: %s", strerror(-len));
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>", file);
				}
			}
			free(OWQ_buffer(&owq));
		}
	}
	fprintf(out, "</TD></TR>\r\n");
	FS_OWQ_destroy(&owq);
}

/* Device entry -- table line for a filetype */
static void ShowText(FILE * out, const char *path, const char *file)
{
	struct one_wire_query owq;
	struct parsedname *pn;		// for convenience

	if (FS_OWQ_create_plus(path, file, NULL, 0, 0, &owq)) {
		return;
	}
	pn = PN(&owq);

	/* Left column */
	fprintf(out, "%s ", file);

	if (IsDir(pn)) {			/* Directory jump */
	} else {
		int canwrite = !Global.readonly && (pn->selected_filetype->write != NO_READ_FUNCTION);
		int canread = (pn->selected_filetype->read != NO_WRITE_FUNCTION);
		enum ft_format format = pn->selected_filetype->format;
		int len = 0;			// initialize to avoid compiler warning

		if (IsStructureDir(pn)) {
			format = ft_ascii;
			canread = 1;
			canwrite = 0;
		}

		OWQ_size(&owq) = FullFileLength(pn);

		/* buffer for field value */
		OWQ_buffer(&owq) = malloc(OWQ_size(&owq) + 1);
		if (OWQ_buffer(&owq) != NULL) {
			OWQ_buffer(&owq)[OWQ_size(&owq)] = '\0';

			if (canread) {		/* At least readable */
				if ((len = FS_read_postparse(&owq)) >= 0) {
					OWQ_buffer(&owq)[len] = '\0';
				}
			}

			if (format == ft_binary) {	/* bianry uses HEX mode */
				if (canread) {	/* At least readable */
					if (len >= 0) {
						int i;
						for (i = 0; i < len; ++i) {
							fprintf(out, "%.2hhX", OWQ_buffer(&owq)[i]);
						}
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out, "(writeonly)");
				}
			} else if (pn->extension >= 0 && (format == ft_yesno || format == ft_bitfield)) {
				if (canread) {	/* at least readable */
					if (len >= 0) {
						fprintf(out, "%c", OWQ_buffer(&owq)[0]);
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out, "(writeonly)");
				}
			} else {
				if (canread) {	/* At least readable */
					if (len >= 0) {
						fprintf(out, "%s", OWQ_buffer(&owq));
					}
				} else if (canwrite) {	/* rare write-only */
					fprintf(out, "(writeonly)");
				}
			}
			free(OWQ_buffer(&owq));
		}
	}
	fprintf(out, "\r\n");
	FS_OWQ_destroy(&owq);
}


/* Now show the device */
struct showdevicestruct {
	char *path;
	FILE *out;
};
static void ShowDeviceTextCallback(void *v, const struct parsedname *const pn_entry)
{
	struct showdevicestruct *sds = v;
    ShowText(sds->out, sds->path, FS_DirName(pn_entry));
}
static void ShowDeviceCallback(void *v, const struct parsedname *const pn_entry)
{
	struct showdevicestruct *sds = v;
    Show(sds->out, sds->path, FS_DirName(pn_entry));
}
static void ShowDeviceText(FILE * out, const struct parsedname *const pn)
{
	struct showdevicestruct sds = { NULL, out };
	char *slash;

	//printf("ShowDevice = %s  bus_nr=%d pn->dev=%p\n",pn->path, pn->known_bus->index, pn->dev) ;
	if (!(sds.path = strdup(pn->path)))
		return;

	HTTPstart(out, "200 OK", ct_text);

	if (pn->selected_filetype) {	/* single item */
		//printf("single item path=%s pn->path=%s\n", path2, pn->path);
		slash = strrchr(sds.path, '/');
		/* Nested function */
		if (slash)
			slash[0] = '\0';	/* pare off device name */
		//printf("single item path=%s\n", path2);
		ShowDeviceTextCallback(&sds, pn);
	} else {					/* whole device */
		//printf("whole directory path=%s pn->path=%s\n", path2, pn->path);
		//printf("pn->dev=%p pn->selected_filetype=%p pn->subdir=%p\n", pn->dev, pn->selected_filetype, pn->subdir);
		FS_dir(ShowDeviceTextCallback, &sds, pn);
	}
	free(sds.path);
}
void ShowDevice(FILE * out, const struct parsedname *const pn)
{
	struct showdevicestruct sds = { NULL, out };
	char *slash;
	int b;
	if (pn->state & ePS_text) {
		ShowDeviceText(out, pn);
		return;
	}
	//printf("ShowDevice = %s  bus_nr=%d pn->dev=%p\n",pn->path, pn->known_bus->index, pn->dev) ;
	if (!(sds.path = strdup(pn->path)))
		return;

	HTTPstart(out, "200 OK", ct_html);
	b = Backup(pn->path);
	HTTPtitle(out, &pn->path[1]);
	HTTPheader(out, &pn->path[1]);
	if (IsLocalCacheEnabled(pn) && NotUncachedDir(pn) && IsRealDir(pn))
		fprintf(out, "<BR><small><A href='/uncached%s'>uncached version</A></small>", pn->path);
	fprintf(out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>");
	fprintf(out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>directory</TD></TR>", b, pn->path);


	if (pn->selected_filetype) {	/* single item */
		//printf("single item path=%s pn->path=%s\n", path2, pn->path);
		slash = strrchr(sds.path, '/');
		/* Nested function */
		if (slash)
			slash[0] = '\0';	/* pare off device name */
		//printf("single item path=%s\n", path2);
		ShowDeviceCallback(&sds, pn);
	} else {					/* whole device */
		//printf("whole directory path=%s pn->path=%s\n", path2, pn->path);
		//printf("pn->dev=%p pn->selected_filetype=%p pn->subdir=%p\n", pn->dev, pn->selected_filetype, pn->subdir);
		FS_dir(ShowDeviceCallback, &sds, pn);
	}
	fprintf(out, "</TABLE>");
	HTTPfoot(out);
	free(sds.path);
}
