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
    COM - serial port functions
    DS2480 -- DS9097 serial connector

    Written 2003 Paul H Alfille
*/

#include "owfs.h"

/* Fuse_main is called with a standard C-style argc / argv parameters */

/* These routines build the char **argv */

int Fuse_setup(struct Fuse_option *fo)
{
	int i;
	fo->max_options = 10;
	fo->argc = 0;
	fo->argv = (char **) owcalloc(fo->max_options + 1, sizeof(char *));
	if (fo->argv == NULL)
		return -ENOMEM;
	for (i = 0; i <= fo->max_options; ++i)
		fo->argv[i] = NULL;
	/* create "0th" item -- the program name */
	return Fuse_add("OWFS", fo);
}

void Fuse_cleanup(struct Fuse_option *fo)
{
	int i;
	if (fo->argv) {
		for (i = 0; i < fo->max_options; ++i)
			if (fo->argv[i])
				owfree(fo->argv[i]);
		owfree(fo->argv);
	}
}

int Fuse_parse(char *opts, struct Fuse_option *fo)
{
	char *start;
	char *next = opts;
	while ((start = next)) {
		strsep(&next, " ");
		if (Fuse_add(start, fo))
			return 1;
	}
	return 0;
}

int Fuse_add(char *opt, struct Fuse_option *fo)
{
	//LEVEL_DEBUG("Adding option %s\n",opt);
	if (fo->argc >= fo->max_options) {	// need to allocate more space
		int i = fo->max_options;
		void *temp = fo->argv;
		fo->max_options += 10;
		fo->argv = (char **) owrealloc(temp, (fo->max_options + 1) * sizeof(char *));
		if (fo->argv == NULL) {
			if (temp)
				owfree(temp);
			return -ENOMEM;		// allocated ok?
		}
		for (; i <= fo->max_options; ++i)
			fo->argv[i] = NULL;	// now clear the new pointers
	}
	fo->argv[fo->argc++] = owstrdup(opt);
	//LEVEL_DEBUG("Added option %d %s\n",fo->argc-1,fo->argv[fo->argc-1]);
	return 0;
}

char *Fuse_arg(char *opt_arg, char *entryname)
{
	char *ret = NULL;
	int len = strlen(opt_arg);
	if (len < 3 || opt_arg[0] != '"' || opt_arg[len - 1] != '"') {
		fprintf(stderr, "Put the %s value in quotes. \"%s\"\n", entryname, opt_arg);
		return NULL;
	}
	ret = owstrdup(&opt_arg[1]);	// start after first quote
	if (ret == NULL) {
		fprintf(stderr, "Insufficient memory to store %s options: %s\n", entryname, opt_arg);
		return NULL;
	}
	ret[len - 2] = '\0';		// pare off trailing quote
	return ret;
}
