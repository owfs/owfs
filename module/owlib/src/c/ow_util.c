/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/*
 * HEXVALUE() is defined in <readline/chardefs.h>, but since it's more
 * common with 0-9 I changed compare-order in the define
 */
#define _HEXVALUE(c) \
  (((c) >= '0' && (c) <= '9') \
        ? (c)-'0' \
        : (c) >= 'A' && (c) <= 'F' ? (c)-'A'+10 : (c)-'a'+10 )

/* 2 hex digits to number. This is the most used function in owfs for the LINK */
BYTE string2num(const char *s)
{
	if (s == NULL) {
		return 0;
	}
	return (_HEXVALUE(s[0]) << 4) + _HEXVALUE(s[1]);
}


//#define _TOHEXCHAR(c) (((c) >= 0 && (c) <= 9) ? '0'+(c) : 'A'+(c)-10 )
// we only use a nibble
#define _TOHEXCHAR(c) (((c) <= 9) ? '0'+(c) : ('A'-10)+(c) )

#if 0
/* number to a hex digit */
char num2char(const BYTE n)
{
	return _TOHEXCHAR(n);
}
#endif

/* number to 2 hex digits */
void num2string(char *s, const BYTE n)
{
	s[0] = _TOHEXCHAR(n >> 4);
	s[1] = _TOHEXCHAR(n & 0x0F);
}

/* 2x hex digits to x number bytes */
void string2bytes(const char *str, BYTE * b, const int bytes)
{
	int i;
	for (i = 0; i < bytes; ++i) {
		b[i] = string2num(&str[i << 1]);
	}
}

/* number(x bytes) to 2x hex digits */
void bytes2string(char *str, const BYTE * b, const int bytes)
{
	int i;
	for (i = 0; i < bytes; ++i) {
		num2string(&str[i << 1], b[i]);
	}
}

void UT_fromDate(const _DATE d, BYTE * data)
{
	data[0] = BYTE_MASK(d);
	data[1] = BYTE_MASK(d >> 8);
	data[2] = BYTE_MASK(d >> 16);
	data[3] = BYTE_MASK(d >> 24);
}

_DATE UT_toDate(const BYTE * data)
{
	return (_DATE) UT_uint32(data);
}

void Test_and_Close( FILE_DESCRIPTOR_OR_ERROR * file_descriptor )
{
	if ( file_descriptor == NULL ) {
		return ;
	}
	if ( FILE_DESCRIPTOR_VALID( *file_descriptor ) ) {
		close( *file_descriptor ) ;
	}
	*file_descriptor = FILE_DESCRIPTOR_BAD ;
}

void Test_and_Close_Pipe( FILE_DESCRIPTOR_OR_ERROR * pipe_fd )
{
	Test_and_Close( &pipe_fd[fd_pipe_read]) ;
	Test_and_Close( &pipe_fd[fd_pipe_write]) ;
}

void Init_Pipe( FILE_DESCRIPTOR_OR_ERROR * pipe_fd )
{
	pipe_fd[fd_pipe_read] =
	pipe_fd[fd_pipe_write] =
	FILE_DESCRIPTOR_BAD ;
}
