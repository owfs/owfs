// -*- C++ -*-
// File: ow.c
//
// Created: Mon Jan 10 22:34:41 2005
//
// $Id$
//

#include <config.h>
#include "owfs_config.h"
#ifdef HAVE_VASPRINTF
#define _GNU_SOURCE
#endif
#include <tcl.h>
#include "ow.h"
#include "owcapi.h"
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
  char *arg;
  int con_len;
  int tcl_return = TCL_OK, r;
  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used) {
    owtcl_ErrorMsg(interp, "owtcl already connected.");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }
  arg = Tcl_GetStringFromObj(objv[1], &con_len);
  r = OW_init(arg);
  if (r != 0) {
    owtcl_ErrorMsg(interp, strerror(errno));
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

  (void) interp ; // suppress compiler warning
  (void) objc ; // suppress compiler warning
  (void) objv ; // suppress compiler warning

  if (OwtclStatePtr->used)
    OW_finish();
  OwtclStatePtr->used = 0;
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
    value = "\n";
    value_len = 1;
  }

  if ((r = OW_put(path, value, (size_t)value_len)) < 0) {
    owtcl_ErrorMsg(interp, strerror(r));
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
  char *arg, *path, *buf, *d, *p;
  int tcl_return = TCL_OK, r, s, lst;
  size_t ss ;
  Tcl_Obj *resultPtr;
  owtcl_ArgObjIncr;

  if (OwtclStatePtr->used == 0) {
    Tcl_AppendResult(interp, "owtcl not connected.", NULL);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  path = "";
  lst = 0;
  for (objix=1; objix<objc; objix++) {
    arg = Tcl_GetStringFromObj(objv[objix], &s);
    if (!strncasecmp(arg, "-", 1)) {
      if (!strncasecmp(arg, "-list", 5)) {
	lst = 1;
      } else {
	owtcl_ErrorMsg(interp, "bad switch \"%s\": should be -list\n", arg);
	tcl_return = TCL_ERROR;
	goto common_exit;
      }
    } else {
      path = Tcl_GetStringFromObj(objv[objix], &s);
    }
  }

  r = OW_get(path, &buf, &ss);
  s = ss ; // to get around OW_get uses size_t
  if ( r<0 ) {
    owtcl_ErrorMsg(interp, strerror(-r));
    free(buf);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  buf[s] = 0;
  if (lst) {
    if (strchr(buf, ',')) {
      resultPtr = Tcl_NewListObj(0, NULL);
      p = buf;
      while((d = strchr(p, ',')) != NULL) {
        Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(p, d-p));
        d++;
        p = d;
      }
      Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(p, -1));
    } else {
      resultPtr = Tcl_NewStringObj(buf, -1);
    }
  } else {
    resultPtr = Tcl_NewStringObj(buf, -1);
  }
  Tcl_SetObjResult(interp, resultPtr);

 common_exit:
  owtcl_ArgObjDecr;
  return tcl_return;
}

owtcl_ObjCmdProc(Owtcl_Version)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char buf[128];
  Tcl_Obj *resultPtr;


  (void) OwtclStatePtr ; // suppress compiler warning
  (void) objc ; // suppress compiler warning
  (void) objv ; // suppress compiler warning

  sprintf(buf, "owtcl:\t%s\nlibow:\t%s", OWTCL_VERSION, VERSION);
  resultPtr = Tcl_NewStringObj(buf, -1);
  Tcl_SetObjResult(interp, resultPtr);
  return TCL_OK;
}

owtcl_ObjCmdProc(Owtcl_IsDir)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char *path;
  int s, r;
  struct parsedname pn;
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
  {"::OW::_init", Owtcl_Connect},
  {"::OW::put", Owtcl_Put},
  {"::OW::get", Owtcl_Get},
  {"::OW::version", Owtcl_Version},
  {"::OW::finish", Owtcl_Delete},
  {"::OW::isdirectory", Owtcl_IsDir},
  {"::OW::isdir", Owtcl_IsDir},
  {NULL, NULL}
};

int
Ow_Init (Tcl_Interp *interp) {
  int i;


  /* This defines the static chars tkTable(Safe)InitScript */
#include "owtclInitScript.h"

  if (
#ifdef USE_TCL_STUBS
      Tcl_InitStubs(interp, "8.1", 0)
#else
      Tcl_PkgRequire(interp, "Tcl", "8.1", 0)
#endif
      == NULL) {
    return TCL_ERROR;
  }

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

  /*
   * The init script can't make certain calls in a safe interpreter,
   * so we always have to use the embedded runtime for it
   */
  return Tcl_Eval(interp, Tcl_IsSafe(interp) ?
		  owtclSafeInitScript : owtclInitScript);

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

