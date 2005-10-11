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

/* initialization, required befopre any other calls. Should be paired with a finish
    OW_init -- simplest, just a device name
              /dev/ttyS0 for serial
              u or u# for USB
              #### for TCP/IP port (owserver)
   OW_init_string -- looks just like the command line to owfs or owhttpd
   OW_init_args -- char** array usually from the main() call

  return value = 0 good
               < 0 error
  
  No need to call OW_finish if an error */
 
int OW_init(        const char * device ) ;
int OW_init_string( const char * params) ;
int OW_init_args(   int argc, char ** args ) ;

/* data read or directory read
  name is OWFS style name,
    "" or "/" for root directory
    "01.23456708ABDE" for device directory
    "10.468ACE13579B/temperature for a specific device property
  buffer holds the result
    ascii, possibly comma delimitted

  Note: NULL path assumed to be root directory, not an error      
  Note: path must be null-terminated if not null
  Note: NULL buffer will return an error
  Note: buffer will never be overrun -- buffer_length will be honored
  
  return value >=0 ok, length of information used
                <0 error
*/
int OW_get( const char * path, char * buffer, size_t buffer_length ) ;

/* data write
  name is OWFS style name,
    "05.468ACE13579B/PIO.A for a specific device property
  buffer holds the value
    ascii, possibly comma delimitted

  Note NULL path or buffer will return an error.
  Note: path must be null-terminated
  Note: buffer_length will be used for length, there is no requirement that buffer be null-terminated
  
  return value >= 0 length of returned data string
                < 0 error
*/ 
int OW_put( const char * path, const char * buffer, size_t buffer_length ) ;

/* cleanup
  Clears internal buffer, frees file descriptors
  Notmal process cleanup will work if program ends before OW_finish is called
  But not calling OW_init more than once without an intervening OW_finish will cause a memory leak
  No error return
*/
void OW_finish( void ) ;

#endif /* OWCAPI_H */
