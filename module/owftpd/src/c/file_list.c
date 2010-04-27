/*
 * $Id$
 */

#include "owftpd.h"
#include <limits.h>
#include <stdarg.h>
#include <fnmatch.h>

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca				/* predefined by HP cc +Olibcalls */
char *alloca();
#   endif
#  endif
# endif
#endif

static void fdprintf(int file_descriptor, const char *fmt, ...);
static void List_show(struct file_parse_s *fps, const struct parsedname *pn);
static void WildLexParse(struct file_parse_s *fps, ASCII * match);
static char *skip_ls_options(char *filespec);

/* if no localtime_r() is available, provide one */
#ifndef HAVE_LOCALTIME_R
#include <pthread.h>

struct tm *localtime_r(const time_t * timep, struct tm *timeptr)
{
	static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

	MUTEX_LOCK(time_lock);
	*timeptr = *(localtime(timep));
	MUTEX_UNLOCK(time_lock);
	return timeptr;
}
#endif							/* HAVE_LOCALTIME_R */

static void List_show(struct file_parse_s *fps, const struct parsedname *pn)
{
	struct stat stbuf;
	time_t now;
	struct tm tm_now;
	double age;
	char date_buf[13];
	ASCII *perms[] = {
		"---------",
		"--x--x--x",
		"-w--w--w-",
		"-wx-wx-wx",
		"r--r--r--",
		"r-xr-xr-x",
		"rw-rw-rw-",
		"rwxrwxrwx",
	};
	LEVEL_DEBUG("List_show %s", pn->path);
	switch (fps->fle) {
	case file_list_list:
		FS_fstat_postparse(&stbuf, pn);
		fdprintf(fps->out, "%s%s", stbuf.st_mode & S_IFDIR ? "d" : "-", perms[stbuf.st_mode & 0x07]);
		/* fps->output link & ownership information */
		fdprintf(fps->out, " %3d %-8d %-8d %8lu ", stbuf.st_nlink, stbuf.st_uid, stbuf.st_gid, (unsigned long) stbuf.st_size);
		/* fps->output date */
		time(&now);
		localtime_r(&stbuf.st_mtime, &tm_now);
		age = difftime(now, stbuf.st_mtime);
		if ((age > 60 * 60 * 24 * 30 * 6)
			|| (age < -(60 * 60 * 24 * 30 * 6))) {
			strftime(date_buf, sizeof(date_buf), "%b %e  %Y", &tm_now);
		} else {
			strftime(date_buf, sizeof(date_buf), "%b %e %H:%M", &tm_now);
		}
		fdprintf(fps->out, "%s ", date_buf);
		/* Fall Through */
	case file_list_nlst:
		/* fps->output filename */
		fdprintf(fps->out, "%s\r\n", &pn->path[fps->start]);
	}
}

