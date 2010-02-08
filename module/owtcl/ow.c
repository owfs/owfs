// -*- C++ -*-
// File: ow.c
//
// Created: Mon Jan 10 22:34:41 2005
//
// $Id$
//

// Some Ugly code to keep command line #define from being over_written by <config.h>
#define SAVE_FOR_NOW_PACKAGE_NAME    PACKAGE_NAME
#undef               PACKAGE_NAME
#define SAVE_FOR_NOW_PACKAGE_STRING  PACKAGE_STRING
#undef               PACKAGE_STRING
#define SAVE_FOR_NOW_PACKAGE_TARNAME PACKAGE_TARNAME
#undef               PACKAGE_TARNAME
#define SAVE_FOR_NOW_PACKAGE_VERSION PACKAGE_VERSION
#undef               PACKAGE_VERSION

#include <config.h>
// Now restore
#undef  PACKAGE_NAME
#define PACKAGE_NAME    SAVE_FOR_NOW_PACKAGE_NAME
#undef  PACKAGE_STRING
#define PACKAGE_STRING  SAVE_FOR_NOW_PACKAGE_STRING
#undef  PACKAGE_TARNAME
#define PACKAGE_TARNAME SAVE_FOR_NOW_PACKAGE_TARNAME
#undef  PACKAGE_VERSION
#define PACKAGE_VERSION SAVE_FOR_NOW_PACKAGE_VERSION

#include "owfs_config.h"
#ifdef HAVE_VASPRINTF
#define _GNU_SOURCE
#endif
#include <tcl.h>
#include "ow.h"
#include "owcapi.h"
#include "version.h"
#include <stdio.h>



/* Shorthand macros for some cumbersome definitions. */
#define owtcl_ObjCmdProc(name)            \
  int name (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj **objv)

#define owtcl_ArgObjIncr         \
  int objix;                         \
  for (objix=0; objix<objc; objix++) \
    Tcl_IncrRefCount(objv[objix])

#define owtcl_ArgObjDecr        \
  for (objix=0; objix<objc; objix++)    \
    Tcl_DecrRefCount(objv[objix])



/* Internally used types and vars. */
typedef struct OwtclStateType {
	int used;
} OwtclStateType;

static OwtclStateType OwtclState;



/*
 * Return error message and code for OWlib errors.
 */
void owtcl_ErrorOWlib(Tcl_Interp * interp)
{
	/* Generate a posix like error message and code. */
	Tcl_SetResult(interp, Tcl_ErrnoMsg(Tcl_GetErrno()), TCL_STATIC);
	Tcl_SetErrorCode(interp, "OWTCL", Tcl_ErrnoId(), Tcl_ErrnoMsg(Tcl_GetErrno()), NULL);
}



/*
 * Return error message and code for owtcl internal and syntax errors.
 */
void owtcl_Error(Tcl_Interp * interp, char *error_family, char *error_code, char *format, ...)
{
#ifdef HAVE_VASPRINTF
	char *buf;
#else
#define ErrBufSize 500
	char buf[ErrBufSize];
#endif
	va_list argsPtr;

	va_start(argsPtr, format);

#ifdef HAVE_VASPRINTF
	if (vasprintf(&buf, format, argsPtr) < 0)
#else
	if (vsnprintf(buf, ErrBufSize, format, argsPtr) < 0)
#endif
	{
		/* Error within vasprintf/vsnprintf */
		Tcl_SetResult(interp, Tcl_ErrnoMsg(Tcl_GetErrno()), TCL_STATIC); 
		#warning Ignore compiler warning since Tcl_ErrnoMsg is treated TCL_STATIC
		Tcl_PosixError(interp);
	} else {
		/* Generate a posix like error message and code. */
		Tcl_SetResult(interp, buf, TCL_VOLATILE); 
		#warning Ignore compiler warning since Tcl_ErrnoMsg is treated TCL_STATIC
		Tcl_SetErrorCode(interp, error_family, error_code, NULL);
	}

	va_end(argsPtr);
#ifdef HAVE_VASPRINTF
	if (buf) {
		free(buf);
	}
#endif
}





/*
 * Connect to onewire host(s).
 */
