#------------------------------------------------------------------------
# SC_PATH_PYTHONCONFIG --
#
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-pythonconfig=...
#		--with-python=...
#
#	Defines the following vars:
#
#------------------------------------------------------------------------

AC_DEFUN([SC_PATH_PYTHON],
    [AC_PREREQ(2.57)dnl

#----------------------------------------------------------------
# Look for Python
#----------------------------------------------------------------

PYCFLAGS=
PYLDFLAGS=
PYLIB=
PYVERSION=
PYSITEDIR=

AC_ARG_WITH(python, [  --with-python           Set location of Python executable],[ PYBIN="$withval"], [PYBIN=yes])
AC_ARG_WITH(pythonconfig, [  --with-pythonconfig        Set location of python-config executable],[ PYTHONCONFIGBIN="$withval"], [PYTHONCONFIGBIN=yes])

if test "x$PYTHONCONFIGBIN" = xyes; then
       AC_CHECK_PROGS(PYTHONCONFIG, python-config python3.12-config python3.11-config python3.10-config python3.9-config python3.8-config)
else
      PYTHONCONFIG="$PYTHONCONFIGBIN"
fi

# First, check for "--without-python" or "--with-python=no".
if test x"${PYBIN}" = xno -o x"${with_alllang}" = xno ; then 
AC_MSG_NOTICE([Disabling Python])
else
# First figure out the name of the Python executable

if test "x$PYBIN" = xyes; then
AC_CHECK_PROGS(PYTHON, python python3.12 python3.11 python3.10 python3.9 python3.8)
else
PYTHON="$PYBIN"
fi


if test ! -z "$PYTHONCONFIG"; then

# python-config available.
   AC_MSG_CHECKING(for Python cflags)
   PYTHONCFLAGS="`$PYTHONCONFIG --cflags 2>/dev/null`"
   if test -z "$PYTHONCFLAGS"; then
	AC_MSG_RESULT(not found)
   else
	AC_MSG_RESULT($PYTHONCFLAGS)
   fi
   PYCFLAGS=$PYTHONCFLAGS

   AC_MSG_CHECKING(for Python ldflags)
   PYTHONLDFLAGS="`$PYTHONCONFIG --ldflags 2>/dev/null`"
   if test -z "$PYTHONLDFLAGS"; then
	AC_MSG_RESULT(not found)
   else
   	AC_MSG_RESULT($PYTHONLDFLAGS)
   fi
   PYLDFLAGS=$PYTHONLDFLAGS

   AC_MSG_CHECKING(for Python libs)
   PYTHONLIBS="`$PYTHONCONFIG --libs 2>/dev/null`"
   if test -z "$PYTHONLIBS"; then
	AC_MSG_RESULT(not found)
   else
   	AC_MSG_RESULT($PYTHONLIBS)
   fi
   PYLIB="$PYTHONLIBS"

   # Need to do this hack since autoconf replaces __file__ with the name of the configure file
   filehack="file__"
   PYVERSION=`($PYTHON -c "import string,operator,os.path; print(operator.getitem(os.path.split(operator.getitem(os.path.split(string.__$filehack),0)),1))")`
   AC_MSG_RESULT($PYVERSION)

   AC_MSG_CHECKING(for Python exec-prefix)
   PYTHONEPREFIX="`$PYTHONCONFIG --exec-prefix 2>/dev/null`"
   if test -z "$PYTHONEPREFIX"; then
     AC_MSG_RESULT(not found)
   else
     AC_MSG_RESULT($PYTHONEPREFIX)
   fi
   PYEPREFIX="$PYTHONEPREFIX"

   AC_MSG_CHECKING(for Python site-dir)
   #This seem to be the site-packages dir where files are installed.
   PYSITEDIR=`($PYTHON -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(plat_specific=1))") 2>/dev/null`
   if test -z "$PYSITEDIR"; then
     # I'm not really sure if it should be installed at /usr/lib64...
     if test -d "$PYEPREFIX/lib$LIBPOSTFIX/$PYVERSION/site-packages"; then
       PYSITEDIR="$PYEPREFIX/lib$LIBPOSTFIX/$PYVERSION/site-packages"
     else
       if test -d "$PYEPREFIX/lib/$PYVERSION/site-packages"; then
         PYSITEDIR="$PYEPREFIX/lib/$PYVERSION/site-packages"
       fi
     fi
   fi
   AC_MSG_RESULT($PYSITEDIR)

else

# python-config not available.
if test -n "$PYTHON"; then
    AC_MSG_CHECKING(for Python prefix)
    PYPREFIX=`($PYTHON -c "import sys; print(sys.prefix)") 2>/dev/null`
    AC_MSG_RESULT($PYPREFIX)
    AC_MSG_CHECKING(for Python exec-prefix)
    PYEPREFIX=`($PYTHON -c "import sys; print(sys.exec_prefix)") 2>/dev/null`
    AC_MSG_RESULT($PYEPREFIX)


    # Note: I could not think of a standard way to get the version string from different versions.
    # This trick pulls it out of the file location for a standard library file.

    AC_MSG_CHECKING(for Python version)

    # Need to do this hack since autoconf replaces __file__ with the name of the configure file
    filehack="file__"
    PYVERSION=`($PYTHON -c "import string,operator,os.path; print(operator.getitem(os.path.split(operator.getitem(os.path.split(string.__$filehack),0)),1))")`
    AC_MSG_RESULT($PYVERSION)

    # Find the directory for libraries this is necessary to deal with
    # platforms that can have apps built for multiple archs: e.g. x86_64
    AC_MSG_CHECKING(for Python lib dir)
    PYLIBDIR=`($PYTHON -c "import sys; print(sys.lib)") 2>/dev/null`
    if test -z "$PYLIBDIR"; then
      # older versions don't have sys.lib  so the best we can do is assume lib
      #PYLIBDIR="lib$LIBPOSTFIX"

      if test -r $PYPREFIX/include/$PYVERSION/Python.h; then
        if test -d "$PYEPREFIX/lib$LIBPOSTFIX/$PYVERSION/config"; then
	  PYLIBDIR="lib$LIBPOSTFIX"
        else
	  if test -d "$PYEPREFIX/lib/$PYVERSION/config"; then
	    # for some reason a 64bit system could have libs installed at lib
	    PYLIBDIR="lib"
	  else
	    # I doubt this will work
	    PYLIBDIR="lib$LIBPOSTFIX"
	  fi
        fi
      else
	# probably very old installation...
	PYLIBDIR="lib"
      fi
    fi
    AC_MSG_RESULT($PYLIBDIR)

    AC_MSG_CHECKING(for Python site-dir)
    PYSITEDIR=`($PYTHON -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(plat_specific=1))") 2>/dev/null`
    if test -z "$PYSITEDIR"; then
      # I'm not really sure if it should be installed at /usr/lib64...
      if test -d "$PYEPREFIX/lib$LIBPOSTFIX/$PYVERSION/site-packages"; then
        PYSITEDIR="$PYEPREFIX/lib$LIBPOSTFIX/$PYVERSION/site-packages"
      else
        if test -d "$PYEPREFIX/lib/$PYVERSION/site-packages"; then
          PYSITEDIR="$PYEPREFIX/lib/$PYVERSION/site-packages"
        fi
      fi
    fi
    AC_MSG_RESULT($PYSITEDIR)

   
    # Set the include directory

    AC_MSG_CHECKING(for Python header files)
    if test -r $PYPREFIX/include/$PYVERSION/Python.h; then
        PYCFLAGS="-I$PYPREFIX/include/$PYVERSION -I$PYEPREFIX/$PYLIBDIR/$PYVERSION/config"
    fi
    if test -z "$PYCFLAGS"; then
        if test -r $PYPREFIX/include/Py/Python.h; then
            PYCFLAGS="-I$PYPREFIX/include/Py -I$PYEPREFIX/$PYLIBDIR/python/lib"
        fi
    fi
    AC_MSG_RESULT($PYCFLAGS)

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

    AC_MSG_CHECKING(for Python LDFLAGS)
    # Check for really old versions
    if test -r $PYLIB/libPython.a; then
         PYLINK="-lModules -lPython -lObjects -lParser"
    else
	if test ! -r $PYLIB/lib$PYVERSION.so -a -r $PYLIB/lib$PYVERSION.a ; then
		# python2.2 on FC1 need this
		PYLINK="$PYLIB/lib$PYVERSION.a"
	else
		PYLINK="-l$PYVERSION"
	fi
    fi
    PYLDFLAGS="$PYLINK"
    AC_MSG_RESULT($PYLDFLAGS)
fi
fi

# Cygwin (Windows) needs the library for dynamic linking
case $host in
*-*-cygwin* | *-*-mingw*) PYTHONDYNAMICLINKING="-L$PYLIB $PYLINK"
         DEFS="-DUSE_DL_IMPORT $DEFS" PYCFLAGS="$PYCFLAGS"
         ;;
*)PYTHONDYNAMICLINKING="";;
esac
fi

])
