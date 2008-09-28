#----------------------------------------------------------------
# Look for Perl5
# $Id$
#----------------------------------------------------------------

AC_DEFUN([SC_PATH_PERL5],
    [AC_PREREQ(2.57)dnl

PERLBIN=

AC_ARG_WITH(perl5, [  --with-perl5            Set location of Perl executable],[ PERLBIN="$withval"], [PERLBIN=yes])


# First, check for "--without-perl5" or "--with-perl5=no".
if test x"${PERLBIN}" = xno -o x"${with_alllang}" = xno ; then 
AC_MSG_NOTICE([Disabling Perl5])
else

# First figure out what the name of Perl5 is

if test "x$PERLBIN" = xyes; then
AC_CHECK_PROGS(PERL, perl perl5.6.1 perl5.6.0 perl5.004 perl5.003 perl5.002 perl5.001 perl5 perl)
else
PERL="$PERLBIN"
fi


# This could probably be simplified as for all platforms and all versions of Perl the following apparently should be run to get the compilation options:
# perl -MExtUtils::Embed -e ccopts
AC_MSG_CHECKING(for Perl5 header files)
if test -n "$PERL"; then
	PERL=`(which $PERL) 2>/dev/null`
	# Make it possible to force a directory during cross-compilation
	if test -z "$PERL5DIR"; then
		PERL5DIR=`($PERL -e 'use Config; print $Config{archlib}, "\n";') 2>/dev/null`
	fi
	if test -n "$PERL5DIR"; then
		dirs="$PERL5DIR $PERL5DIR/CORE"
		PERL5EXT=none
		for i in $dirs; do
			if test -r $i/perl.h; then
				AC_MSG_RESULT($i)
				PERL5EXT="$i"
				break;
			fi
		done
		if test "$PERL5EXT" = none; then
			PERL5EXT="$PERL5DIR/CORE"
			AC_MSG_RESULT(could not locate perl.h...using $PERL5EXT)
		fi

		AC_MSG_CHECKING(for Perl5 library)
		PERL5NAME=`($PERL -e 'use Config; $_=$Config{libperl}; s/^lib//; s/$Config{_a}$//; print $_, "\n"') 2>/dev/null`
		if test "$PERL5NAME" = "" ; then
			AC_MSG_RESULT(not found)
		else
			AC_MSG_RESULT($PERL5NAME)
		fi
		AC_MSG_CHECKING(for Perl5 compiler options)
 		PERL5CCFLAGS=`($PERL -e 'use Config; print $Config{ccflags}, "\n"' | sed "s/-I/$ISYSTEM/") 2>/dev/null`
 		if test "$PERL5CCFLAGS" = "" ; then
 			AC_MSG_RESULT(not found)
 		else
 			AC_MSG_RESULT($PERL5CCFLAGS)
 		fi
	else
		AC_MSG_RESULT(unable to determine perl5 configuration)
		PERL5EXT=$PERL5DIR
	fi
else
       	AC_MSG_RESULT(could not figure out how to run perl5)
fi

# Cygwin (Windows) needs the library for dynamic linking
case $host in
*-*-cygwin* | *-*-mingw*) PERL5DYNAMICLINKING="-L$PERL5EXT -l$PERL5NAME";;
*)PERL5DYNAMICLINKING="";;
esac
fi

])
