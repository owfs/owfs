#------------------------------------------------------------------------
# SC_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the directory containing
#				the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([SC_PATH_PYTHON],
    [AC_PREREQ(2.57)dnl

#----------------------------------------------------------------
# Look for Python
#----------------------------------------------------------------

PYINCLUDE=
PYLIB=
PYPACKAGE=

#AC_ARG_WITH(python, AS_HELP_STRING([--without-python], [Disable Python])
#AS_HELP_STRING([--with-python=path], [Set location of Python executable]),[ PYBIN="$withval"], [PYBIN=yes])
AC_ARG_WITH(python, [  --with-python           Set location of Python executable],[ PYBIN="$withval"], [PYBIN=yes])


# First, check for "--without-python" or "--with-python=no".
if test x"${PYBIN}" = xno -o x"${with_alllang}" = xno ; then 
AC_MSG_NOTICE([Disabling Python])
else
# First figure out the name of the Python executable

if test "x$PYBIN" = xyes; then
AC_CHECK_PROGS(PYTHON, python python2.4 python2.3 python2.2 python2.1 python2.0 python1.6 python1.5 python1.4 python)
else
PYTHON="$PYBIN"
fi

if test -n "$PYTHON"; then
    AC_MSG_CHECKING(for Python prefix)
    PYPREFIX=`($PYTHON -c "import sys; print sys.prefix") 2>/dev/null`
    AC_MSG_RESULT($PYPREFIX)
    AC_MSG_CHECKING(for Python exec-prefix)
    PYEPREFIX=`($PYTHON -c "import sys; print sys.exec_prefix") 2>/dev/null`
    AC_MSG_RESULT($PYEPREFIX)


    # Note: I could not think of a standard way to get the version string from different versions.
    # This trick pulls it out of the file location for a standard library file.

    AC_MSG_CHECKING(for Python version)

    # Need to do this hack since autoconf replaces __file__ with the name of the configure file
    filehack="file__"
    PYVERSION=`($PYTHON -c "import string,operator,os.path; print operator.getitem(os.path.split(operator.getitem(os.path.split(string.__$filehack),0)),1)")`
    AC_MSG_RESULT($PYVERSION)

    # Find the directory for libraries this is necessary to deal with
    # platforms that can have apps built for multiple archs: e.g. x86_64
    AC_MSG_CHECKING(for Python lib dir)
    PYLIBDIR=`($PYTHON -c "import sys; print sys.lib") 2>/dev/null`
    if test -z "$PYLIBDIR"; then
      # older versions don't have sys.lib  so the best we can do is assume lib
      PYLIBDIR="lib" 
    fi
    AC_MSG_RESULT($PYLIBDIR)
    
    # Set the include directory

    AC_MSG_CHECKING(for Python header files)
    if test -r $PYPREFIX/include/$PYVERSION/Python.h; then
        PYINCLUDE="-I$PYPREFIX/include/$PYVERSION -I$PYEPREFIX/$PYLIBDIR/$PYVERSION/config"
    fi
    if test -z "$PYINCLUDE"; then
        if test -r $PYPREFIX/include/Py/Python.h; then
            PYINCLUDE="-I$PYPREFIX/include/Py -I$PYEPREFIX/$PYLIBDIR/python/lib"
        fi
    fi
    AC_MSG_RESULT($PYINCLUDE)

    # Set the library directory blindly.   This probably won't work with older versions
    AC_MSG_CHECKING(for Python library)
    dirs="$PYVERSION/config $PYVERSION/$PYLIBDIR python/$PYLIBDIR"
    for i in $dirs; do
        if test -d $PYEPREFIX/$PYLIBDIR/$i; then
           PYLIB="$PYEPREFIX/$PYLIBDIR/$i"
           break
        fi
    done
    if test -z "$PYLIB"; then
        AC_MSG_RESULT(Not found)
    else
        AC_MSG_RESULT($PYLIB)
    fi

    # Check for really old versions
    if test -r $PYLIB/libPython.a; then
         PYLINK="-lModules -lPython -lObjects -lParser"
    else
         PYLINK="-l$PYVERSION"
    fi
fi

# Cygwin (Windows) needs the library for dynamic linking
case $host in
*-*-cygwin* | *-*-mingw*) PYTHONDYNAMICLINKING="-L$PYLIB $PYLINK"
         DEFS="-DUSE_DL_IMPORT $DEFS" PYINCLUDE="$PYINCLUDE"
         ;;
*)PYTHONDYNAMICLINKING="";;
esac
fi

])
