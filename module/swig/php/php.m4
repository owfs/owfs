#-------------------------------------------------------------------------
# Look for PHP
#-------------------------------------------------------------------------

AC_DEFUN([SC_PATH_PHP], [AC_PREREQ(2.57)dnl

AC_ARG_WITH(php,
	[AS_HELP_STRING([--with-php],
		[Set location of php executable])],
	[PHPBIN="$withval"], [PHPBIN=yes])
AC_ARG_WITH(phpconfig,
	[AS_HELP_STRING([--with-phpconfig],
		[Set location of php-config executable])],
	[PHPCONFIGBIN="$withval"], [PHPCONFIGBIN=yes])

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
      AC_CHECK_PROGS(PHPCONFIG,
	php-config php5-config php-config5 php4-config php-config4)
else
      PHPCONFIG="$PHPCONFIGBIN"
fi

AC_MSG_CHECKING(for PHP header files)
PHPINC="$($PHPCONFIG --includes 2>/dev/null)"
if test "$PHPINC"; then
	AC_MSG_RESULT($PHPINC)
else
	AC_MSG_RESULT(not found)
fi

AC_MSG_CHECKING(for PHP extension-dir)
PHPLIBDIR="$($PHPCONFIG --extension-dir 2>/dev/null)"
if test "$PHPLIBDIR"; then
	AC_MSG_RESULT($PHPLIBDIR)
else
	AC_MSG_RESULT(not found)
fi

AC_MSG_CHECKING(for PHP major version)
PHPVMAJOR="$($PHPCONFIG --version 2>/dev/null | cut -f1 -d.)"
if test "$PHPVMAJOR"; then
	AC_MSG_RESULT($PHPVMAJOR)
else
	AC_MSG_RESULT(unable to determine)
fi

fi

])
