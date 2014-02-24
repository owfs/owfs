/*
$Id$
   OWFS (owfs, owhttpd, owserver, owperl, owtcl, owphp, owpython, owcapi)
   one-wire file system and related programs

    By Paul H Alfille
    {c} 2003-5 GPL
    paul.alfille@gmail.com
*/

/* OWCAPI - specific header */
/* OWCAPI is the simple C library for OWFS */
/* Commands OW_init, OW_get, OW_put, OW_finish */

#ifndef OWCAPI_H
#define OWCAPI_H

#include <owfs_config.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* initialization, required before any other calls. Should be paired with a finish
   OW_init -- simplest, just a device name
              /dev/ttyS0 for serial
              u or u# for USB
              #### for TCP/IP port (owserver)
              looks just like the command line to owfs or owhttpd
   OW_init_args -- char** array usually from the main() call

   return value = 0 good
               < 0 error
  
  No need to call OW_finish if an error
*/
	ssize_t OW_init(const char *params);
	ssize_t OW_init_args(int argc, char **args);

/* repeat initialization can be the true init, or called safely a second time
 * this allows init to be called more than once, but the parameters are ignored after the first one.
   OW_safe_init -- simplest, just a device name
              /dev/ttyS0 for serial
              u or u# for USB
              #### for TCP/IP port (owserver)
              looks just like the command line to owfs or owhttpd
   OW_safe_init_args -- char** array usually from the main() call

   return value = 0 good
               < 0 error
  
  No need to call OW_finish if an error
*/
	ssize_t OW_safe_init(const char *params);
	ssize_t OW_safe_init_args(int argc, char **args);

	void OW_set_error_level(const char *params);
	void OW_set_error_print(const char *params);

	/* OW_get -- data read or directory read
	   path is OWFS style name,
	   "" or "/" for root directory
	   "01.23456708ABDE" for device directory
	   "10.468ACE13579B/temperature for a specific device property

	   buffer is a char buffer that is allocated by OW_get.
	   buffer MUST BE "free"ed after use. 
	   buffer_length, if not NULL, will be assigned the length of the returned data

	   If path is NULL, it is assumed to be "/" the root directory
	   If path is not a valid C string, the results are unpredictable.

	   If buffer is NULL, an error is returned
	   If buffer_length is NULL it is ignored

	   return value >=0 ok, length of information returned (in bytes)
	   <0 error
	 */
	ssize_t OW_get(const char *path, char **buffer, size_t * buffer_length);

	/* OW_present -- check if path is present
	   path is OWFS style name,
	   "" or "/" for root directory
	   "01.23456708ABDE" for device directory
	   "10.468ACE13579B/temperature for a specific device property

	   return value = 0 ok
                    < 0 error
	 */
	int OW_present(const char *path);

/* OW_put -- data write
  path is OWFS style name,
    "05.468ACE13579B/PIO.A for a specific device property
  buffer holds the value
    ascii, possibly comma delimitted

  Note NULL path or buffer will return an error.
  Note: path must be null-terminated
  Note: buffer_length will be used for length, there is no requirement that buffer be null-terminated
  
  return value  = 0 ok
                < 0 error
*/
	ssize_t OW_put(const char *path, const char *buffer, size_t buffer_length);

/*  OW_lread -- read data with offset
  path is OWFS style name,
    "05.468ACE13579B/PIO.A for a specific device property
  buffer holds the value
    ascii, possibly comma delimitted

    buffer must be size long
    offset is from start of value
    only ascii and binary data appropriate
*/
	ssize_t OW_lread(const char *path, char *buf, const size_t size, const off_t offset);

/*  OW_lwrite -- write data with offset
  path is OWFS style name,
    "05.468ACE13579B/PIO.A for a specific device property
  buffer holds the value
    ascii, possibly comma delimitted

    buffer must be size long
    offset is from start of value
    only ascii and binary data appropriate
*/
	ssize_t OW_lwrite(const char *path, const char *buf, const size_t size, const off_t offset);

/* cleanup
  Clears internal buffer, frees file descriptors
  Normal process cleanup will work if program ends before OW_finish is called
  But not calling OW_init more than once without an intervening OW_finish will cause a memory leak
  No error return
*/
	void OW_finish(void);

#ifdef __cplusplus
}
#endif
#endif							/* OWCAPI_H */
