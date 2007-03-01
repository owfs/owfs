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

ssize_t OW_init(const char *params)
{
	ssize_t ret = 0;

	if (OWLIB_can_init_start()) {
		OWLIB_can_init_end();
		return -EALREADY;
	}

	/* Set up owlib */
	LibSetup(opt_c);

	/* Proceed with init while lock held */
	/* grab our executable name */
	Global.progname = strdup("OWCAPI");

	ret = owopt_packed(params);

	if (ret || (ret = LibStart())) {
		LibClose();
	}

	OWLIB_can_init_end();
	return ReturnAndErrno(ret);
}

ssize_t OW_init_args(int argc, char **argv)
{
	ssize_t ret = 0;
	int c;

	if (OWLIB_can_init_start()) {
		OWLIB_can_init_end();
		return ReturnAndErrno(-EALREADY);
	}

	/* Set up owlib */
	LibSetup(opt_c);

	/* Proceed with init while lock held */
	/* grab our executable name */
	if (argc > 0)
		Global.progname = strdup(argv[0]);

	while (((c =
			 getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1)
		   && (ret == 0)
		) {
		ret = owopt(c, optarg);
	}

	/* non-option arguments */
	while (optind < argc) {
		OW_ArgGeneric(argv[optind]);
		++optind;
	}

	if (ret || (ret = LibStart())) {
		LibClose();
	}

	OWLIB_can_init_end();
	return ReturnAndErrno(ret);
}

static void getdircallback(void *v, const struct parsedname *const pn2)
{
	struct charblob *cb = v;
	char buf[OW_FULLNAME_MAX + 2];
	FS_DirName(buf, OW_FULLNAME_MAX, pn2);
	CharblobAdd(buf, strlen(buf), cb);
	if (IsDir(pn2))
		CharblobAddChar('/', cb);
}

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getdir(struct one_wire_query * owq)
{
	struct charblob cb;
	int ret;

	CharblobInit(&cb);
    ret = FS_dir(getdircallback, &cb, &OWQ_pn(owq));
	if (ret < 0) {
        OWQ_buffer(owq) = NULL;
        OWQ_size(owq) = 0 ;
    } else if ((OWQ_buffer(owq) = strdup(cb.blob))) {
        ret = cb.used;
        OWQ_size(owq) = ret ;
	} else {
		ret = -ENOMEM;
        OWQ_size(owq) = 0 ;
	}
	CharblobClear(&cb);
	return ret;
}

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getval(struct one_wire_query * owq)
{
	ssize_t ret;
    ssize_t s = FullFileLength(&OWQ_pn(owq));
	if (s <= 0)
		return -ENOENT;
    if ((OWQ_buffer(owq) = malloc(s + 1)) == NULL)
		return -ENOMEM;
	ret = FS_read_postparse(owq);
	if (ret < 0) {
        free(OWQ_buffer(owq));
        OWQ_buffer(owq) = NULL;
        OWQ_size(owq) = 0 ;
    } else {
        OWQ_size(owq) = ret ;
        OWQ_buffer(owq)[ret] = '\0' ;
    }
	return ret;
}

ssize_t OW_get(const char *path, char **buffer, size_t * buffer_length)
{
	//struct parsedname pn;
    struct one_wire_query owq ;
	ssize_t ret = 0;			/* current buffer string length */

	/* Check the parameters */
	if (buffer == NULL)
		return ReturnAndErrno(-EINVAL);
	if (path == NULL)
		path = "/";
	if (strlen(path) > PATH_MAX)
		return ReturnAndErrno(-EINVAL);

	*buffer = NULL;				// default return string on error
	if (buffer_length)
		*buffer_length = 0;

	if (OWLIB_can_access_start()) {	/* Check for prior init */
		ret = -ENETDOWN;
	} else if (FS_OWQ_create(path, NULL, 0, 0, &owq)) {	/* Can we parse the input string */
		ret = -ENOENT;
	} else {
        if (IsDir(&OWQ_pn(&owq))) {		/* A directory of some kind */
            ret = getdir(&owq);
            if (ret > 0) {
                if ( buffer_length) *buffer_length = OWQ_size(&owq);
                *buffer = OWQ_buffer(&owq) ;
            }
		} else {				/* A regular file */
			ret = getval(&owq);
            if (ret > 0) {
                if ( buffer_length) *buffer_length = OWQ_size(&owq);
                *buffer = OWQ_buffer(&owq) ;
            }
		}
		FS_OWQ_destroy(&owq);
	}
	OWLIB_can_access_end();
	return ReturnAndErrno(ret);
}

ssize_t OW_lread(const char *path, char *buf, const size_t size,
				 const off_t offset)
{
	return ReturnAndErrno(FS_read(path, buf, size, offset));
}

ssize_t OW_lwrite(const char *path, const char *buf, const size_t size,
				  const off_t offset)
{
	return ReturnAndErrno(FS_write(path, buf, size, offset));
}

ssize_t OW_put(const char *path, const char *buffer, size_t buffer_length)
{
	ssize_t ret;

	/* Check the parameters */
	if (buffer == NULL || buffer_length == 0)
		return ReturnAndErrno(-EINVAL);
	if (path == NULL)
		return ReturnAndErrno(-EINVAL);
	if (strlen(path) > PATH_MAX)
		return ReturnAndErrno(-EINVAL);

	/* Check for prior init */
	if (OWLIB_can_access_start()) {
		ret = -ENETDOWN;
	} else {
		ret = FS_write(path, buffer, buffer_length, 0);
	}
	OWLIB_can_access_end();
	return ReturnAndErrno(ret);
}

void OW_finish(void)
{
	OWLIB_can_finish_start();
	LibClose();
	OWLIB_can_finish_end();
}
