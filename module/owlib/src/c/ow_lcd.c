/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_lcd.h"

/* ------- Prototypes ----------- */

/* LCD display */
READ_FUNCTION(FS_r_version);
READ_FUNCTION(FS_r_counters);
READ_FUNCTION(FS_r_gpio);
WRITE_FUNCTION(FS_w_gpio);
READ_FUNCTION(FS_r_data);
WRITE_FUNCTION(FS_w_data);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_register);
WRITE_FUNCTION(FS_w_register);
WRITE_FUNCTION(FS_simple_command);
#if OW_CACHE
READ_FUNCTION(FS_r_cum);
WRITE_FUNCTION(FS_w_cum);
#endif							/* OW_CACHE */
WRITE_FUNCTION(FS_w_screenX);
WRITE_FUNCTION(FS_w_lineX);

/* ------- Device Constants ----- */
#define _LCD_COMMAND_POWER_OFF   0x05
#define _LCD_COMMAND_POWER_ON    0x03
#define _LCD_COMMAND_BACKLIGHT_OFF   0x07
#define _LCD_COMMAND_BACKLIGHT_ON    0x08
#define _LCD_COMMAND_REGISTER_TO_SCRATCH    0x11
#define _LCD_COMMAND_REGISTER_WRITE_BYTE   0x10
#define _LCD_COMMAND_DATA_TO_SCRATCH    0x13
#define _LCD_COMMAND_DATA_WRITE_BYTE   0x12
#define _LCD_COMMAND_SCRATCHPAD_READ   0xBE
#define _LCD_COMMAND_SCRATCHPAD_WRITE  0x4E
#define _LCD_COMMAND_PRINT_FROM_SCRATCH  0x48
#define _LCD_COMMAND_EEPROM_READ_TO_SCRATCH    0x37
#define _LCD_COMMAND_EEPROM_WRITE_FROM_SCRATCH   0x39
#define _LCD_COMMAND_GPIO_READ_TO_SCRATCH    0x22
#define _LCD_COMMAND_GPIO_WRITE_BYTE   0x21
#define _LCD_COMMAND_COUNTER_READ_TO_SCRATCH   0x23
#define _LCD_COMMAND_LCD_CLEAR   0x49
#define _LCD_COMMAND_VERSION_READ_TO_SCRATCH   0x41

#define _LCD_PAGE_SIZE      16

#define PACK_ON_OFF(on,off) (((unsigned int)((BYTE)(on))<<8) | ((unsigned int)((BYTE)(off))))
#define UNPACK_ON(u)    ((BYTE)((u)>>8))
#define UNPACK_OFF(u)    ((BYTE)((u)&0xFF))
/* ------- Structures ----------- */

static struct aggregate ALCD = { 4, ag_numbers, ag_aggregate, };
static struct aggregate ALCD_L16 = { 4, ag_numbers, ag_separate, };
static struct aggregate ALCD_L20 = { 4, ag_numbers, ag_separate, };
static struct aggregate ALCD_L40 = { 2, ag_numbers, ag_separate, };
static struct filetype LCD[] = {
	F_STANDARD,
	{"memory", 112, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},

