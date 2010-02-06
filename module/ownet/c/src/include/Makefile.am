headerdir = ${prefix}/include
header_DATA = ownetapi.h

OWLIB_DIR = ../../../../owlib
OWLIB_DIR_C = ${OWLIB_DIR}/src/c
OWLIB_DIR_H = ${OWLIB_DIR}/src/include

EXTRA_DIST = ${header_DATA} \
        ${OWLIB_DIR_H}/compat_getopt.h   \
        ${OWLIB_DIR_H}/compat.h          \
        ${OWLIB_DIR_H}/compat_netdb.h    \
        ${OWLIB_DIR_H}/ow_charblob.h     \
        ow_connection.h   \
        ow_debug.h        \
        ow_dl.h           \
        ow_dnssd.h        \
        ow_functions.h    \
        ow_global.h       \
        ow.h              \
        ow_localtypes.h   \
        ${OWLIB_DIR_H}/ow_message.h      \
        ow_mutexes.h      \
        ownetapi.h        \
        ow_server.h       \
        ow_stub.h         \
        ${OWLIB_DIR_H}/rwlock.h          \
        ${OWLIB_DIR_H}/sem.h

clean-generic:

	@RM@ -f *~ .*~
