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
char *strsep(char **stringp, const char *delim)
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

#endif							/* !defined(HAVE_STRSEP) */


#ifndef HAVE_TDESTROY
/*
  uClibc older than 0.9.19 is missing tdestroy() (don't know exactly when
  it was added) I added a replacement to this, just to be able to compile
  owfs for WRT54G without any patched uClibc.
*/

#ifndef NODE_T_DEFINED
#define NODE_T_DEFINED
typedef struct node_t {
	void *key;
	struct node_t *left, *right;
} node;
#endif

static void tdestroy_recurse_(node * root, void (*freefct) (void *))
{
	if (root->left != NULL)
		tdestroy_recurse_(root->left, freefct);
	if (root->right != NULL)
		tdestroy_recurse_(root->right, freefct);
	if (root->key) {
		(*freefct) ((void *) root->key);
		//free(root->key);
		root->key = NULL;
	}
	/* Free the node itself.  */
	free(root);
}

void tdestroy(void *vroot, void (*freefct) (void *))
{
	node *root = (node *) vroot;
	if (root != NULL) {
		tdestroy_recurse_(root, freefct);
	}
}
#endif							/* HAVE_TDESTROY */

#ifndef HAVE_TSEARCH
#ifndef NODE_T_DEFINED
#define NODE_T_DEFINED
typedef struct node_t {
	void *key;
	struct node_t *left, *right;
} node;
#endif

/* find or insert datum into search tree.
char 	*key;			 key to be located
register node	**rootp;	 address of tree root
int	(*compar)();		 ordering function
*/

void *tsearch(__const void *key, void **vrootp, __compar_fn_t compar)
{
	register node *q;
	register node **rootp = (node **) vrootp;

	if (rootp == (struct node_t **) 0)
		return ((struct node_t *) 0);
	while (*rootp != (struct node_t *) 0) {	/* Knuth's T1: */
		int r;

		if ((r = (*compar) (key, (*rootp)->key)) == 0)	/* T2: */
			return (*rootp);	/* we found it! */
		rootp = (r < 0) ? &(*rootp)->left :	/* T3: follow left branch */
			&(*rootp)->right;	/* T4: follow right branch */
	}
	q = (node *) malloc(sizeof(node));	/* T5: key not found */
	if (q != (struct node_t *) 0) {	/* make new node */
		*rootp = q;				/* link new node to old */
		q->key = (void *) key;	/* initialize new node */
		q->left = q->right = (struct node_t *) 0;
	}
	return (q);
}
#endif

#ifndef HAVE_TFIND
#ifndef NODE_T_DEFINED
#define NODE_T_DEFINED
typedef struct node_t {
	void *key;
	struct node_t *left, *right;
} node;
#endif

void *tfind(__const void *key, void *__const * vrootp, __compar_fn_t compar)
{
	register node **rootp = (node **) vrootp;

	if (rootp == (struct node_t **) 0)
		return ((struct node_t *) 0);
	while (*rootp != (struct node_t *) 0) {	/* Knuth's T1: */
		int r;

		if ((r = (*compar) (key, (*rootp)->key)) == 0)	/* T2: */
			return (*rootp);	/* we found it! */
		rootp = (r < 0) ? &(*rootp)->left :	/* T3: follow left branch */
			&(*rootp)->right;	/* T4: follow right branch */
	}
	return NULL;
}
#endif

#ifndef HAVE_TFIND
#ifndef NODE_T_DEFINED
#define NODE_T_DEFINED
typedef struct node_t {
	void *key;
	struct node_t *left, *right;
} node;
#endif
/* delete node with given key
char	*key;			key to be deleted
register node	**rootp;	address of the root of tree
int	(*compar)();		comparison function
*/
void *tdelete(__const void *key, void **vrootp, __compar_fn_t compar)
{
	node *p;
	register node *q;
	register node *r;
	int cmp;
	register node **rootp = (node **) vrootp;

	if (rootp == (struct node_t **) 0 || (p = *rootp) == (struct node_t *) 0)
		return ((struct node_t *) 0);
	while ((cmp = (*compar) (key, (*rootp)->key)) != 0) {
		p = *rootp;
		rootp = (cmp < 0) ? &(*rootp)->left :	/* follow left branch */
			&(*rootp)->right;	/* follow right branch */
		if (*rootp == (struct node_t *) 0)
			return ((struct node_t *) 0);	/* key not found */
	}
	r = (*rootp)->right;		/* D1: */
	if ((q = (*rootp)->left) == (struct node_t *) 0)	/* Left (struct node_t *)0? */
		q = r;
	else if (r != (struct node_t *) 0) {	/* Right link is null? */
		if (r->left == (struct node_t *) 0) {	/* D2: Find successor */
			r->left = q;
			q = r;
		} else {				/* D3: Find (struct node_t *)0 link */
			for (q = r->left; q->left != (struct node_t *) 0; q = r->left)
				r = q;
			r->left = q->right;
			q->left = (*rootp)->left;
			q->right = (*rootp)->right;
		}
	}
	free((struct node_t *) *rootp);	/* D4: Free node */
	*rootp = q;					/* link parent to new node */
	return (p);
}
#endif

#ifndef HAVE_TWALK
#ifndef NODE_T_DEFINED
#define NODE_T_DEFINED
typedef struct node_t {
	void *key;
	struct node_t *left, *right;
} node;
#endif
/* Walk the nodes of a tree
register node	*root;		Root of the tree to be walked
register void	(*action)();	Function to be called at each node
register int	level;
*/
static void trecurse(__const void *vroot, __action_fn_t action, int level)
{
	register node *root = (node *) vroot;

	if (root->left == (struct node_t *) 0 && root->right == (struct node_t *) 0)
		(*action) (root, leaf, level);
	else {
		(*action) (root, preorder, level);
		if (root->left != (struct node_t *) 0)
			trecurse(root->left, action, level + 1);
		(*action) (root, postorder, level);
		if (root->right != (struct node_t *) 0)
			trecurse(root->right, action, level + 1);
		(*action) (root, endorder, level);
	}
}

/* void twalk(root, action)		Walk the nodes of a tree 
node	*root;			Root of the tree to be walked
void	(*action)();		Function to be called at each node
PTR
*/
void twalk(__const void *vroot, __action_fn_t action)
{
	register __const node *root = (node *) vroot;

	if (root != (node *) 0 && action != (__action_fn_t) 0)
		trecurse(root, action, 0);
}
#endif
