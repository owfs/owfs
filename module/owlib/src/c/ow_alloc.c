/*
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
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#if OW_ALLOC_DEBUG
/* Special routines for tracking memory leakage */
/* Turned on by
 * ./configure --enable-owmalloc
 * */

void *OWcalloc(const char * file, int line, const char * func, size_t nmemb, size_t size)
{
	void * v = calloc(nmemb,size);
	printf("%p alloc %s:%s[%d] CALLOC size=%d nmemb=%d\n",v,file,func,line,(int)size,(int)nmemb);
	return v;
}

void *OWmalloc(const char * file, int line, const char * func, size_t size)
{
	void * v = malloc(size);
	printf("%p alloc %s:%s[%d] MALLOC size=%d\n",v,file,func,line,(int)size);
	return v;
}

void  OWfree(const char * file, int line, const char * func, void *ptr)
{
	printf("%p free  %s:%s[%d] FREE\n",ptr,file,func,line);
	free(ptr);
}

void  OWtreefree(void *ptr)
{
	OWfree("BinaryTree", 0, "tdestroy", ptr) ;
}

void *OWrealloc(const char * file, int line, const char * func, void *ptr, size_t size)
{
	void * v = realloc(ptr,size);
	printf("%p free  %s:%s[%d] REALLOC\n",ptr,file,func,line);
	printf("%p alloc %s:%s[%d] REALLOC size=%d\n",v,file,func,line,(int)size);
	return v;
}

char *OWstrdup(const char * file, int line, const char * func, const char *s)
{
	char * c = strdup(s);
	printf("%p alloc %s:%s[%d] STRDUP s=%s\n",c,file,func,line,s);
	return c;
}

#endif							/* OW_ALLOC_H */
