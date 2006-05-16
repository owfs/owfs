/*
 * $Id$
 *
 * This data type allows modules to cleanly return error information in a 
 * relatively clean fashion.  It only includes an error number and a
 * description string right now.  It could be modified to include a large
 * number of other data, e.g. module, file/line, timestamp.  I don't
 * need that for my program right now, so I'm going to keep it simple. 
 *
 * -Shane
 */

#include "owftpd.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static int invariant(const struct error_code_s *err);

void error_init(struct error_code_s *err, int error_code, const char *desc_fmt, ...)
{
    va_list args;

    daemon_assert(err != NULL);
    daemon_assert(error_code >= 0);
    daemon_assert(desc_fmt != NULL);

    err->error_code = error_code;
    va_start(args, desc_fmt);
    vsnprintf(err->desc, sizeof(err->desc), desc_fmt, args);
    va_end(args);

    daemon_assert(invariant(err));
}

int error_get_error_code(const struct error_code_s *err)
{
    daemon_assert(invariant(err));
    return err->error_code;
}

const char *error_get_desc(const struct error_code_s *err)
{
    daemon_assert(invariant(err));
    return err->desc;
}

#ifndef NDEBUG
static int invariant(const struct error_code_s *err)
{
    if (err == NULL) {
        return 0;
    }
    if (err->error_code < 0) {
        return 0;
    }
    if (strlen(err->desc) >= sizeof(err->desc)) {
        return 0;
    }
    return 1;
}
#endif /* NDEBUG */

