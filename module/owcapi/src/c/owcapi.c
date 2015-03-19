/*
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

#define MAX_ARGS 20

static ssize_t OW_init_both(const char *params, enum restart_init repeat) ;
static ssize_t OW_init_args_both(int argc, char **argv, enum restart_init repeat);

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
	return OW_init_both( params, restart_if_repeat ) ;
}

ssize_t OW_safe_init(const char *params)
{
	return OW_init_both( params, continue_if_repeat ) ;
}

static ssize_t OW_init_both(const char *params, enum restart_init repeat)
{
	/* Set up owlib */
	API_setup(program_type_clibrary);

	if ( BAD( API_init(params, repeat) ) ) {
		API_finish();
		return ReturnAndErrno(-EINVAL) ;
	}

	return ReturnAndErrno(0);
}

ssize_t OW_init_args(int argc, char **argv)
{
	return OW_init_args_both( argc, argv, restart_if_repeat ) ;
}

ssize_t OW_safe_init_args(int argc, char **argv)
{
	return OW_init_args_both( argc, argv, continue_if_repeat ) ;
}

static ssize_t OW_init_args_both(int argc, char **argv, enum restart_init repeat)
{
	/* Set up owlib */
	API_setup(program_type_clibrary);

	if ( BAD( API_init_args( argc, argv, repeat) ) ) {
		API_finish();
		return ReturnAndErrno(-EINVAL) ;
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
			ret = 0 ; // good result
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
