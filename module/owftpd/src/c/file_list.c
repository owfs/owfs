/*
 * $Id$
 */

#include "owftpd.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

/* GLOB_PERIOD is defined in Linux but not Solaris */
#ifndef GLOB_PERIOD
#define GLOB_PERIOD 0
#endif /* GLOB_PERIOD */

/* GLOB_ABORTED is defined in Linux but not on FreeBSD */
#ifndef GLOB_ABORTED
#ifdef GLOB_ABEND
#define GLOB_ABORTED GLOB_ABEND
#endif
#endif

static int is_valid_dir(const char *dir);
static void fdprintf(int fd, const char *fmt, ...);
static const char *skip_ls_options(const char *filespec);

/* if no localtime_r() is available, provide one */
#ifndef HAVE_LOCALTIME_R
#include <pthread.h>

struct tm *localtime_r(const time_t *timep, struct tm *timeptr) 
{
    static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&time_lock);
    *timeptr = *(localtime(timep));
    pthread_mutex_unlock(&time_lock);
    return timeptr;
}
#endif /* HAVE_LOCALTIME_R */

int file_nlst(int out, const char *cur_dir, const char *filespec)
{
    int dir_len;
    char pattern[PATH_MAX+1];
    int glob_ret;
    glob_t glob_buf;
    int i;
    char *file_name;

    daemon_assert(out >= 0);
    daemon_assert(is_valid_dir(cur_dir));
    daemon_assert(filespec != NULL);

    if (filespec[0] == '/') {
        cur_dir = "";
        dir_len = 0;
    } else {
        strcpy(pattern, cur_dir);
	if ((cur_dir[0] != '/') || (cur_dir[1] != '\0')) {
            strcat(pattern, "/");
	}
        dir_len = strlen(pattern);
    }

    /* make sure we have enough space */
    if ((dir_len + 1 + strlen(filespec)) > PATH_MAX) {
        fdprintf(out, "Error; Path name too long\r\n");
	return 0;
    }
    strcat(pattern, filespec);

    /* do a glob() */
    memset(&glob_buf, 0, sizeof(glob_buf));
    glob_ret = glob(pattern, 
                    GLOB_ERR | GLOB_NOSORT | GLOB_PERIOD, 
		    NULL, 
		    &glob_buf);
    if (glob_ret == GLOB_NOSPACE) {
        fdprintf(out, "Error; Out of memory\r\n");
	return 0;
#ifdef GLOB_NOMATCH  /* not present in FreeBSD */
    } else if (glob_ret == GLOB_NOMATCH) {
        return 1;
#endif /* GLOB_NOMATCH */
#ifdef GLOB_ABORTED  /* not present in older gcc */
    } else if (glob_ret == GLOB_ABORTED) {
        fdprintf(out, "Error; Read error\r\n");
	return 0;
#endif /* GLOB_ABORTED */
    } else if (glob_ret != 0) {
        fdprintf(out, "Error; Unknown glob() error %d\r\n", glob_ret);
	return 0;
    }

    /* print our results */
    for (i=0; i<glob_buf.gl_pathc; i++) {
        file_name = glob_buf.gl_pathv[i];
	if (memcmp(file_name, pattern, dir_len) == 0) {
	    file_name += dir_len;
	}
	fdprintf(out, "%s\r\n", file_name);
    }

    /* free and return */
    globfree(&glob_buf);
    return 1;
}

typedef struct {
    char *name;
    char *full_path;
    struct stat stat;
} file_info_t;

