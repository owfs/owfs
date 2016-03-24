#ifndef OWFS_OWTEST_HELPER_H
#define OWFS_OWTEST_HELPER_H

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#include <check.h>

extern struct one_wire_query* owq;

void owlib_test_setup(void);
void owlib_test_teardown(void);

#endif //OWFS_OWTEST_HELPER_H