owtcl_ObjCmdProc(Owtcl_Connect)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *arg;
	int con_len;
	int tcl_return = TCL_OK, r;
	owtcl_ArgObjIncr;

	/* Check we aren't already connected to onewire host(s). */
	if (OwtclStatePtr->used) {
		owtcl_Error(interp, "OWTCL", "CONNECTED", "owtcl already connected");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Actually connect to onewire host(s). */
	arg = Tcl_GetStringFromObj(objv[1], &con_len);
	r = OW_init(arg);
	if (r != 0) {
		owtcl_ErrorOWlib(interp);
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Remember connected state. */
	OwtclStatePtr->used = 1;

  common_exit:
	owtcl_ArgObjDecr;
	return tcl_return;
}



/*
 * Disconnect from onewire host(s).
 */
owtcl_ObjCmdProc(Owtcl_Delete)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;

	(void) interp;				// suppress compiler warning
	(void) objc;				// suppress compiler warning
	(void) objv;				// suppress compiler warning

	/* Disconnect if connected, otherwise ignore. */
	if (OwtclStatePtr->used)
		OW_finish();

	/* Remember disconnected state. */
	OwtclStatePtr->used = 0;
	return TCL_OK;
}

/*
 * Set error_level
 */
owtcl_ObjCmdProc(Owtcl_Set_error_level)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *arg;
	int arg_len;
	owtcl_ArgObjIncr;

	(void) interp;				// suppress compiler warning
	(void) objc;				// suppress compiler warning
	(void) objv;				// suppress compiler warning

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "?value?");
		owtcl_ArgObjDecr;
		return TCL_ERROR;
	}

	if (!OwtclStatePtr->used) {
		owtcl_Error(interp, "OWTCL", "DISCONNECTED", "owtcl disconnected");
		owtcl_ArgObjDecr;
		return TCL_ERROR;
	}
	arg = Tcl_GetStringFromObj(objv[1], &arg_len);

	OW_set_error_level(arg);

	return TCL_OK;
}

/*
 * Set error_level
 */
owtcl_ObjCmdProc(Owtcl_Set_error_print)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *arg;
	int arg_len;
	owtcl_ArgObjIncr;

	(void) interp;				// suppress compiler warning
	(void) objc;				// suppress compiler warning
	(void) objv;				// suppress compiler warning

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "?value?");
		owtcl_ArgObjDecr;
		return TCL_ERROR;
	}

	if (!OwtclStatePtr->used) {
		owtcl_Error(interp, "OWTCL", "DISCONNECTED", "owtcl disconnected");
		owtcl_ArgObjDecr;
		return TCL_ERROR;
	}
	arg = Tcl_GetStringFromObj(objv[1], &arg_len);

	OW_set_error_print(arg);

	return TCL_OK;
}

/*
 * Change a owfs node's value. 
 */
