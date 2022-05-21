#include "ow_testhelper.h"

#define _DEFINE_SUITE(suite_name) Suite* suite_name(void);
#define _INCLUDE_SUITE(suite_name) srunner_add_suite(runner, suite_name());

/**
 * Add all your test suites here, and in setup_test_suites below
 */

_DEFINE_SUITE(ow_parseinput_suite);

static void setup_test_suites(SRunner *runner) {
	_INCLUDE_SUITE(ow_parseinput_suite);
}

int main(void)
{
	Globals.error_level = e_err_debug ;
	Globals.error_level_restore = e_err_debug ;
	Globals.error_print = e_err_print_console;

	SRunner *sr;

	sr = srunner_create(NULL);

	setup_test_suites(sr);

	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);

	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
