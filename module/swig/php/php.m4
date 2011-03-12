#-------------------------------------------------------------------------
# Look for Php5 or Php4
#-------------------------------------------------------------------------

AC_DEFUN([SC_PATH_PHP],
    [AC_PREREQ(2.57)dnl

PHP4BIN=

#AC_ARG_WITH(php4, AS_HELP_STRING([--without-php4], [Disable PHP4])
#AS_HELP_STRING([--with-php4=path], [Set location of PHP4 executable]),[ PHP4BIN="$withval"], [PHP4BIN=yes])
AC_ARG_WITH(php, [  --with-php              Set location of Php executable],[ PHPBIN="$withval"], [PHPBIN=yes])
AC_ARG_WITH(phpconfig, [  --with-phpconfig        Set location of php-config executable],[ PHPCONFIGBIN="$withval"], [PHPCONFIGBIN=yes])

# First, check for "--without-php" or "--with-php=no".
if test x"${PHPBIN}" = xno -o x"${with_alllang}" = xno ; then 
      AC_MSG_NOTICE([Disabling PHP])
else

if test "x$PHPBIN" = xyes; then
      AC_CHECK_PROGS(PHP, php php5 php4)
else
      PHP="$PHPBIN"
fi

if test "x$PHPCONFIGBIN" = xyes; then
      AC_CHECK_PROGS(PHPCONFIG, php-config php5-config php-config5 php4-config php-config4)
else
      PHPCONFIG="$PHPCONFIGBIN"
fi


AC_MSG_CHECKING(for PHP header files)
PHPINC="`$PHPCONFIG --includes 2>/dev/null`"
if test "$PHPINC"; then
	AC_MSG_RESULT($PHPINC)
else
	dirs="/usr/include/php /usr/local/include/php /usr/include/php5 /usr/local/include/php5 /usr/include/php4 /usr/local/include/php4 /usr/local/apache/php"
	for i in $dirs; do
		echo $i
		if test -r $i/main/php_config.h -o -r $i/main/php_version.h; then
			AC_MSG_RESULT($i is found)
			PHPEXT="$i"
			PHPINC="-I$PHPEXT -I$PHPEXT/main -I$PHPEXT/TSRM -I$PHPEXT/Zend"
			break;
		fi
	done
fi
if test -z "$PHPINC"; then
	AC_MSG_RESULT(not found)
fi

AC_MSG_CHECKING(for PHP extension-dir)
PHPLIBDIR="`$PHPCONFIG --extension-dir 2>/dev/null`"
if test ! -z "$PHPLIBDIR"; then
	AC_MSG_RESULT($PHPLIBDIR)
else
	AC_MSG_RESULT(not found)
fi

fi

])
