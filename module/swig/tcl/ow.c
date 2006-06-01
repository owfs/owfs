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
  char *con_str, *arg;
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
		     "/dev/ttyS0|usb?N?|u?N?|host:port|socket:/local-socket-path ?...? ?options?");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  LibSetup();
  delay_background = 1 ;

  for (objix=1; objix<objc; objix++) {
    arg = Tcl_GetStringFromObj(objv[objix], &con_len);
    if (!strncasecmp(arg, "-", 1)) {
      if (!strncasecmp(arg, "-format", 7)) {
        objix++;
        arg = Tcl_GetStringFromObj(objv[objix], &con_len);
        if (!strcasecmp(arg,"f.i"))        set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
        else if (!strcasecmp(arg,"fi"))    set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
        else if (!strcasecmp(arg,"f.i.c")) set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
        else if (!strcasecmp(arg,"f.ic"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
        else if (!strcasecmp(arg,"fi.c"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
        else if (!strcasecmp(arg,"fic"))   set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
        else {
  	  owtcl_ErrorMsg(interp, "bad format \"%s\": should be one of f.i, fi, f.i.c, f.ic, fi.c or fic\n",arg);
	  tcl_return = TCL_ERROR;
	  goto common_exit;
        }
      } else if (!strncasecmp(arg, "-celsius", 8)) {
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
      } else if (!strncasecmp(arg, "-fahrenheit", 11)) {
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
      } else if (!strncasecmp(arg, "-kelvin", 7)) {
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
      } else if (!strncasecmp(arg, "-rankine", 8)) {
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
      } else if (!strncasecmp(arg, "-cache", 6)) {
        objix++;
        Timeout(Tcl_GetStringFromObj(objv[objix], &con_len));
      } else if (!strncasecmp(arg, "-readonly", 9)) {
        readonly = 1;
      } else if (!strncasecmp(arg, "-error-print", 12)) {
        objix++;
        arg = Tcl_GetStringFromObj(objv[objix], &con_len);
        error_print = atoi(arg);
      } else if (!strncasecmp(arg, "-error-level", 12)) {
        objix++;
        arg = Tcl_GetStringFromObj(objv[objix], &con_len);
	error_level = atoi(arg);
      } else {
        owtcl_ErrorMsg(interp, "bad option \"%s\": should be one of -format, -celsius, -fahrenheit, -kelvin, -rankine, -cache -readonly, -error-print or -error-level\n", arg);
        tcl_return = TCL_ERROR;
        goto common_exit;
      }
    } else {
      if (strrchr(arg,':')) {
	if (!strncasecmp(arg, "socket:", 7))
	  OW_ArgNet(&arg[7]);
	else
	  OW_ArgNet(arg);
      } else if (!strncasecmp(arg, "usb", 3) || !strncasecmp(arg, "u", 1)) {
	char *p = arg;
	while (*p != '\0') {
	  if ((*p >= '0') && (*p <= '9')) break;
	  p++;
	}
	OW_ArgUSB(p);
      } else {
	OW_ArgDevice(arg);
      }
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

  if ((r = FS_write(path, value, (size_t)value_len, (off_t)0)) < 0) {
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
  char *path;
  int s;
  struct parsedname pn;
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

  memset(&pn, 0, sizeof(struct parsedname)); // make sure common_exit could be used for all exits.
  if (OwtclStatePtr->used == 0) {
    Tcl_AppendResult(interp, "owtcl not connected.", NULL);
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if (objc < 2)
    path = "";
  else if (objc == 2)
    path = Tcl_GetStringFromObj(objv[1], &s);
  else {
    Tcl_WrongNumArgs(interp,
		     1,
		     objv,
		     "?path?");
    tcl_return = TCL_ERROR;
    goto common_exit;
  }

  if ((r = FS_ParsedName(path, &pn))) {
    owtcl_ErrorMsg(interp, strerror(-r));
    tcl_return = TCL_ERROR;
    goto common_exit;
  }
  if (pn.dev==NULL || pn.ft == NULL || pn.subdir) { /* A directory of some kind */
    resultPtr = Tcl_NewListObj(0, NULL);
    FS_dir(directory, &pn) ;
    Tcl_SetObjResult(interp, resultPtr);
  } else { /* A regular file */
    s = FullFileLength(&pn) ;
    if (s < 0) {
	owtcl_ErrorMsg(interp, strerror(-s));
	tcl_return = TCL_ERROR;
	goto common_exit;
    }
    if ((buf=(char *) ckalloc((size_t)s+1))) {
      r =  FS_read_postparse(buf, (size_t)s, (off_t)0, &pn) ;
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
  FS_ParsedName_destroy(&pn) ;
  owtcl_ArgObjDecr;
  return tcl_return;
}


owtcl_ObjCmdProc(Owtcl_Version)
{
  OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
  char buf[128];
  Tcl_Obj *resultPtr;
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
  {"::OW::init", Owtcl_Connect},
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

