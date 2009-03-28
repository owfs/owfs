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

    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    ---------------------------------------------------------------------------
    Implementation:
    2006 dirblob
*/

// Define for full memmory allocation debugging
// #define OW_ALLOC_DEBUG

#ifndef OW_ALLOC_H			/* tedious wrapper */
#define OW_ALLOC_H

#ifndef OW_ALLOC_DEBUG
	#define owmalloc(size)        malloc(size)
	#define owcalloc(nmemb,size)  calloc(nmemb,size)
	#define owfree(ptr)           free(ptr)
	#define owrealloc(ptr,size)   realloc(ptr,size)
	#define owstrdup(s)           strdup(s)
#else  /* OW_ALLOC_DEBUG */
	#define owmalloc(size)        OWmalloc(__FILE__,__LINE__,__func__,size)
	#define owcalloc(nmemb,size)  OWcalloc(__FILE__,__LINE__,__func__,nmemb,size)
	#define owfree(ptr)           OWfree(__FILE__,__LINE__,__func__,ptr)
	#define owrealloc(ptr,size)   OWrealloc(__FILE__,__LINE__,__func__,ptr,size)
	#define owstrdup(s)           OWstrdup(__FILE__,__LINE__,__func__,s)
	void *OWcalloc(const char * file, int line, const char * func, size_t nmemb, size_t size);
	void *OWmalloc(const char * file, int line, const char * func, size_t size);
	void  OWfree(const char * file, int line, const char * func, void *ptr);
	void *OWrealloc(const char * file, int line, const char * func, void *ptr, size_t size);
	char *OWstrdup(const char * file, int line, const char * func, const char *s);
#endif  /* OW_ALLOC_DEBUG */

#endif							/* OW_ALLOC_H */
