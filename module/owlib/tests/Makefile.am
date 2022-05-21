#if HAVE_CHECK

# Each check_xxx.c file must be added to OWLIB_CHECK_SOURCES
# and must also be called from owlib_test.c
OWLIB_CHECK_SOURCES = check_ow_parseinput.c


# Main entrypoint is owlib_test.
TESTS=owlib_test
check_PROGRAMS = owlib_test
owlib_test_SOURCES = owlib_test.c ow_testhelper.c ow_testhelper.h ${OWLIB_CHECK_SOURCES}

owlib_test_CFLAGS = -I../src/include @CHECK_CFLAGS@
owlib_test_LDADD = ../src/c/libow.la @CHECK_LIBS@

#endif
