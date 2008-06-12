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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "owcapi.h"
#include <limits.h>

#if OW_MT
pthread_t main_threadid;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else							/* OW_MT */
#define IS_MAINTHREAD 1
#endif							/* OW_MT */

#define MAX_ARGS 20

static ssize_t ReturnAndErrno(ssize_t ret)
{
	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		errno = 0;
		return ret;
	}
}

void OW_set_error_print(const char *params)
{
	API_set_error_print(params);
}

void OW_set_error_level(const char *params)
{
	API_set_error_level(params);
}

ssize_t OW_init(const char *params)
{
	ssize_t ret = 0;

	/* Set up owlib */
	API_setup(opt_c);

	/* Proceed with init while lock held */
	/* grab our executable name */
	Global.progname = strdup("OWCAPI");

    ret = API_init(params) ;

	if (ret) {
		API_finish();
	}

	return ReturnAndErrno(ret);
}

ssize_t OW_init_args(int argc, char **argv)
{
	ssize_t ret = 0;
	int c;

	/* Set up owlib */
	API_setup(opt_c);

	/* Proceed with init while lock held */
	/* grab our executable name */
	if (argc > 0)
		Global.progname = strdup(argv[0]);

	while (((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1)
		   && (ret == 0)
		) {
		ret = owopt(c, optarg);
	}

	/* non-option arguments */
	while (optind < argc) {
		OW_ArgGeneric(argv[optind]);
		++optind;
	}

    if (ret || (ret = API_init(NULL))) {
		API_finish();
	}

	return ReturnAndErrno(ret);
}

static void getdircallback(void *v, const struct parsedname *const pn_entry)
{
	struct charblob *cb = v;
	const char * buf = FS_DirName(pn_entry);
	CharblobAdd(buf, strlen(buf), cb);
	if (IsDir(pn_entry))
		CharblobAddChar('/', cb);
}

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getdir(struct one_wire_query *owq)
{
	struct charblob cb;
	int ret;

	CharblobInit(&cb);
	ret = FS_dir(getdircallback, &cb, PN(owq));
	if (ret < 0) {
		OWQ_buffer(owq) = NULL;
		OWQ_size(owq) = 0;
	} else if ((OWQ_buffer(owq) = strdup(cb.blob))) {
		ret = cb.used;
		OWQ_size(owq) = ret;
	} else {
		ret = -ENOMEM;
		OWQ_size(owq) = 0;
	}
	CharblobClear(&cb);
	return ret;
}

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getval(struct one_wire_query *owq)
{
	ssize_t ret;
	ssize_t s = FullFileLength(PN(owq));
	if (s <= 0)
		return -ENOENT;
	if ((OWQ_buffer(owq) = malloc(s + 1)) == NULL)
		return -ENOMEM;
	OWQ_size(owq) = s;
	ret = FS_read_postparse(owq);
	if (ret < 0) {
		free(OWQ_buffer(owq));
		OWQ_buffer(owq) = NULL;
		OWQ_size(owq) = 0;
	} else {
		OWQ_size(owq) = ret;
		OWQ_buffer(owq)[ret] = '\0';
	}
	return ret;
}

ssize_t OW_get(const char *path, char **buffer, size_t * buffer_length)
{
	//struct parsedname pn;
	struct one_wire_query owq;
	ssize_t ret = -EACCES;			/* current buffer string length */

	/* Check the parameters */
    if (buffer == NULL) {
		return ReturnAndErrno(-EINVAL);
    }

    if (path == NULL) {
		path = "/";
    }

	*buffer = NULL;				// default return string on error
    if ( buffer_length != NULL ) {
        *buffer_length = 0 ;
    }

    if (API_access_start()==0) {	/* Check for prior init */
	   if (FS_OWQ_create(path, NULL, 0, 0, &owq)) {	/* Can we parse the input string */
    		ret = -ENOENT;
        } else {
            if (IsDir(PN(&owq))) {	/* A directory of some kind */
                ret = getdir(&owq);
            } else {				/* A regular file */
                ret = getval(&owq);
            }
            if ( ret >= 0 ) {
                *buffer = OWQ_buffer(&owq);
                if ( buffer_length != NULL ) {
                    *buffer_length = OWQ_size(&owq) ;
                }
            }
            FS_OWQ_destroy(&owq);
        }
        API_access_end() ;
	}
	return ReturnAndErrno(ret);
}

ssize_t OW_lread(const char *path, char *buffer, const size_t size, const off_t offset)
{
    ssize_t ret = -EACCES;         /* current buffer string length */

    /* Check the parameters */
    if (buffer == NULL || path==NULL ) {
        return ReturnAndErrno(-EINVAL);
    }
    
    if ( API_access_start()==0 ) {
        ret = FS_read(path, buffer, size, offset) ;
        API_access_end() ;
    }
    return ReturnAndErrno(ret);
}

ssize_t OW_lwrite(const char *path, const char *buffer, const size_t size, const off_t offset)
{
    ssize_t ret = -EACCES;         /* current buffer string length */

    /* Check the parameters */
    if (buffer == NULL || path==NULL ) {
        return ReturnAndErrno(-EINVAL);
    }
    
    if ( API_access_start()==0 ) {
        ret = FS_write(path, buffer, size, offset) ;
        API_access_end() ;
    }
    return ReturnAndErrno(ret);
}

ssize_t OW_put(const char *path, const char *buffer, size_t buffer_length)
{
	ssize_t ret = -EACCES ;

	/* Check the parameters */
    if (buffer == NULL || path==NULL ) {
		return ReturnAndErrno(-EINVAL);
    }
    
	/* Check that we have done init, and that we can access */
    if ( API_access_start()==0 ) {
		ret = FS_write(path, buffer, buffer_length, 0);
        API_access_end() ;
    }
	return ReturnAndErrno(ret);
}

void OW_finish(void)
{
	API_finish();
}
