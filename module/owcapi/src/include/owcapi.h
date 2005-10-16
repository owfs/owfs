/*
$Id$
   OWFS (owfs, owhttpd, owserver, owperl, owtcl, owphp, owpython, owcapi)
   one-wire file system and related programs

    By Paul H Alfille
    {c} 2003-5 GPL
    palfille@earthlink.net
*/

/* OWCAPI - specific header */
/* OWCAPI is the simple C library for OWFS */
/* Commands OW_init, OW_get, OW_put, OW_finish */

#ifndef OWCAPI_H
#define OWCAPI_H

/* initialization, required before any other calls. Should be paired with a finish
    OW_init -- simplest, just a device name
              /dev/ttyS0 for serial
              u or u# for USB
              #### for TCP/IP port (owserver)
   OW_init_string -- looks just like the command line to owfs or owhttpd
   OW_init_args -- char** array usually from the main() call

  return value = 0 good
               < 0 error
  
  No need to call OW_finish if an error
*/ 
int OW_init(        const char * device ) ;
int OW_init_string( const char * params) ;
int OW_init_args(   int argc, char ** args ) ;

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
int OW_get( const char * path, char ** buffer, size_t * buffer_length ) ;

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
int OW_put( const char * path, const char * buffer, size_t buffer_length ) ;

/* cleanup
  Clears internal buffer, frees file descriptors
  Normal process cleanup will work if program ends before OW_finish is called
  But not calling OW_init more than once without an intervening OW_finish will cause a memory leak
  No error return
*/
void OW_finish( void ) ;

#endif /* OWCAPI_H */
