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
//#include "ow.h"
#include "ow_dl.h"

DLHANDLE DL_open(const char *pathname, int mode)
{
#ifdef HAVE_DLOPEN
  return dlopen(pathname, mode);
#elif OW_CYGWIN
  return LoadLibrary(pathname);
#endif

}

void *DL_sym(DLHANDLE handle, const char *name)
{
#ifdef HAVE_DLOPEN
  return dlsym(handle, name);
#elif OW_CYGWIN
  return (void *) GetProcAddress(handle, name);
#endif

}


int DL_close(DLHANDLE handle)
{
#ifdef HAVE_DLOPEN
  return dlclose(handle);
#elif OW_CYGWIN
  if (FreeLibrary(handle)) return 0;
  return -1;
#endif
}

char *DL_error(void)
{
#ifdef HAVE_DLOPEN
  return dlerror();
#elif OW_CYGWIN
  return "Error in WIN32 DL";
#endif
}