owtcl_ObjCmdProc(Owtcl_Put)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *path, *value;
	int path_len, value_len, r;
	int tcl_return = TCL_OK;
	owtcl_ArgObjIncr;

	/* Check for arguments to the commmand. */
	if ((objc < 2) || (objc > 3)) {
		Tcl_WrongNumArgs(interp, 1, objv, "path ?value?");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}
	if (objc == 3)
		value = Tcl_GetStringFromObj(objv[2], &value_len);
	else {
		value = "\n";
		value_len = 1;
	}

	/* Check we are connected to onewire host(s). */
	if (OwtclStatePtr->used == 0) {
		owtcl_Error(interp, "OWTCL", "DISCONNECTED", "owtcl disconnected");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Change the owfs node's value. */
	path = Tcl_GetStringFromObj(objv[1], &path_len);
	if ((r = OW_put(path, value, (size_t) value_len)) < 0) {
		owtcl_ErrorOWlib(interp);
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

  common_exit:
	owtcl_ArgObjDecr;
	return tcl_return;
}



/*
 * Get a owfs node's value. (for directories a directory listing)
 */
owtcl_ObjCmdProc(Owtcl_Get)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *arg, *path, *buf = NULL, *d, *p;
	int tcl_return = TCL_OK, r, s, lst;
	size_t ss;
	Tcl_Obj *resultPtr;
	owtcl_ArgObjIncr;

	/* Check for arguments and options to the commmand. */
	path = "";
	lst = 0;
	for (objix = 1; objix < objc; objix++) {
		arg = Tcl_GetStringFromObj(objv[objix], &s);
		if (!strncasecmp(arg, "-", 1)) {
			if (!strncasecmp(arg, "-list", 5)) {
				lst = 1;
			} else {
				owtcl_Error(interp, "NONE", NULL, "bad switch \"%s\": should be -list", arg);
				tcl_return = TCL_ERROR;
				goto common_exit;
			}
		} else {
			path = Tcl_GetStringFromObj(objv[objix], &s);
		}
	}

	/* Check we are connected to onewire host(s). */
	if (OwtclStatePtr->used == 0) {
		owtcl_Error(interp, "OWTCL", "DISCONNECTED", "owtcl disconnected");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Get the owfs node's value. */
	r = OW_get(path, &buf, &ss);
	s = ss;						// to get around OW_get uses size_t
	if (r < 0) {
		owtcl_ErrorOWlib(interp);
		if (buf != NULL) {
			free(buf);
		}
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Arrange the value to form a proper tcl result. */
	if (buf == NULL) {
		tcl_return = TCL_OK;
		goto common_exit;
	}
	buf[s] = 0;
	if (lst) {
		if (strchr(buf, ',')) {
			resultPtr = Tcl_NewListObj(0, NULL);
			p = buf;
			while ((d = strchr(p, ',')) != NULL) {
				Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(p, d - p));
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
	free(buf);
	
  common_exit:
	owtcl_ArgObjDecr;
	return tcl_return;
}



/*
 * Get version info for owtcl and used owlib.
 */
owtcl_ObjCmdProc(Owtcl_Version)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char buf[128];
	Tcl_Obj *resultPtr;

	(void) OwtclStatePtr;		// suppress compiler warning
	(void) objc;				// suppress compiler warning
	(void) objv;				// suppress compiler warning

	sprintf(buf, "owtcl:\t%s\nlibow:\t%s", OWTCL_VERSION, VERSION);
	resultPtr = Tcl_NewStringObj(buf, -1);
	Tcl_SetObjResult(interp, resultPtr);
	return TCL_OK;
}



/*
 * Check if a owfs node is a directory.
 */
owtcl_ObjCmdProc(Owtcl_IsDir)
{
	OwtclStateType *OwtclStatePtr = (OwtclStateType *) clientData;
	char *path;
	int s, r;
	struct parsedname pn;
	int tcl_return = TCL_OK;
	Tcl_Obj *resultPtr;

	owtcl_ArgObjIncr;

	/* Check for arguments to the commmand. */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "path");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Check we are connected to onewire host(s). */
	if (OwtclStatePtr->used == 0) {
		owtcl_Error(interp, "OWTCL", "DISCONNECTED", "owtcl disconnected");
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Get the owfs node. */
	path = Tcl_GetStringFromObj(objv[1], &s);
	if ((r = FS_ParsedName(path, &pn))) {
		Tcl_SetErrno(ENOENT);	/* No such file or directory */
		owtcl_ErrorOWlib(interp);
		tcl_return = TCL_ERROR;
		goto common_exit;
	}

	/* Check if directory or file. */
	resultPtr = Tcl_GetObjResult(interp);
	if (pn.selected_device == NULL || pn.selected_filetype == NULL || pn.subdir)	/* A directory of some kind */
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
	{
	"::OW::_init", Owtcl_Connect}, {
	"::OW::put", Owtcl_Put}, {
	"::OW::get", Owtcl_Get}, {
	"::OW::set_error_level", Owtcl_Set_error_level}, {
	"::OW::set_error_print", Owtcl_Set_error_print}, {
	"::OW::version", Owtcl_Version}, {
	"::OW::finish", Owtcl_Delete}, {
	"::OW::isdirectory", Owtcl_IsDir}, {
	"::OW::isdir", Owtcl_IsDir}, {
	NULL, NULL}
};

int Ow_Init(Tcl_Interp * interp)
{
	int i;

	/* This defines the static chars tkTable(Safe)InitScript */
#include "owtclInitScript.h"

	if (
#ifdef USE_TCL_STUBS
		   Tcl_InitStubs(interp, "8.1", 0)
#else
		   Tcl_PkgRequire(interp, "Tcl", "8.1", 0)
#endif
		   == NULL)
		return TCL_ERROR;

	OwtclState.used = 0;

	/* Initialize the new Tcl commands */
	i = 0;
	while (OwtclCmdList[i].name != NULL) {
		Tcl_CreateObjCommand(interp, OwtclCmdList[i].name, (Tcl_ObjCmdProc *) OwtclCmdList[i].func,
							 (ClientData) & OwtclState, (Tcl_CmdDeleteProc *) NULL);
		i++;
	}

	/* Callback - clean up procs left open on interpreter deletetion. */
	Tcl_CallWhenDeleted(interp, (Tcl_InterpDeleteProc *) Owtcl_Delete, (ClientData) & OwtclState);

	/* Announce successful package loading to "package require". */
	if (Tcl_PkgProvide(interp, "ow", OWTCL_VERSION) != TCL_OK)
		return TCL_ERROR;

	/*
	 * The init script can't make certain calls in a safe interpreter,
	 * so we always have to use the embedded runtime for it
	 */
	return Tcl_Eval(interp, Tcl_IsSafe(interp) ? owtclSafeInitScript : owtclInitScript);
}



/*
 *----------------------------------------------------------------------
 * Owtcl_SafeInit --
 *  call the standard init point.
 *----------------------------------------------------------------------
 */
int Ow_SafeInit(Tcl_Interp * interp)
{
	int result;

	result = Ow_Init(interp);
	return (result);
}
