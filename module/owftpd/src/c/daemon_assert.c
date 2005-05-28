#include <owftpd.h>

#ifndef NDEBUG
void daemon_assert_fail(const char *assertion,
                        const char *file,
                        int line,
                        const char *function)
{
    LEVEL_DEFAULT("%s:%d: %s: %s\n", file, line, function, assertion);
    ow_exit(1);
}
#endif

