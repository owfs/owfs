#ifndef DAEMON_ASSERT_H
#define DAEMON_ASSERT_H

#ifdef NDEBUG

#define daemon_assert(expr)

#else

void daemon_assert_fail(const char *assertion,
                        const char *file,
                        int line,
                        const char *function);

#define daemon_assert(expr)                                                   \
           ((expr) ? 0 :                                                      \
            (daemon_assert_fail(__STRING(expr), __FILE__, __LINE__, __func__)))

#endif

#endif /* DAEMON_ASSERT_H */
