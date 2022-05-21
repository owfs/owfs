#include "owftpd.h"

#ifndef NDEBUG
void daemon_assert_fail(const char *assertion, const char *file, int line, const char *function)
{
	syslog(LOG_CRIT, "%s:%d: %s: %s", file, line, function, assertion);
	fprintf(stderr, "%s:%d: %s: %s\n", file, line, function, assertion);
	exit(1);
}
#endif