int file_list(int out, const char *cur_dir, const char *filespec)
{
    int dir_len;
    char pattern[PATH_MAX+1];
    int glob_ret;
    glob_t glob_buf;
    int i;
    file_info_t *file_info;
    int num_files;
    unsigned long total_blocks;
    char *file_name;

    mode_t mode;
    time_t now;
    struct tm tm_now;
    double age;
    char date_buf[13];
    char a_link[PATH_MAX+1];
    int link_len;
    
    daemon_assert(out >= 0);
    daemon_assert(is_valid_dir(cur_dir));
    daemon_assert(filespec != NULL);

    filespec = skip_ls_options(filespec);

    if (filespec[0] == '/') {
        cur_dir = "";
        dir_len = 0;
    } else {
        strcpy(pattern, cur_dir);
	if ((cur_dir[0] != '/') || (cur_dir[1] != '\0')) {
            strcat(pattern, "/");
	}
        dir_len = strlen(pattern);
    }

    /* make sure we have enough space */
    if ((dir_len + 1 + strlen(filespec)) > PATH_MAX) {
        fdprintf(out, "Error; Path name too long\r\n");
	return 0;
    }
    strcat(pattern, filespec);

    /* do a glob() */
    memset(&glob_buf, 0, sizeof(glob_buf));
    glob_ret = glob(pattern, GLOB_ERR, NULL, &glob_buf);
#ifndef GLOB_NOMATCH /* FreeBSD */
    if (glob_ret == GLOB_NOCHECK) {
#else
    if (glob_ret == GLOB_NOMATCH) {
#endif
        fdprintf(out, "total 0\r\n");
        return 1;
    } else if (glob_ret == GLOB_NOSPACE) {
        fdprintf(out, "Error; Out of memory\r\n");
	return 0;
#ifdef GLOB_ABORTED  /* not present in older gcc */
    } else if (glob_ret == GLOB_ABORTED) {
        fdprintf(out, "Error; Read error\r\n");
	return 0;
#endif /* GLOB_ABORTED */
    } else if (glob_ret != 0) {
        fdprintf(out, "Error; Unknown glob() error %d\r\n", glob_ret);
	return 0;
    }

    /* make a buffer to store our information */
#ifdef HAVE_ALLOCA
    file_info = (file_info_t *)alloca(sizeof(file_info_t) * glob_buf.gl_pathc);
#else
    file_info = (file_info_t *)malloc(sizeof(file_info_t) * glob_buf.gl_pathc);
#endif
    if (file_info == NULL) {
        fdprintf(out, "Error; Out of memory\r\n");
	globfree(&glob_buf);
	return 0;
    }

    /* collect information */
    num_files = 0;
    total_blocks = 0;
    for (i=0; i<glob_buf.gl_pathc; i++) {
        file_name = glob_buf.gl_pathv[i];
	if (memcmp(file_name, pattern, dir_len) == 0) {
	    file_name += dir_len;
	}
	if (lstat(glob_buf.gl_pathv[i], &file_info[num_files].stat) == 0) {
#ifdef AC_STRUCT_ST_BLKSIZE
	    total_blocks += file_info[num_files].stat.st_blocks;
#endif
	    file_info[num_files].name = file_name;
	    file_info[num_files].full_path = glob_buf.gl_pathv[i];
	    num_files++;
	}
    }

    /* okay, we have information, now display it */
    fdprintf(out, "total %lu\r\n", total_blocks);
    time(&now);
    for (i=0; i<num_files; i++) {

	mode = file_info[i].stat.st_mode;

        /* output file type */
	switch (mode & S_IFMT) {
	    case S_IFSOCK:  fdprintf(out, "s"); break;
	    case S_IFLNK:   fdprintf(out, "l"); break;
	    case S_IFBLK:   fdprintf(out, "b"); break;
	    case S_IFDIR:   fdprintf(out, "d"); break;
	    case S_IFCHR:   fdprintf(out, "c"); break;
	    case S_IFIFO:   fdprintf(out, "p"); break;
	    default:        fdprintf(out, "-"); 
	}

	/* output permissions */
	fdprintf(out, (mode & S_IRUSR) ? "r" : "-");
	fdprintf(out, (mode & S_IWUSR) ? "w" : "-");
	if (mode & S_ISUID) { 
	    fdprintf(out, (mode & S_IXUSR) ? "s" : "S");
	} else {
	    fdprintf(out, (mode & S_IXUSR) ? "x" : "-");
	}
	fdprintf(out, (mode & S_IRGRP) ? "r" : "-");
	fdprintf(out, (mode & S_IWGRP) ? "w" : "-");
	if (mode & S_ISGID) { 
	    fdprintf(out, (mode & S_IXGRP) ? "s" : "S");
	} else {
	    fdprintf(out, (mode & S_IXGRP) ? "x" : "-");
	}
	fdprintf(out, (mode & S_IROTH) ? "r" : "-");
	fdprintf(out, (mode & S_IWOTH) ? "w" : "-");
	if (mode & S_ISVTX) {
	    fdprintf(out, (mode & S_IXOTH) ? "t" : "T");
	} else {
	    fdprintf(out, (mode & S_IXOTH) ? "x" : "-");
	}

        /* output link & ownership information */
	fdprintf(out, " %3d %-8d %-8d ", 
	    file_info[i].stat.st_nlink, 
	    file_info[i].stat.st_uid, 
	    file_info[i].stat.st_gid);

        /* output either i-node information or size */
#ifdef AC_STRUCT_ST_RDEV
	if (((mode & S_IFMT) == S_IFBLK) || ((mode & S_IFMT) == S_IFCHR)) {
	    fdprintf(out, "%3d, %3d ", 
	        (int)((file_info[i].stat.st_rdev >> 8) & 0xff),
	        (int)(file_info[i].stat.st_rdev & 0xff));
	} else {
	    fdprintf(out, "%8lu ", 
	        (unsigned long)file_info[i].stat.st_size);
	}
#else
	fdprintf(out, "%8lu ", (unsigned long)file_info[i].stat.st_size);
#endif
	    
        /* output date */
	localtime_r(&file_info[i].stat.st_mtime, &tm_now);
        age = difftime(now, file_info[i].stat.st_mtime);
	if ((age > 60 * 60 * 24 * 30 * 6) || (age < -(60 * 60 * 24 * 30 * 6))) {
	    strftime(date_buf, sizeof(date_buf), "%b %e  %Y", &tm_now);
	} else {
	    strftime(date_buf, sizeof(date_buf), "%b %e %H:%M", &tm_now);
	}
        fdprintf(out, "%s ", date_buf);

	/* output filename */
	fdprintf(out, "%s", file_info[i].name);
  
        /* display symbolic link information */
	if ((mode & S_IFMT) == S_IFLNK) {
	    link_len = readlink(file_info[i].full_path, a_link, sizeof(a_link));
	    if (link_len > 0) {
	        fdprintf(out, " -> ");
	        a_link[link_len] = '\0';
	        fdprintf(out, "%s", link);
	    }
	}

	/* advance to next line */
	fdprintf(out, "\r\n");
    }

    /* free memory & return */
#ifndef HAVE_ALLOCA
    free(file_info);
#endif 
    globfree(&glob_buf);
    return 1;
}

static int is_valid_dir(const char *dir)
{
    /* directory can not be NULL (of course) */
    if (dir == NULL) {
        return 0;
    }
 
    /* directory must be absolute (i.e. start with '/') */
    if (dir[0] != '/') {
        return 0;
    }

    /* length cannot be greater than PATH_MAX */
    if (strlen(dir) > PATH_MAX) {
        return 0;
    }

    /* assume okay */
    return 1;
}

static void fdprintf(int fd, const char *fmt, ...)
{
    char buf[PATH_MAX+1];
    int buflen;
    va_list ap;
    int amt_written;
    int write_ret;

    daemon_assert(fd >= 0);
    daemon_assert(fmt != NULL);

    va_start(ap, fmt);
    buflen = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (buflen <= 0) {
        return;
    }
    if (buflen >= sizeof(buf)) {
        buflen = sizeof(buf)-1;
    }

    amt_written = 0;
    while (amt_written < buflen) {
        write_ret = write(fd, buf+amt_written, buflen-amt_written);
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
const char *
skip_ls_options(const char *filespec)
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

