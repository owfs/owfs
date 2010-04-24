/*
$Id$
     OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions
    COM - serial port functions
    DS2480 -- DS9097 serial connector

    Written 2003 Paul H Alfille
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "owcapi.h"
#include <limits.h>

#if OW_MT
pthread_t main_threadid;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else							/* OW_MT */
#define IS_MAINTHREAD 1
#endif							/* OW_MT */

#define MAX_ARGS 20

static ssize_t ReturnAndErrno(ssize_t ret)
{
	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		errno = 0;
		return ret;
	}
}

void OW_set_error_print(const char *params)
{
	API_set_error_print(params);
}

void OW_set_error_level(const char *params)
{
	API_set_error_level(params);
}

ssize_t OW_init(const char *params)
{
	/* Set up owlib */
	API_setup(opt_c);

	/* Proceed with init while lock held */
	/* grab our executable name */
	Globals.progname = owstrdup("OWCAPI");

	if ( BAD( API_init(params) ) ) {
		API_finish();
		return ReturnAndErrno(-EINVAL) ;
	}

	return ReturnAndErrno(0);
}

ssize_t OW_init_args(int argc, char **argv)
{
	/* Set up owlib */
	API_setup(opt_c);

	/* grab our executable name */
	if (argc > 0) {
		Globals.progname = owstrdup(argv[0]);
	}

	// process the command line flags
	do {
		int c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL) ;
		if ( c == -1 ) {
			break ;
		}
		if ( BAD( owopt(c, optarg) ) ) {
			// clean up and exit
			API_finish() ;
			return ReturnAndErrno( -EINVAL ) ;
		}
	} while ( 1 ) ;

	/* non-option arguments */
	while (optind < argc) {
		if ( BAD( ARG_Generic(argv[optind]) ) ) {
			// clean up and exit
			API_finish() ;
			return ReturnAndErrno( -EINVAL ) ;
		}
		++optind;
	}

	if ( BAD( API_init(NULL) ) ) {
		// clean up and exit
		API_finish() ;
		return ReturnAndErrno( -EINVAL ) ;
	}

	return ReturnAndErrno(0);
}

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */

ssize_t OW_get(const char *path, char **return_buffer, size_t * buffer_length)
{
	SIZE_OR_ERROR size_or_error = -EACCES ;		/* current buffer string length */

	if (API_access_start() == 0) {	/* Check for prior init */
		size_or_error = FS_get( path, return_buffer, buffer_length );
		API_access_end();
	}
	return ReturnAndErrno(size_or_error);
}

int OW_present(const char *path)
{
	ssize_t ret = -ENOENT;		/* current buffer string length */

	if (API_access_start() == 0) {	/* Check for prior init */
		struct parsedname s_pn ;
		if (FS_ParsedName(path, &s_pn) != 0) {	/* Can we parse the path string */
			ret = -ENOENT;
		} else {
			FS_ParsedName_destroy(&s_pn);
		}
		API_access_end();
	}
	return ReturnAndErrno(ret);
}

ssize_t OW_lread(const char *path, char *buffer, const size_t size, const off_t offset)
{
	ssize_t ret = -EACCES;		/* current buffer string length */

	/* Check the parameters */
	if (buffer == NULL || path == NULL) {
		return ReturnAndErrno(-EINVAL);
	}

	if (API_access_start() == 0) {
		ret = FS_read(path, buffer, size, offset);
		API_access_end();
	}
	return ReturnAndErrno(ret);
}

ssize_t OW_lwrite(const char *path, const char *buffer, const size_t size, const off_t offset)
{
	ssize_t ret = -EACCES;		/* current buffer string length */

	/* Check the parameters */
	if (buffer == NULL || path == NULL) {
		return ReturnAndErrno(-EINVAL);
	}

	if (API_access_start() == 0) {
		ret = FS_write(path, buffer, size, offset);
		API_access_end();
	}
	return ReturnAndErrno(ret);
}

ssize_t OW_put(const char *path, const char *buffer, size_t buffer_length)
{
	ssize_t ret = -EACCES;

	/* Check the parameters */
	if (buffer == NULL || path == NULL) {
		return ReturnAndErrno(-EINVAL);
	}

	/* Check that we have done init, and that we can access */
	if (API_access_start() == 0) {
		ret = FS_write(path, buffer, buffer_length, 0);
		API_access_end();
	}
	return ReturnAndErrno(ret);
}

void OW_finish(void)
{
	API_finish();
}