	{"LCDon", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_simple_command, VISIBLE, {u:PACK_ON_OFF(_LCD_COMMAND_POWER_ON, _LCD_COMMAND_POWER_OFF)},},
	{"backlight", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_simple_command, VISIBLE, {u:PACK_ON_OFF(_LCD_COMMAND_BACKLIGHT_ON, _LCD_COMMAND_BACKLIGHT_OFF)},},
	{"version", _LCD_PAGE_SIZE, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"gpio", PROPERTY_LENGTH_BITFIELD, &ALCD, ft_bitfield, fc_volatile, FS_r_gpio, FS_w_gpio, VISIBLE, NO_FILETYPE_DATA,},
	{"register", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_register, FS_w_register, VISIBLE, NO_FILETYPE_DATA,},
	{"data", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, FS_w_data, VISIBLE, NO_FILETYPE_DATA,},
	{"counters", PROPERTY_LENGTH_UNSIGNED, &ALCD, ft_unsigned, fc_volatile, FS_r_counters, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
#if OW_CACHE
	{"cumulative", PROPERTY_LENGTH_UNSIGNED, &ALCD, ft_unsigned, fc_volatile, FS_r_cum, FS_w_cum, VISIBLE, NO_FILETYPE_DATA,},
#endif							/*OW_CACHE */
	{"screen16", 128, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_screenX, VISIBLE, {i:16},},
	{"screen20", 128, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_screenX, VISIBLE, {i:20},},
	{"screen40", 128, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_screenX, VISIBLE, {i:40},},
	{"line16", 16, &ALCD_L16, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_lineX, VISIBLE, {i:16},},
	{"line20", 20, &ALCD_L20, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_lineX, VISIBLE, {i:20},},
	{"line40", 40, &ALCD_L40, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_w_lineX, VISIBLE, {i:40},},
};

DeviceEntryExtended(FF, LCD, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */

/* LCD by L. Swart */
static GOOD_OR_BAD OW_r_scratch(BYTE * data, int length, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_scratch(const BYTE * data, int length, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_register(BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_register(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_data(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_data(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_gpio(BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_gpio(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_version(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_counters(UINT * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_memory(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_memory(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_clear(const struct parsedname *pn);
static GOOD_OR_BAD OW_w_screen(BYTE lcd_location, const char *text, int size, const struct parsedname *pn);
static GOOD_OR_BAD LCD_byte(BYTE b, int delay, const struct parsedname *pn);
static GOOD_OR_BAD LCD_2byte(BYTE * bytes, int delay, const struct parsedname *pn);
static GOOD_OR_BAD OW_simple_command(BYTE lcd_command_code, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_unpaged_to_screen(BYTE lcd_location, BYTE length, const char *text, const struct parsedname *pn);

/* Internal files */
Make_SlaveSpecificTag(CUM, fc_persistent);	//cumulative

/* LCD */
static ZERO_OR_ERROR FS_simple_command(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT lcd_command_pair = (UINT) pn->selected_filetype->data.u;
	BYTE lcd_command_code = OWQ_Y(owq) ? UNPACK_ON(lcd_command_pair) : UNPACK_OFF(lcd_command_pair);
	return GB_to_Z_OR_E(OW_simple_command(lcd_command_code, pn)) ;
}

static ZERO_OR_ERROR FS_r_version(struct one_wire_query *owq)
{
	BYTE v[_LCD_PAGE_SIZE];
	RETURN_ERROR_IF_BAD( OW_r_version(v, PN(owq)) ) ;
	return OWQ_format_output_offset_and_size((ASCII *) v, _LCD_PAGE_SIZE, owq);
}

static ZERO_OR_ERROR FS_r_gpio(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD( OW_r_gpio(&data, PN(owq)) );
	OWQ_U(owq) = (~data) & 0x0F;
	return 0;
}

/* 4 value array */
static ZERO_OR_ERROR FS_w_gpio(struct one_wire_query *owq)
{
	BYTE data = ~OWQ_U(owq) & 0x0F;
	/* Now set pins */
	return GB_to_Z_OR_E(OW_w_gpio(data, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_register(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD( OW_r_register(&data, PN(owq)) ) ;
	OWQ_U(owq) = data;
	return 0;
}

static ZERO_OR_ERROR FS_w_register(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_register((BYTE) (BYTE_MASK(OWQ_U(owq))), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_data(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD( OW_r_data(&data, PN(owq)) ) ;
	OWQ_U(owq) = data;
	return 0;
}

static ZERO_OR_ERROR FS_w_data(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_data((BYTE) (BYTE_MASK(OWQ_U(owq))), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_counters(struct one_wire_query *owq)
{
	UINT u[4];
	RETURN_ERROR_IF_BAD( OW_r_counters(u, PN(owq)) ) ;
	OWQ_array_U(owq, 0) = u[0];
	OWQ_array_U(owq, 1) = u[1];
	OWQ_array_U(owq, 2) = u[2];
	OWQ_array_U(owq, 3) = u[3];
	return 0;
}

#if OW_CACHE					/* Special code for cumulative counters -- read/write -- uses the caching system for storage */
static ZERO_OR_ERROR FS_r_cum(struct one_wire_query *owq)
{
	UINT u[4];
	
	/* just to prime the "CUM" data */
	RETURN_ERROR_IF_BAD( OW_r_counters(u, PN(owq)) ) ;

	if ( BAD( Cache_Get_SlaveSpecific((void *) u, 4 * sizeof(UINT), SlaveSpecificTag(CUM), PN(owq))) ) {
		return -EINVAL;
	}
	OWQ_array_U(owq, 0) = u[0];
	OWQ_array_U(owq, 1) = u[1];
	OWQ_array_U(owq, 2) = u[2];
	OWQ_array_U(owq, 3) = u[3];
	return 0;
}

static ZERO_OR_ERROR FS_w_cum(struct one_wire_query *owq)
{
	UINT u[4] = {
		OWQ_array_U(owq, 0),
		OWQ_array_U(owq, 1),
		OWQ_array_U(owq, 2),
		OWQ_array_U(owq, 3),
	};
	return GB_to_Z_OR_E( Cache_Add_SlaveSpecific((const void *) u, 4 * sizeof(UINT), SlaveSpecificTag(CUM), PN(owq)) );
}
#endif							/*OW_CACHE */

static ZERO_OR_ERROR FS_w_lineX(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int width = pn->selected_filetype->data.i;
	BYTE lcd_line_start[] = { 0x00, 0x40, 0x00 + width, 0x40 + width };
	char line[width];
	size_t size = OWQ_size(owq);
	int start = OWQ_offset(owq);

	if (start >= width) {
		return -EADDRNOTAVAIL;
	}
	if ((int) (start + size) > width) {
		size = width - start;
	}
	memset(line, ' ', width);
	memcpy(&line[start], OWQ_buffer(owq), size);
	return GB_to_Z_OR_E(OW_w_screen(lcd_line_start[pn->extension], line, width, pn)) ;
}

static ZERO_OR_ERROR FS_w_screenX(struct one_wire_query *owq)
{
	struct one_wire_query * owq_line ;
	int extension;

	struct parsedname *pn = PN(owq);

	int width = pn->selected_filetype->data.i;
	int rows = (width == 40) ? 2 : 4;	/* max number of rows */
	char *start_of_remaining_text = OWQ_buffer(owq);
	char *pointer_after_all_text = OWQ_buffer(owq) + OWQ_size(owq);

	if (OWQ_offset(owq)) {
		return -ERANGE;
	}

	if (BAD( OW_clear(pn) ) ) {
		return -EFAULT;
	}

	owq_line = OWQ_create_separate( 0, owq ) ;
	if ( owq_line == NO_ONE_WIRE_QUERY ) {
		return -ENOMEM ;
	}

	for (extension = 0; extension < rows; ++extension) {
		char *newline_location = memchr(start_of_remaining_text, '\n',
										pointer_after_all_text - start_of_remaining_text);
		OWQ_pn(owq_line).extension = extension;
		OWQ_buffer(owq_line) = start_of_remaining_text;
		if ((newline_location != NULL)
			&& (newline_location < start_of_remaining_text + width)) {
			OWQ_size(owq_line) = newline_location - start_of_remaining_text;
			start_of_remaining_text = newline_location + 1;	/* skip over newline */
		} else {
			char *lineend_location = start_of_remaining_text + width;
			if (lineend_location > pointer_after_all_text) {
				lineend_location = pointer_after_all_text;
			}
			OWQ_size(owq_line) = lineend_location - start_of_remaining_text;
			start_of_remaining_text = lineend_location;
		}
		if (FS_w_lineX(owq_line)) {
			OWQ_destroy( owq_line ) ;
			return -EINVAL;
		}
		if (start_of_remaining_text >= pointer_after_all_text)
			break;
	}
	OWQ_destroy( owq_line ) ;
	return 0;
}

static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, _LCD_PAGE_SIZE, OW_r_memory)) ;
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, _LCD_PAGE_SIZE, OW_w_memory)) ;
}

static GOOD_OR_BAD OW_w_scratch(const BYTE * data, int length, const struct parsedname *pn)
{
	BYTE write_command[1] = { _LCD_COMMAND_SCRATCHPAD_WRITE, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(write_command),
		TRXN_WRITE(data, length),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_r_scratch(BYTE * data, int length, const struct parsedname *pn)
{
	BYTE read_command[1] = { _LCD_COMMAND_SCRATCHPAD_READ, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(read_command),
		TRXN_READ(data, length),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_w_register(BYTE data, const struct parsedname *pn)
{
	BYTE w[] = { _LCD_COMMAND_REGISTER_WRITE_BYTE, data, };
	// 100uS
	return LCD_2byte(w, 1, pn);
}

static GOOD_OR_BAD OW_r_register(BYTE * data, const struct parsedname *pn)
{
	// 150uS
	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_REGISTER_TO_SCRATCH, 1, pn)) ;
	return OW_r_scratch(data, 1, pn) ;
}

static GOOD_OR_BAD OW_w_data(const BYTE data, const struct parsedname *pn)
{
	BYTE w[] = { _LCD_COMMAND_DATA_WRITE_BYTE, data, };
	// 100uS
	return LCD_2byte(w, 1, pn);
}

static GOOD_OR_BAD OW_r_data(BYTE * data, const struct parsedname *pn)
{
	// 150uS
	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_DATA_TO_SCRATCH, 1, pn)) ;
	return OW_r_scratch(data, 1, pn) ;
}

static GOOD_OR_BAD OW_w_gpio(BYTE data, const struct parsedname *pn)
{
	/* Note, it would be nice to control separately, nut
	   we can't know the set state of the pin, i.e. sensed and set
	   are confused */
	/* Datasheet says bit 7 should be 1 */
	BYTE w[] = { _LCD_COMMAND_GPIO_WRITE_BYTE, 0x80 | data, };
	// 20uS
	return LCD_2byte(w, 1, pn);
}

static GOOD_OR_BAD OW_r_gpio(BYTE * data, const struct parsedname *pn)
{
	// 70uS
	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_GPIO_READ_TO_SCRATCH, 1, pn)) ;
	return OW_r_scratch(data, 1, pn) ;
}

static GOOD_OR_BAD OW_r_counters(UINT * data, const struct parsedname *pn)
{
	BYTE d[8];
	UINT cum[4];

	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_COUNTER_READ_TO_SCRATCH, 1, pn)) ;
	RETURN_BAD_IF_BAD( OW_r_scratch(d, 8, pn)) ;
	
	data[0] = ((UINT) d[1]) << 8 | d[0];
	data[1] = ((UINT) d[3]) << 8 | d[2];
	data[2] = ((UINT) d[5]) << 8 | d[4];
	data[3] = ((UINT) d[7]) << 8 | d[6];

//printf("OW_COUNTER key=%s\n",key);
	if ( BAD( Cache_Get_SlaveSpecific((void *) cum, sizeof(cum), SlaveSpecificTag(CUM), pn)) ) {	/* First pass at cumulative */
		cum[0] = data[0];
		cum[1] = data[1];
		cum[2] = data[2];
		cum[3] = data[3];
	} else {
		cum[0] += data[0];
		cum[1] += data[1];
		cum[2] += data[2];
		cum[3] += data[3];
	}
	Cache_Add_SlaveSpecific((void *) cum, sizeof(cum), SlaveSpecificTag(CUM), pn);
	return gbGOOD;
}

/* memory is 112 bytes */
/* can only read 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static GOOD_OR_BAD OW_r_memory(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE location_setup[2] = { BYTE_MASK(offset), BYTE_MASK(size), };

	// 500uS
	RETURN_BAD_IF_BAD( OW_w_scratch(location_setup, 2, pn)) ;
	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_EEPROM_READ_TO_SCRATCH, 1, pn)) ;
	return OW_r_scratch(data, BYTE_MASK(size), pn);
}

/* memory is 112 bytes */
/* can only write 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static GOOD_OR_BAD OW_w_memory(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE location_and_buffer[1 + _LCD_PAGE_SIZE] = { BYTE_MASK(offset), };

	if (size == 0) {
		return gbGOOD;
	}
	memcpy(&location_and_buffer[1], data, size);

	// 4mS/byte
	RETURN_BAD_IF_BAD(OW_w_scratch(location_and_buffer, size + 1, pn)) ;
	return LCD_byte(_LCD_COMMAND_EEPROM_WRITE_FROM_SCRATCH, 4 * size, pn) ;
}

/* data is 16 bytes */
static GOOD_OR_BAD OW_r_version(BYTE * data, const struct parsedname *pn)
{
	// 500uS
	RETURN_BAD_IF_BAD( LCD_byte(_LCD_COMMAND_VERSION_READ_TO_SCRATCH, 1, pn)) ;
	return OW_r_scratch(data, _LCD_PAGE_SIZE, pn) ;
}

static GOOD_OR_BAD OW_w_screen(BYTE lcd_location, const char *text, int size, const struct parsedname *pn)
{
	int chars_left_to_print = size;

	while (chars_left_to_print > 0) {
		int current_index_in_buffer;
		int chars_printing_now = chars_left_to_print;
		if (chars_printing_now > _LCD_PAGE_SIZE) {
			chars_printing_now = _LCD_PAGE_SIZE;
		}
		current_index_in_buffer = size - chars_left_to_print;
		RETURN_BAD_IF_BAD( OW_w_unpaged_to_screen(lcd_location + current_index_in_buffer, chars_printing_now, &text[current_index_in_buffer], pn) ) ;
		chars_left_to_print -= chars_printing_now;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_unpaged_to_screen(BYTE lcd_location, BYTE length, const char *text, const struct parsedname *pn)
{
	BYTE location_and_string[1 + _LCD_PAGE_SIZE] = { lcd_location, };
	memcpy(&location_and_string[1], text, length);
	RETURN_BAD_IF_BAD( OW_w_scratch(location_and_string, 1 + length, pn) ) ;
	return LCD_byte(_LCD_COMMAND_PRINT_FROM_SCRATCH, 2, pn) ;
}

static GOOD_OR_BAD OW_clear(const struct parsedname *pn)
{
	/* clear */
	return LCD_byte(_LCD_COMMAND_LCD_CLEAR, 3, pn);
	// 2.5mS
}

static GOOD_OR_BAD OW_simple_command(BYTE lcd_command_code, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&lcd_command_code),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD LCD_byte(BYTE b, int delay, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&b),
		TRXN_DELAY(delay),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD LCD_2byte(BYTE * bytes, int delay, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(bytes),
		TRXN_DELAY(delay),
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}
