// -*- C++ -*-
// File: ow.c
//
// Created: Mon Jan 10 22:34:41 2005
//
// $Id$
//

#include "owfs_config.h"
#ifdef HAVE_VASPRINTF
#define _GNU_SOURCE
#endif
#include <tcl.h>
#include "ow.h"
#include "version.h"

//extern int errno;

typedef struct OwtclStateType {
  int used;
} OwtclStateType;
  
OwtclStateType OwtclState;

#define owtcl_ObjCmdProc(name)						\
  int name (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj **objv)

#define owtcl_ArgObjIncr		     \
  int objix;                         \
  for (objix=0; objix<objc; objix++) \
    Tcl_IncrRefCount(objv[objix])

#define owtcl_ArgObjDecr				\
  for (objix=0; objix<objc; objix++)		\
    Tcl_DecrRefCount(objv[objix])

void owtcl_ErrorMsg(Tcl_Interp *interp, const char *format, ...)
{
#ifdef HAVE_VASPRINTF
  char *buf;
#else
  #define ErrBufSize 500
  char buf[ErrBufSize];
#endif
  Tcl_Obj *obj;
  va_list argsPtr;
  va_start(argsPtr, format);
#ifdef HAVE_VASPRINTF
  if (vasprintf(&buf, format, argsPtr) < 0)
#else
  if (vsnprintf(buf, ErrBufSize, format, argsPtr) < 0)
#endif
    obj = Tcl_NewStringObj(strerror(errno), -1);
  else
    obj = Tcl_NewStringObj(buf, -1);
  va_end(argsPtr);
#ifdef HAVE_VASPRINTF
  if (buf) free(buf);
#endif
  Tcl_SetObjResult(interp, obj);
}

owtcl_ObjCmdProc(Owtcl_Connect)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char *con_str;
  int con_len;
  int tcl_return = TCL_OK;
  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used) {
    owtcl_ErrorMsg(interp, "owtcl already connected.");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if (objc < 2) {
    Tcl_WrongNumArgs(interp,
		     objc,
		     objv,
		     "host:port|/dev/ttyS0|usb|u");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  LibSetup();
  background = 0 ;
  pid_file = 0 ;
  con_str = Tcl_GetStringFromObj(objv[1], &con_len);
  if ( busmode != bus_unknown ) {
    owtcl_ErrorMsg(interp, "owlib is busy");
    tcl_return = TCL_ERROR;
    goto common_exit;
  } else if (strrchr(con_str,':')) {
    servername = strdup(con_str);
    if (Server_detect()) {
      owtcl_ErrorMsg(interp, strerror(errno));
      free(servername);
      servername = NULL;
      LibClose();
      tcl_return = TCL_ERROR;
      goto common_exit;
    }
  } else if (!strncasecmp(con_str, "usb", 3) || !strncasecmp(con_str, "u", 1)) {
    if (USBSetup()) {
      owtcl_ErrorMsg(interp, strerror(errno));
      LibClose();
      tcl_return = TCL_ERROR;
      goto common_exit;
    }
  } else {
    if (ComSetup(con_str)) {
      owtcl_ErrorMsg(interp, strerror(errno));
      LibClose();
      tcl_return = TCL_ERROR;
      goto common_exit;
    }
  }

  if (LibStart()) {
    owtcl_ErrorMsg(interp, strerror(errno));
    LibClose();
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  OwtclStatePtr->used = 1;

 common_exit:
  owtcl_ArgObjDecr;
  return tcl_return;
}

owtcl_ObjCmdProc(Owtcl_Delete)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  if (OwtclStatePtr->used)
    LibClose();
  return TCL_OK;
}

owtcl_ObjCmdProc(Owtcl_Put)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char *path, *value;
  int path_len, value_len, r;
  int tcl_return = TCL_OK;
  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used == 0) {
    Tcl_AppendResult(interp, "owtcl not connected.", NULL);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if ((objc < 2) || (objc > 3)) {
    Tcl_WrongNumArgs(interp,
		     1,
		     objv,
		     "path ?value?");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }
  path = Tcl_GetStringFromObj(objv[1], &path_len);

  if (objc == 3)
    value = Tcl_GetStringFromObj(objv[2], &value_len);
  else {
    value = "\0";
    value_len = 1;
  }

  if ((r = FS_write(path, value, (size_t)value_len, (off_t)0))) {
    owtcl_ErrorMsg(interp, strerror(-r));
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

 common_exit:
  owtcl_ArgObjDecr;
  return tcl_return;
}

owtcl_ObjCmdProc(Owtcl_Get)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char *path;
  int s;
  struct parsedname pn;
  struct stateinfo si;
  char * buf = NULL, *p, *d ;
  int tcl_return = TCL_OK, r;
  Tcl_Obj *resultPtr;
  char name[OW_FULLNAME_MAX+2];
  void directory( const struct parsedname * const pn2 ) {
    FS_DirName(name, OW_FULLNAME_MAX, pn2) ;
    if (
	pn2->dev ==NULL
	|| pn2->ft ==NULL
	|| pn2->ft->format ==ft_subdir
	|| pn2->ft->format ==ft_directory
	) strcat(name, "/");
    Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(name, -1));
  }

  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used == 0) {
    Tcl_AppendResult(interp, "owtcl not connected.", NULL);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if (objc != 2) {
    Tcl_WrongNumArgs(interp,
		     1,
		     objv,
		     "path");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  path = Tcl_GetStringFromObj(objv[1], &s);

  pn.si = &si;
//  si.sg.int32 = SemiGlobal.int32;
  if ((r = FS_ParsedName(path, &pn))) {
    owtcl_ErrorMsg(interp, strerror(-r));
    tcl_return = TCL_ERROR;
    goto common_exit;
  }
  if (pn.dev==NULL || pn.ft == NULL || pn.subdir) { /* A directory of some kind */
    resultPtr = Tcl_NewListObj(0, NULL);
    FS_dir(directory, &pn) ;
    FS_ParsedName_destroy(&pn) ;
    Tcl_SetObjResult(interp, resultPtr);
  } else { /* A regular file */
    s = FullFileLength(&pn);
    if (s < 0) {
	owtcl_ErrorMsg(interp, strerror(-s));
	tcl_return = TCL_ERROR;
	goto common_exit;
    }
    if ((buf=(char *) ckalloc((size_t)s+1))) {
      r =  FS_read_postparse(buf, (size_t)s, (off_t)0, &pn) ;
      FS_ParsedName_destroy(&pn) ;
      if ( r<0 ) {
	owtcl_ErrorMsg(interp, strerror(-r));
	ckfree(buf);
	tcl_return = TCL_ERROR;
	goto common_exit;
      }
      buf[s] = '\0';
      if (strchr(buf, ',')) {
	resultPtr = Tcl_NewListObj(0, NULL);
	p = buf;
	while((d = strchr(p, ',')) != NULL) {
	  Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(p, d-p));
	  d++;
	  p = d;
	  printf("p: '%s'\n", p);
	}
	Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(p, -1));
      } else {
	resultPtr = Tcl_NewStringObj(buf, -1);
      }
    }
  }
  Tcl_SetObjResult(interp, resultPtr);
  
 common_exit:
  owtcl_ArgObjDecr;
  return tcl_return;
}

