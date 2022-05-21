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
#include "ow_dl.h"

#if OW_ZERO

DLHANDLE DL_open(const char *pathname)
{
#if OW_CYGWIN
	return LoadLibrary(pathname);
#elif defined(HAVE_DLOPEN)
	return dlopen(pathname, RTLD_LAZY | RTLD_GLOBAL );
#endif

}

void *DL_sym(DLHANDLE handle, const char *name)
{
#if OW_CYGWIN
	return (void *) GetProcAddress(handle, name);
#elif defined(HAVE_DLOPEN)
	return dlsym(handle, name);
#endif

}


int DL_close(DLHANDLE handle)
{
#if OW_CYGWIN
	if (FreeLibrary(handle)) {
		return 0;
	}
	return -1;
#elif defined(HAVE_DLOPEN)
	return dlclose(handle);
#endif
}

char *DL_error(void)
{
#if OW_CYGWIN
	return "Error in WIN32 DL";
#elif defined(HAVE_DLOPEN)
	return dlerror();
#endif
}

#endif
