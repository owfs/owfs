/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

//#include "compat.h"


#ifndef HAVE_STRSEP
/*-
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
 /*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char *
strsep(char **stringp, const char *delim)
{
  char *s;
  const char *spanp;
  int c, sc;
  char *tok;
 
  if ((s = *stringp) == NULL)
    return (NULL);
  for (tok = s;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
                                else
				  s[-1] = 0;
	*stringp = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}
 
#endif /* !defined(HAVE_STRSEP) */


#ifndef HAVE_TDESTROY
/*
  uClibc older than 0.9.19 is missing tdestroy() (don't know exactly when
  it was added) I added a replacement to this, just to be able to compile
  owfs for WRT54G without any patched uClibc.
*/

typedef struct node_t {
    void        *key;
    struct node_t *left, *right;
} node;

static void tdestroy_recurse_ (node *root, void *freefct) {
    if (root->left != NULL) {
        tdestroy_recurse_ (root->left, freefct);
#ifdef DELETE_KEY
	free(root->left);
	root->left = NULL;
#endif
    }
    if (root->right != NULL) {
        tdestroy_recurse_ (root->right, freefct);
#ifdef DELETE_KEY
	free(root->right);
	root->right = NULL;
#endif
    }
    //(*freefct) ((void *) root->key);
#ifdef DELETE_KEY
    if(root->key) {
      free(root->key);
      root->key = NULL;
    }
#endif
}

void tdestroy(void *vroot, void *freefct) {
    node *root = (node *) vroot;
    if (root != NULL) {
        tdestroy_recurse_ (root, freefct);
#ifdef DELETE_KEY
	/* Free the node itself.  */
	free (root);
#endif
    }
}
#endif /* HAVE_TDESTROY */