owtcl_ObjCmdProc(Owtcl_IsDir)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char *path;
  int s, r;
  struct parsedname pn;
  struct stateinfo si;
  int tcl_return = TCL_OK;
  Tcl_Obj *resultPtr;

  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used == 0) {
    Tcl_AppendResult(interp, "owtcl not connected.", NULL);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if (objc != 2) {
    Tcl_WrongNumArgs(interp,
		     1,
		     objv,
		     "path");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  path = Tcl_GetStringFromObj(objv[1], &s);

  pn.si = &si;
  if ((r = FS_ParsedName(path, &pn))) {
    owtcl_ErrorMsg(interp, strerror(-r));
    tcl_return = TCL_ERROR;
    goto common_exit;
  }
  resultPtr = Tcl_GetObjResult(interp);
  if (pn.dev==NULL || pn.ft == NULL || pn.subdir) /* A directory of some kind */
    Tcl_SetIntObj(resultPtr, 1);
  else
    Tcl_SetIntObj(resultPtr, 0);

 common_exit:
  owtcl_ArgObjDecr;
  return tcl_return;
}

/*
 *----------------------------------------------------------------------
 * Ow_Init --
 *   perform all initialization for the owlib - Tcl interface.
 *
 *   a call to Owtcl_Init should exist in Tcl_CreateInterp or
 *   Tcl_CreateExtendedInterp.
 *----------------------------------------------------------------------
 */

struct CmdListType {
  char *name;
  void *func;
} OwtclCmdList[] = {
  {"::OW::init", Owtcl_Connect},
  {"::OW::put", Owtcl_Put},
  {"::OW::get", Owtcl_Get},
  {"::OW::finish", Owtcl_Delete},
  {"::OW::isdirectory", Owtcl_IsDir},
  {"::OW::isdir", Owtcl_IsDir},
  {NULL, NULL}
};

int
Ow_Init (Tcl_Interp *interp) {
  int i;
  OwtclState.used = 0;

  /*
   * Initialize the new Tcl commands
   */
  i = 0;
  while(OwtclCmdList[i].name != NULL) {
    Tcl_CreateObjCommand (interp, OwtclCmdList[i].name, (Tcl_ObjCmdProc*) OwtclCmdList[i].func,
			  (ClientData) &OwtclState, (Tcl_CmdDeleteProc *) NULL);
    i++;
  }
  
  /* callback - clean up procs left open on interpreter deletetion */
  Tcl_CallWhenDeleted(interp,
  		      (Tcl_InterpDeleteProc *) Owtcl_Delete,
  		      (ClientData) &OwtclState);
  
  if (Tcl_PkgProvide(interp, "ow", OWTCL_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * Owtcl_SafeInit --
 *	call the standard init point.
 *----------------------------------------------------------------------
 */

int
Ow_SafeInit (Tcl_Interp *interp) {
  int result;
  
  result = Ow_Init(interp);
  return (result);
}

