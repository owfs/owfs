/* getline.c -- Replacement for GNU C library function getline

Copyright (C) 1993 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Written by Jan Brittenson, bson@gnu.ai.mit.edu.  */
/* modified very slightly for OWFS proect  by Paul Alfille
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#ifdef WE_NEED_GETLINE

#include <sys/types.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/* Read up to (and including) a TERMINATOR from STREAM into *LINEPTR
   + OFFSET (and null-terminate it). *LINEPTR is a pointer returned from
   malloc (or NULL), pointing to *N characters of space.  It is realloc'd
   as necessary.  Return the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

int getstr (char ** lineptr, size_t *n, FILE * stream, char terminator, int offset)
{
  int nchars_avail;             /* Allocated but unused chars in *LINEPTR.  */
  char *read_pos;               /* Where we're reading into *LINEPTR. */
  int ret;

  if (!lineptr || !n || !stream)
    return -1;

  if (!*lineptr)
    {
      *n = 64;
      *lineptr = (char *) malloc (*n);
      if (!*lineptr)
        return -1;
    }

  nchars_avail = *n - offset;
  read_pos = *lineptr + offset;

  for (;;)
    {
      register int c = getc (stream);

      /* We always want at least one char left in the buffer, since we
         always (unless we get an error while reading the first char)
         NUL-terminate the line buffer.  */

      assert(*n - nchars_avail == read_pos - *lineptr);
      if (nchars_avail < 1)
        {
          if (*n > 64)
            *n *= 2;
          else
            *n += 64;

          nchars_avail = *n + *lineptr - read_pos;
          *lineptr = (char *) realloc (*lineptr, *n);
          if (!*lineptr)
            return -1;
          read_pos = *n - nchars_avail + *lineptr;
          assert(*n - nchars_avail == read_pos - *lineptr);
        }

      if (c == EOF || ferror (stream))
        {
          /* Return partial line, if any.  */
          if (read_pos == *lineptr)
            return -1;
          else
            break;
        }

      *read_pos++ = c;
      nchars_avail--;

      if (c == terminator)
        /* Return the line.  */
        break;
    }

  /* Done - NUL terminate and return the number of chars read.  */
  *read_pos = '\0';

  ret = read_pos - (*lineptr + offset);
  return ret;
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
  return getstr (lineptr, n, stream, '\n', 0);
}

#endif /* WE_NEED_GETLINE */