void FileLexParse(struct file_parse_s *fps)
{
	struct parsedname pn;
	while (1) {
		switch (fps->pse) {
		case parse_status_init:
			LEVEL_DEBUG("FTP parse_status_init Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			fps->ret = 0;
			fps->start = strlen(fps->buffer);
			if (fps->start > 1)
				++fps->start;
			fps->rest = skip_ls_options(fps->rest);
			if (fps->rest == NULL || fps->rest[0] == '\0') {
				fps->pse = parse_status_tame;
			} else {
				if (fps->rest[0] == '/') {	// root specification
					fps->buffer[1] = '\0';
					fps->start = 0;
					++fps->rest;
				}
				fps->pse = parse_status_init2;
			}
			break;
		case parse_status_init2:
			LEVEL_DEBUG("FTP parse_status_init2 Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			if ((fps->rest[0] == '.' && fps->rest[1] == '.')
				|| strpbrk(fps->rest, "*[?")) {
				fps->pse = parse_status_back;
			} else {
				fps->pse = parse_status_tame;
			}
			break;
		case parse_status_back:
			LEVEL_DEBUG("FTP parse_status_back Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			if (fps->rest[0] == '.' && fps->rest[1] == '.') {
				// Move back
				ASCII *back = strrchr(fps->buffer, '/');
				back[1] = '\0';
				fps->start = strlen(fps->buffer);
				// look for next file part
				if (fps->rest[2] == '\0') {
					fps->pse = parse_status_last;
					fps->rest = NULL;
				} else if (fps->rest[2] == '/') {
					fps->pse = parse_status_next;
					fps->rest = &fps->rest[3];
				} else {
					fps->ret = -ENOENT;
					return;
				}
			} else {
				fps->pse = parse_status_next;	// off the double dot trail
			}
			break;
		case parse_status_next:
			LEVEL_DEBUG("FTP parse_status_next Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			if (fps->rest == NULL || fps->rest[0] == '\0') {
				fps->pse = parse_status_last;
			} else {
				ASCII *oldrest = strsep(&fps->rest, "/");
				if (strpbrk(oldrest, "*[?")) {
					WildLexParse(fps, oldrest);
					return;
				} else {
					if (oldrest && (strlen(fps->buffer) + strlen(oldrest) + 4 > PATH_MAX)) {
						fps->ret = -ENAMETOOLONG;
						return;
					}
					if (fps->buffer[1])
						strcat(fps->buffer, "/");
					strcat(fps->buffer, oldrest);
					fps->pse = parse_status_next;
				}
			}
			break;
		case parse_status_tame:
			LEVEL_DEBUG("FTP parse_status_tame Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			if (fps->rest && (strlen(fps->buffer) + strlen(fps->rest) + 4 > PATH_MAX)) {
				fps->ret = -ENAMETOOLONG;
				return;
			}
			if (fps->buffer[1])
				strcat(fps->buffer, "/");
			strcat(fps->buffer, fps->rest);
			if (FS_ParsedName(fps->buffer, &pn) == 0) {
				if (IsDir(&pn)) {
					fps->start = strlen(fps->buffer) + 1;
					if (fps->start == 2) {
						fps->start = 1;
					} else if (fps->buffer[fps->start - 2] == '/') {
						--fps->start;
						fps->buffer[fps->start - 1] = '\0';
					}
					fps->rest = NULL;
					WildLexParse(fps, "*");
				} else {
					List_show(fps, &pn);
				}
				FS_ParsedName_destroy(&pn);
			} else {
				fps->ret = -ENOENT;
			}
			return;
		case parse_status_last:
			LEVEL_DEBUG("FTP parse_status_last Path<%s> File <%s>", fps->buffer, fps->rest);
			/* fps->buffer is absolute */
			/* trailing / only at root */
			if (fps->rest && (strlen(fps->buffer) + strlen(fps->rest) + 4 > PATH_MAX)) {
				fps->ret = -ENAMETOOLONG;
				return;
			}
			if (FS_ParsedNamePlus(fps->buffer, fps->rest, &pn) == 0) {
				List_show(fps, &pn);
				FS_ParsedName_destroy(&pn);
			}
			return;
		}
	}
}

struct wildlexparse {
	ASCII *end;
	ASCII *match;
	struct file_parse_s *fps;
};
/* Called for each directory element, and operates recursively */
/* uses C library fnmatch for file wildcard comparisons */
static void WildLexParseCallback(void *v, const struct parsedname *const pn_entry)
{
	struct wildlexparse *wlp = v;
	struct file_parse_s fps;	// duplicate for recursive call

	/* get real name from parsedname struct */
	strcpy(&wlp->end[1], FS_DirName(pn_entry));

	LEVEL_DEBUG("WildLexParseCallback: Try %s vs %s", &wlp->end[1], wlp->match);
	if (fnmatch(wlp->match, &wlp->end[1], FNM_PATHNAME)) {
		return;
	}

	/* Matched! So set up for recursive call on nect elements in path name */
	memcpy(&fps, wlp->fps, sizeof(fps));
	fps.pse = parse_status_next;
	FileLexParse(&fps);
}

/* Called for each directory, calls WildLEexParseCallback on each element
   to see if matches wildcards (even used for normal listings with * wildcard)
 */
static void WildLexParse(struct file_parse_s *fps, ASCII * match)
{
	struct parsedname pn;
	/* Embedded callback function */

	LEVEL_DEBUG("FTP Wildcard patern matching: Path=%s, Pattern=%s, File=%s", SAFESTRING(fps->buffer), SAFESTRING(match), SAFESTRING(fps->rest));

	/* Check potential length */
	if (strlen(fps->buffer) + OW_FULLNAME_MAX + 2 > PATH_MAX) {
		fps->ret = -ENAMETOOLONG;
		return;
	}

	/* Can we understand the current path? */
	if (FS_ParsedName(fps->buffer, &pn) != 0 ) {
		fps->ret = -ENOENT;
		return;
	}

	if (pn.selected_filetype) {
		fps->ret = -ENOTDIR;
	} else {
		struct wildlexparse wlp = { NULL, match, fps, };
		int root = (fps->buffer[1] == '\0');

		wlp.end = &fps->buffer[strlen(fps->buffer)];

		if (root)
			--wlp.end;
		wlp.end[0] = '/';
		FS_dir(WildLexParseCallback, &wlp, &pn);
		if (root)
			++wlp.end;
		wlp.end[0] = '\0';		// restore fps->buffer
	}
	FS_ParsedName_destroy(&pn);

}

/* write with care for max length and incomplete outout */
static void fdprintf(int file_descriptor, const char *fmt, ...)
{
	char buf[PATH_MAX + 1];
	ssize_t buflen;
	va_list ap;
	int amt_written;
	int write_ret;

	daemon_assert(file_descriptor >= 0);
	daemon_assert(fmt != NULL);

	va_start(ap, fmt);
	buflen = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (buflen <= 0) {
		return;
	}
	if ((size_t) buflen >= sizeof(buf)) {
		buflen = sizeof(buf) - 1;
	}

	amt_written = 0;
	while (amt_written < buflen) {
		write_ret = write(file_descriptor, buf + amt_written, buflen - amt_written);
		if (write_ret <= 0) {
			return;
		}
		amt_written += write_ret;
	}
}

/* 
  hack workaround clients like Midnight Commander that send:
      LIST -al /dirname 
*/
static char *skip_ls_options(char *filespec)
{
	daemon_assert(filespec != NULL);

	for (;;) {
		/* end when we've passed all options */
		if (*filespec != '-') {
			break;
		}
		filespec++;

		/* if we find "--", eat it and any following whitespace then return */
		if ((filespec[0] == '-') && (filespec[1] == ' ')) {
			filespec += 2;
			while (isspace(*filespec)) {
				filespec++;
			}
			break;
		}

		/* otherwise, skip this option */
		while ((*filespec != '\0') && !isspace(*filespec)) {
			filespec++;
		}

		/* and skip any whitespace */
		while (isspace(*filespec)) {
			filespec++;
		}
	}

	daemon_assert(filespec != NULL);

	return filespec;
}
