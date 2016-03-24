#include "ow_testhelper.h"

// Configure a fake LCD device
#define LCD_ADDR "FF.AAAAAA000000"
static void add_lcd_device() {
	// ow_lcd.c device
	const BYTE addr[] = {0xFF,0xAA,0xAA,0xAA,0x00,0x00,0x00,0xA9};
	ck_assert_int_eq(gbGOOD, Cache_Add_Device(0, addr));
}


// Fake line write function
static ZERO_OR_ERROR FS_w_lineX_FAKE(struct one_wire_query *owq) {

}

// Setup a query for the above LCD device, property /line20.ALL
static void setup_lcd_line20ALL_query() {
	add_lcd_device();
	owq = owmalloc(sizeof(struct one_wire_query));
	memset(owq, 0, sizeof(struct one_wire_query));
	ck_assert_int_eq(gbGOOD, OWQ_create("/" LCD_ADDR "/line20.ALL", owq));
}


// Test that we get EINVAL on empty string
START_TEST(test_FS_input_ascii_array_empty)
{
	setup_lcd_line20ALL_query();

	char buf[20];
	OWQ_assign_write_buffer( buf, 0, 0, owq) ;
	ck_assert_int_eq(-EINVAL, OWQ_parse_input(owq));
}
END_TEST

// Test writing single row to ALL
START_TEST(test_FS_input_ascii_array_single_row)
{
	setup_lcd_line20ALL_query();
	struct parsedname *pn = PN(owq);

	char buf[20];
	memcpy(buf, "aaa", 3);
	OWQ_assign_write_buffer( buf, 3, 0, owq) ;
	ck_assert_int_eq(0, OWQ_parse_input(owq));

	ck_assert_int_eq(EXTENSION_ALL, pn->extension);
	ck_assert_int_eq(3, OWQ_array_length(owq, 0));
	ck_assert_int_eq(0, OWQ_array_length(owq, 1));
	ck_assert_int_eq(0, OWQ_array_length(owq, 2));
	ck_assert_int_eq(0, OWQ_array_length(owq, 3));
	ck_assert(!memcmp("aaa", OWQ_buffer(owq) + 0, 3));
}
END_TEST

// Test writing 4 non-full rows to ALL
START_TEST(test_FS_input_ascii_array_all_rows)
{
	setup_lcd_line20ALL_query();
	struct parsedname *pn = PN(owq);

	char buf[20];
	memcpy(buf, "aaa,bbb,ccc,ddd", 15);
	OWQ_assign_write_buffer( buf, 15, 0, owq) ;
	ck_assert_int_eq(0, OWQ_parse_input(owq));

	ck_assert_int_eq(EXTENSION_ALL, pn->extension);
	ck_assert_int_eq(3, OWQ_array_length(owq, 0));
	ck_assert_int_eq(3, OWQ_array_length(owq, 1));
	ck_assert_int_eq(3, OWQ_array_length(owq, 2));
	ck_assert_int_eq(3, OWQ_array_length(owq, 3));
	ck_assert(!memcmp("aaa", OWQ_buffer(owq) + 0, 3));
	ck_assert(!memcmp("bbb", OWQ_buffer(owq) + 3, 3));
	ck_assert(!memcmp("ccc", OWQ_buffer(owq) + 6, 3));
	ck_assert(!memcmp("ddd", OWQ_buffer(owq) + 9, 3));
}
END_TEST

// Test writing 4 overflowed rows (more data than filetype allows) to ALL
START_TEST(test_FS_input_ascii_array_overflow)
{
	setup_lcd_line20ALL_query();
	struct parsedname *pn = PN(owq);

	char buf[200];
	// Max 20 characters per line; we have 30 each here
	memcpy(buf,
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
			"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,"
			"cccccccccccccccccccccccccccccc,"
			"dddddddddddddddddddddddddddddd", 123);
	OWQ_assign_write_buffer( buf, 123, 0, owq) ;
	ck_assert_int_eq(0, OWQ_parse_input(owq));

	ck_assert_int_eq(EXTENSION_ALL, pn->extension);
	ck_assert_int_eq(20, OWQ_array_length(owq, 0));
	ck_assert_int_eq(20, OWQ_array_length(owq, 1));
	ck_assert_int_eq(20, OWQ_array_length(owq, 2));
	ck_assert_int_eq(20, OWQ_array_length(owq, 3));
	ck_assert(!memcmp("aaaaaaaaaaaaaaaaaaaa", OWQ_buffer(owq) + 0, 20));
	ck_assert(!memcmp("bbbbbbbbbbbbbbbbbbbb", OWQ_buffer(owq) + 20, 20));
	ck_assert(!memcmp("cccccccccccccccccccc", OWQ_buffer(owq) + 40, 20));
	ck_assert(!memcmp("dddddddddddddddddddd", OWQ_buffer(owq) + 60, 20));
}
END_TEST

// Create test-suite
Suite* ow_parseinput_suite(void) {
	Suite *s;
	TCase *tc;

	s = suite_create("Owfs");
	tc = tcase_create("parseinput");

	tcase_add_checked_fixture(tc, owlib_test_setup, owlib_test_teardown);
	suite_add_tcase (s, tc);
	tcase_add_test(tc, test_FS_input_ascii_array_empty);
	tcase_add_test(tc, test_FS_input_ascii_array_all_rows);
	tcase_add_test(tc, test_FS_input_ascii_array_overflow);
	return s;
}
