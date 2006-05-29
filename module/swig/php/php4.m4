#-------------------------------------------------------------------------
# Look for Php4
#-------------------------------------------------------------------------

AC_DEFUN([SC_PATH_PHP4],
    [AC_PREREQ(2.57)dnl

PHP4BIN=

#AC_ARG_WITH(php4, AS_HELP_STRING([--without-php4], [Disable PHP4])
#AS_HELP_STRING([--with-php4=path], [Set location of PHP4 executable]),[ PHP4BIN="$withval"], [PHP4BIN=yes])
AC_ARG_WITH(python, [  --with-php4             Set location of Php executable],[ PHP4BIN="$withval"], [PHP4BIN=yes])

# First, check for "--without-php4" or "--with-php4=no".
if test x"${PHP4BIN}" = xno -o x"${with_alllang}" = xno ; then 
      AC_MSG_NOTICE([Disabling PHP4])
else

if test "x$PHP4BIN" = xyes; then
      AC_CHECK_PROGS(PHP4, php php4)
else
      PHP4="$PHP4BIN"
fi

AC_MSG_CHECKING(for PHP4 header files)
PHP4CONFIG="${PHP4}-config"
PHP4INC="`$PHP4CONFIG --includes 2>/dev/null`"
if test "$PHP4INC"; then
	AC_MSG_RESULT($PHP4INC)
else
	dirs="/usr/include/php /usr/local/include/php /usr/include/php4 /usr/local/include/php4 /usr/local/apache/php"
	for i in $dirs; do
		echo $i
		if test -r $i/php_config.h -o -r $i/php_version.h; then
			AC_MSG_RESULT($i)
			PHP4EXT="$i"
			PHP4INC="-I$PHP4EXT -I$PHP4EXT/Zend -I$PHP4EXT/main -I$PHP4EXT/TSRM"
			break;
		fi
	done
fi
if test -z "$PHP4INC"; then
	AC_MSG_RESULT(not found)
fi

AC_MSG_CHECKING(for PHP4 extension-dir)
PHP4LIBDIR="`$PHP4CONFIG --extension-dir 2>/dev/null`"
if test ! -z "$PHP4LIBDIR"; then
	AC_MSG_RESULT($PHP4LIBDIR)
else
	AC_MSG_RESULT(not found)
fi

fi

])
