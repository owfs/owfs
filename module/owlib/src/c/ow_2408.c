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

/* LCD drivers, two designs
   Maxim / AAG uses 7 PIO pins
   based on Public domain code from Application Note 3286

   Hobby-Boards by Eric Vickery
   Paul,

Go right ahead and use it for whatever you want. I just provide it as an
example for people who are using the LCD Driver.

It originally came from an application that I was working on (and may
again) but that particular code is in the public domain now.

Let me know if you have any other questions.

Eric
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2408.h"

/* ------- Prototypes ----------- */

/* DS2408 switch */
READ_FUNCTION(FS_r_strobe);
WRITE_FUNCTION(FS_w_strobe);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_power);
READ_FUNCTION(FS_r_latch);
WRITE_FUNCTION(FS_w_latch);
READ_FUNCTION(FS_r_s_alarm);
WRITE_FUNCTION(FS_w_s_alarm);
READ_FUNCTION(FS_r_por);
WRITE_FUNCTION(FS_w_por);
WRITE_FUNCTION(FS_Mclear);
WRITE_FUNCTION(FS_Mhome);
WRITE_FUNCTION(FS_Mscreen);
WRITE_FUNCTION(FS_Mmessage);
WRITE_FUNCTION(FS_Hclear);
WRITE_FUNCTION(FS_Hhome);
WRITE_FUNCTION(FS_Hscreen);
WRITE_FUNCTION(FS_Hscreenyx);
WRITE_FUNCTION(FS_Hmessage);
WRITE_FUNCTION(FS_Honoff);

#define PROPERTY_LENGTH_LCD_MESSAGE   128

/* ------- Structures ----------- */

struct aggregate A2408 = { 8, ag_numbers, ag_aggregate, };
struct filetype DS2408[] = {
	F_STANDARD,
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"PIO", PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_stable, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_volatile, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"latch", PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_volatile, FS_r_latch, FS_w_latch, VISIBLE, NO_FILETYPE_DATA,},
	{"strobe", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_strobe, FS_w_strobe, VISIBLE, NO_FILETYPE_DATA,},
	{"set_alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_s_alarm, FS_w_s_alarm, VISIBLE, NO_FILETYPE_DATA,},
	{"por", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_por, FS_w_por, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_M", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_stable, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_M/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_Mclear, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_M/home", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_Mhome, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_M/screen", PROPERTY_LENGTH_LCD_MESSAGE, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_Mscreen, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_M/message", PROPERTY_LENGTH_LCD_MESSAGE, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_Mmessage, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_stable, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_Hclear, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/home", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_Hhome, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/screen", PROPERTY_LENGTH_LCD_MESSAGE, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_Hscreen, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/screenyx", PROPERTY_LENGTH_LCD_MESSAGE, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_Hscreenyx, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/message", PROPERTY_LENGTH_LCD_MESSAGE, NON_AGGREGATE, ft_ascii, fc_stable, NO_READ_FUNCTION, FS_Hmessage, VISIBLE, NO_FILETYPE_DATA,},
	{"LCD_H/onoff", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_Honoff, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(29, DS2408, DEV_alarm | DEV_resume | DEV_ovdr);

#define _1W_READ_PIO_REGISTERS  0xF0
#define _1W_CHANNEL_ACCESS_READ 0xF5
#define _1W_CHANNEL_ACCESS_WRITE 0x5A
#define _1W_WRITE_CONDITIONAL_SEARCH_REGISTER 0xCC
#define _1W_RESET_ACTIVITY_LATCHES 0xC3

#define _ADDRESS_PIO_LOGIC_STATE 0x0088
#define _ADDRESS_CONTROL_STATUS_REGISTER 0x008D

/* Internal properties */
//static struct internal_prop ip_init = { "INI", fc_stable };
MakeInternalProp(INI, fc_stable);	// LCD screen initialized?

/* Nibbles for LCD controller */
#define LCD_DATA_FLAG       0x08
#define NIBBLE_ONE(x)       ((x)&0xF0)
#define NIBBLE_TWO(x)       (((x)<<4)&0xF0)
#define NIBBLE_CTRL( x )    NIBBLE_ONE(x)               , NIBBLE_TWO(x)
#define NIBBLE_DATA( x )    NIBBLE_ONE(x)|LCD_DATA_FLAG , NIBBLE_TWO(x)|LCD_DATA_FLAG

/* ------- Functions ------------ */

/* DS2408 */
static GOOD_OR_BAD OW_w_control(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_c_latch(const struct parsedname *pn);
static GOOD_OR_BAD OW_w_pio(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_reg(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_s_alarm(const BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_pios(const BYTE * data, const size_t size, const struct parsedname *pn);

/* 2408 switch */
/* 2408 switch -- is Vcc powered?*/
static ZERO_OR_ERROR FS_power(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_Y(owq) = UT_getbit(&data[5], 7);
	return 0;
}

static ZERO_OR_ERROR FS_r_strobe(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_Y(owq) = UT_getbit(&data[5], 2);
	return 0;
}

static ZERO_OR_ERROR FS_w_strobe(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE data[6];
	if ( BAD( OW_r_reg(data, pn) ) ) {
		return -EINVAL;
	}
	UT_setbit(&data[5], 2, OWQ_Y(owq));
	return RETURN_Z_OR_E( OW_w_control(data[5], pn) ) ;
}

static ZERO_OR_ERROR FS_Mclear(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int init = 1;

	if (Cache_Get_Internal_Strict(&init, sizeof(init), InternalProp(INI), pn)) {
		OWQ_Y(owq) = 1;
		if ( FS_r_strobe(owq) != 0	// set reset pin to strobe mode
			|| BAD( OW_w_pio(0x30, pn) ) ) {
			return -EINVAL;
		}
		UT_delay(100);
		// init
		if ( BAD( OW_w_pio(0x38, pn) ) ) {
			return -EINVAL;
		}
		UT_delay(10);
		// Enable Display, Cursor, and Blinking
		// Entry-mode: auto-increment, no shift
		if ( BAD( OW_w_pio(0x0F, pn) )  || BAD( OW_w_pio(0x06, pn) ) ) {
			return -EINVAL;
		}
		Cache_Add_Internal(&init, sizeof(init), InternalProp(INI), pn);
	}
	// clear
	if ( BAD( OW_w_pio(0x01, pn) ) ) {
		return -EINVAL;
	}
	UT_delay(2);
	return FS_Mhome(owq);
}

static ZERO_OR_ERROR FS_Mhome(struct one_wire_query *owq)
{
// home
	if ( BAD( OW_w_pio(0x02, PN(owq)) ) ) {
		return -EINVAL;
	}
	UT_delay(2);
	return 0;
}

static ZERO_OR_ERROR FS_Mscreen(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t size = OWQ_size(owq);
	BYTE data[size];
	size_t i;
	for (i = 0; i < size; ++i) {
		// no upper ascii chars
		if (OWQ_buffer(owq)[i] & 0x80) {
			return -EINVAL;
		}
		data[i] = OWQ_buffer(owq)[i] | 0x80;
	}
	return RETURN_Z_OR_E( OW_w_pios(data, size, pn) ) ;
}

static ZERO_OR_ERROR FS_Mmessage(struct one_wire_query *owq)
{
	if (FS_Mclear(owq)) {
		return -EINVAL;
	}
	return FS_Mscreen(owq);
}

/* 2408 switch PIO sensed*/
/* From register 0x88 */
static ZERO_OR_ERROR FS_sense(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_U(owq) = data[0];
	return 0;
}

/* 2408 switch PIO set*/
/* From register 0x89 */
static ZERO_OR_ERROR FS_r_pio(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_U(owq) = BYTE_INVERSE(data[1]);	/* reverse bits */
	return 0;
}

/* 2408 switch PIO change*/
static ZERO_OR_ERROR FS_w_pio(struct one_wire_query *owq)
{
    BYTE data = BYTE_INVERSE(OWQ_U(owq)) & 0xFF ;   /* reverse bits */
    
    return RETURN_Z_OR_E(OW_w_pio(data, PN(owq))) ;
}

/* 2408 read activity latch */
/* From register 0x8A */
static ZERO_OR_ERROR FS_r_latch(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_U(owq) = data[2];
	return 0;
}

/* 2408 write activity latch */
/* Actually resets them all */
static ZERO_OR_ERROR FS_w_latch(struct one_wire_query *owq)
{
	return RETURN_Z_OR_E(OW_c_latch(PN(owq))) ;
}

/* 2408 alarm settings*/
/* From registers 0x8B-0x8D */
static ZERO_OR_ERROR FS_r_s_alarm(struct one_wire_query *owq)
{
	BYTE d[6];
	int i, p;
	UINT U;
	if ( BAD( OW_r_reg(d, PN(owq)) ) ) {
		return -EINVAL;
	}
	/* register 0x8D */
	U = (d[5] & 0x03) * 100000000;
	/* registers 0x8B and 0x8C */
	for (i = 0, p = 1; i < 8; ++i, p *= 10) {
		U += UT_getbit(&d[3], i) | (UT_getbit(&d[4], i) << 1) * p;
	}
	OWQ_U(owq) = U;
	return 0;
}

/* 2408 alarm settings*/
/* First digit source and logic data[2] */
/* next 8 channels */
/* data[1] polarity */
/* data[0] selection  */
static ZERO_OR_ERROR FS_w_s_alarm(struct one_wire_query *owq)
{
	BYTE data[3];
	int i;
	UINT p;
	UINT U = OWQ_U(owq);
	for (i = 0, p = 1; i < 8; ++i, p *= 10) {
		UT_setbit(&data[1], i, ((int) (U / p) % 10) & 0x01);
		UT_setbit(&data[0], i, (((int) (U / p) % 10) & 0x02) >> 1);
	}
	data[2] = ((U / 100000000) % 10) & 0x03;
	return RETURN_Z_OR_E(OW_w_s_alarm(data, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_por(struct one_wire_query *owq)
{
	BYTE data[6];
	if ( BAD( OW_r_reg(data, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_Y(owq) = UT_getbit(&data[5], 3);
	return 0;
}

static ZERO_OR_ERROR FS_w_por(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE data[6];
	if ( BAD( OW_r_reg(data, pn) ) ) {
		return -EINVAL;
	}
	UT_setbit(&data[5], 3, OWQ_Y(owq));
	return RETURN_Z_OR_E( OW_w_control(data[5], pn) ) ;
}

#define LCD_SECOND_ROW_ADDRESS        0x40
#define LCD_COMMAND_CLEAR_DISPLAY     0x01
#define LCD_COMMAND_RETURN_HOME       0x02
#define LCD_COMMAND_RIGHT_TO_LEFT     0x06
#define LCD_COMMAND_DISPLAY_ON        0x0C
#define LCD_COMMAND_DISPLAY_OFF       0x08
#define LCD_COMMAND_4_BIT             0x20
#define LCD_COMMAND_4_BIT_2_LINES     0x28
#define LCD_COMMAND_ATTENTION         0x30
#define LCD_COMMAND_SET_DDRAM_ADDRESS 0x80

// structure holding the information to be placed on the LCD screen
struct yx {
	int y ;  // row
	int x ;  // column
	char * string ; // input string including coordinates
	size_t length ; // length of string
	int text_start ; // counter into string
} ;
static GOOD_OR_BAD Parseyx( struct yx * YX ) ;
static GOOD_OR_BAD binaryyx( struct yx * YX ) ;
static GOOD_OR_BAD asciiyx( struct yx * YX ) ;
static GOOD_OR_BAD OW_Hprintyx(struct yx * YX, struct parsedname * pn) ;
static GOOD_OR_BAD OW_Hinit(struct parsedname * pn) ;

#define LCD_LINE_START           1
#define LCD_LINE_END             20
#define LCD_FIRST_ROW            1
#define LCD_LAST_ROW             4
#define LCD_SAME_LOCATION_VALUE  0

// Clear the display after potential initialization
static ZERO_OR_ERROR FS_Hclear(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE clear[] = {
		NIBBLE_CTRL(LCD_COMMAND_CLEAR_DISPLAY),
		NIBBLE_CTRL(LCD_COMMAND_DISPLAY_ON),
		NIBBLE_CTRL(LCD_COMMAND_RIGHT_TO_LEFT),
	};
	if ( BAD( OW_Hinit(pn) ) ) {
		LEVEL_DEBUG("Screen initialization error");	
		return -EINVAL ;
	}
	return RETURN_Z_OR_E(OW_w_pios(clear, 6, pn)) ;
}

static GOOD_OR_BAD OW_Hinit(struct parsedname * pn)
{
	int init = 1;
	// clear, display on, mode
	BYTE start[] = { LCD_COMMAND_ATTENTION, };
	BYTE next[] = {
		LCD_COMMAND_ATTENTION,
		LCD_COMMAND_ATTENTION,
		LCD_COMMAND_4_BIT,
		NIBBLE_CTRL(LCD_COMMAND_4_BIT_2_LINES),
	};
	BYTE data[6];

	// already done?
	if (Cache_Get_Internal_Strict(&init, sizeof(init), InternalProp(INI), pn)==0) {
		return gbGOOD;
	}
	
	if ( BAD( OW_w_control(0x04, pn) )	// strobe
		|| BAD( OW_r_reg(data, pn) ) ) {
		LEVEL_DEBUG("Trouble sending strobe to Hobbyboard LCD") ;
		return gbBAD;
	}
	if ( data[5] != 0x84 )	{
		LEVEL_DEBUG("LCD is not powered"); // not powered
		return gbBAD ;
	}
	if ( BAD( OW_c_latch(pn) ) ) {
		LEVEL_DEBUG("Trouble clearing latches") ;
		return gbBAD ;
	}// clear PIOs
	if ( BAD( OW_w_pios(start, 1, pn) ) ) {
		LEVEL_DEBUG("Error sending initial attention");	
		return gbBAD;
	}
	UT_delay(5);
	if ( BAD( OW_w_pios(next, 5, pn) ) ) {
		LEVEL_DEBUG("Error sending setup commands");	
		return gbBAD;
	}
	Cache_Add_Internal(&init, sizeof(init), InternalProp(INI), pn);
	return gbGOOD ;
}

static GOOD_OR_BAD Parseyx( struct yx * YX )
{
	if ( YX->length < 2 ) {
		// not long enough to have an address
		LEVEL_DEBUG("String too short to contain the location (%d bytes)",YX->length);
		return gbBAD ;
	}

	if ( YX->string[0] > '0' ) {
		// ascii address rather than binary
		return asciiyx(YX);
	}
	return binaryyx( YX ) ;
}

// Extract coordinates from binary string and point coordinates. string and length already set.
static GOOD_OR_BAD binaryyx( struct yx * YX )
{
	YX->y = YX->string[0] ;
	YX->x = YX->string[1] ;
	YX->text_start = 2 ;

	return gbGOOD ; // next char
}

// Extract coordinates from ascii string and point past colon. string and length already set.
static GOOD_OR_BAD asciiyx( struct yx * YX )
{
	char * colon = memchr( YX->string, ':', YX->length ) ; // position of mandatory colon
	if ( colon==NULL ) {
		LEVEL_DEBUG("No colon in screen text location. Should be 'y.x:text'");
		return gbBAD ;
	}

	if ( sscanf(YX->string, "%d,%d:", &YX->y, &YX->x ) < 2 ) {
		// only one value. Set row=1
		YX->y = 1 ;
		if ( sscanf(YX->string, "%d:", &YX->x ) < 1 ) {
			LEVEL_DEBUG("Ascii string location not valid");
			return gbBAD ;
		}
	}
	// start text after colon
	YX->text_start = ( colon - YX->string ) + 1 ;
	return gbGOOD ;
}

// put in home position
static ZERO_OR_ERROR FS_Hhome(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct yx YX = { 1, 1, "", 0, 0 } ;
	if ( BAD( OW_Hinit(pn) ) ) {
		return -EINVAL ;
	}
	return RETURN_Z_OR_E( OW_Hprintyx(&YX, pn) );
}

// Print from home position
static ZERO_OR_ERROR FS_Hmessage(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct yx YX = { 1, 1, OWQ_buffer(owq), OWQ_size(owq), 0 } ;
	if ( BAD( OW_Hinit(pn) ) ) {
		return -EINVAL ;
	}
	if (FS_Hclear(owq) != 0) {
		return -EINVAL;
	}
	return RETURN_Z_OR_E( OW_Hprintyx(&YX, pn) );
}

// print from current position
static ZERO_OR_ERROR FS_Hscreen(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	// y=0 is flag to do no position setting
	struct yx YX = { LCD_SAME_LOCATION_VALUE, LCD_SAME_LOCATION_VALUE, OWQ_buffer(owq), OWQ_size(owq), 0 } ;
	if ( BAD( OW_Hinit(pn) ) ) {
		return -EINVAL ;
	}
	return RETURN_Z_OR_E( OW_Hprintyx(&YX, pn) );
}

// print from specified positionh --
// either in ascii format "y.x:text" or "x:text"
// or binary (first 2 bytes are y and x)
static ZERO_OR_ERROR FS_Hscreenyx(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct yx YX = { 0, 0, OWQ_buffer(owq), OWQ_size(owq), 0 } ;

	if ( BAD( Parseyx( &YX )  ) ) {
		return -EINVAL ;
	}
	
	if ( BAD( OW_Hinit(pn) ) ) {
		return -EINVAL ;
	}
	return RETURN_Z_OR_E( OW_Hprintyx(&YX, pn) );
}

// YX structure is set with y,x,string,length, and start position of text 
static GOOD_OR_BAD OW_Hprintyx(struct yx * YX, struct parsedname * pn)
{
	BYTE translated_data[2 + 2 * (YX->length-YX->text_start)];
	size_t original_index ;
	size_t translate_index = 0;

	int Valid_YX = ( YX->x <= LCD_LINE_END && YX->y <= LCD_LAST_ROW && YX->x >= LCD_LINE_START && YX->y >= LCD_FIRST_ROW ) ;
	int Continue_YX = ( YX->y == LCD_SAME_LOCATION_VALUE || YX->x == LCD_SAME_LOCATION_VALUE ) ;

	if ( Valid_YX ) {
		BYTE chip_command ;
		switch (YX->y) { // row value
			case 1:
				chip_command = LCD_COMMAND_SET_DDRAM_ADDRESS;
				break;
			case 2:
				chip_command = LCD_COMMAND_SET_DDRAM_ADDRESS + LCD_SECOND_ROW_ADDRESS;
				break;
			case 3:
				chip_command = LCD_COMMAND_SET_DDRAM_ADDRESS + LCD_LINE_END;
				break;
			case 4:
				chip_command = (LCD_COMMAND_SET_DDRAM_ADDRESS + LCD_SECOND_ROW_ADDRESS) + LCD_LINE_START;
				break;
			default:
				LEVEL_DEBUG("Unrecognized row %d",YX->y) ;
				return 1 ;
		}
		chip_command += YX->x - 1; // add column (0 index)
		// Initial location (2 half bytes)
		translated_data[translate_index++] = NIBBLE_ONE(chip_command);
		translated_data[translate_index++] = NIBBLE_TWO(chip_command);
	} else if ( !Continue_YX ) {
		LEVEL_DEBUG("Bad screen coordinates y=%d x=%d",YX->y,YX->x);
		return gbBAD;
	}
		
	//printf("Hscreen test<%*s>\n",(int)size,buf) ;
	for ( original_index = YX->text_start ; original_index < YX->length ; ++original_index ) {
		if (YX->string[original_index]) {
			translated_data[translate_index++] = NIBBLE_ONE(YX->string[original_index]) | LCD_DATA_FLAG;
			translated_data[translate_index++] = NIBBLE_TWO(YX->string[original_index]) | LCD_DATA_FLAG;
		} else {				//null byte becomes space
			translated_data[translate_index++] = NIBBLE_ONE(' ') | LCD_DATA_FLAG;
			translated_data[translate_index++] = NIBBLE_TWO(' ') | LCD_DATA_FLAG;
		}
	}
	LEVEL_DEBUG("Print the message");
	return OW_w_pios(translated_data, translate_index, pn) ;
}

// 0x01 => blinking cursor on
// 0x02 => cursor on
// 0x04 => display on
static ZERO_OR_ERROR FS_Honoff(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE onoff[] = { NIBBLE_DATA(OWQ_U(owq)) };

	if ( BAD( OW_Hinit(pn) ) ) {
		return -EINVAL ;
	}
	// onoff
	if ( BAD( OW_w_pios(onoff, 2, pn) ) ) {
		LEVEL_DEBUG("Error setting LCD state");	
		return -EINVAL;
	}
	return 0;
}

/* Read 6 bytes --
   0x88 PIO logic State
   0x89 PIO output Latch state
   0x8A PIO Activity Latch
   0x8B Conditional Ch Mask
   0x8C Londitional Ch Polarity
   0x8D Control/Status
   plus 2 more bytes to get to the end of the page and qualify for a CRC16 checksum
*/
static GOOD_OR_BAD OW_r_reg(BYTE * data, const struct parsedname *pn)
{
	BYTE p[3 + 8 + 2] = { _1W_READ_PIO_REGISTERS,
		LOW_HIGH_ADDRESS(_ADDRESS_PIO_LOGIC_STATE),
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 8),
		TRXN_END,
	};

	//printf( "R_REG read attempt\n");
	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}
	//printf( "R_REG read ok\n");

	memcpy(data, &p[3], 6);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_pio(const BYTE data, const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_CHANNEL_ACCESS_WRITE, data, (BYTE) ~ data, };
	BYTE read_back[2];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_string),
		TRXN_READ2(read_back),
		TRXN_END,
	};

	//printf( "W_PIO attempt\n");
    if (BUS_transaction(t, pn)) {
		return gbBAD;
    }
	//printf( "W_PIO attempt\n");
	//printf("wPIO data = %2X %2X %2X %2X %2X\n",write_string[0],write_string[1],write_string[2],read_back[0],read_back[1]) ;
    if (read_back[0] != 0xAA) {
		return gbBAD;
    }
	//printf( "W_PIO 0xAA ok\n");
	/* Ignore byte 5 read_back[1] the PIO status byte */
	return gbGOOD;
}

/* Send several bytes to the channel */
static GOOD_OR_BAD OW_w_pios(const BYTE * data, const size_t size, const struct parsedname *pn)
{
	BYTE cmd[] = { _1W_CHANNEL_ACCESS_WRITE, };
	size_t formatted_size = 4 * size;
	BYTE formatted_data[formatted_size];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(cmd),
		TRXN_MODIFY(formatted_data, formatted_data, formatted_size),
		TRXN_END,
	};
	size_t i;

	// setup the array
	// each byte takes 4 bytes after formatting
	for (i = 0; i < size; ++i) {
		int formatted_data_index = 4 * i;
		formatted_data[formatted_data_index + 0] = data[i];
		formatted_data[formatted_data_index + 1] = (BYTE) ~ data[i];
		formatted_data[formatted_data_index + 2] = 0xFF;
		formatted_data[formatted_data_index + 3] = 0xFF;
	}
	//{ int j ; printf("IN  "); for (j=0 ; j<formatted_size ; ++j ) printf("%.2X ",formatted_data[j]); printf("\n") ; }
	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}
	//{ int j ; printf("OUT "); for (j=0 ; j<formatted_size ; ++j ) printf("%.2X ",formatted_data[j]); printf("\n") ; }
	// check the array
	for (i = 0; i < size; ++i) {
		int formatted_data_index = 4 * i;
		BYTE rdata = ((BYTE)~data[i]);  // get rid of warning: comparison of promoted ~unsigned with unsigned
		if (formatted_data[formatted_data_index + 0] != data[i]) {
			return gbBAD;
		}
		if (formatted_data[formatted_data_index + 1] != rdata) {
			return gbBAD;
		}
		if (formatted_data[formatted_data_index + 2] != 0xAA) {
			return gbBAD;
		}
		if (formatted_data[formatted_data_index + 3] != data[i]) {
			return gbBAD;
		}
	}

	return gbGOOD;
}

/* Reset activity latch */
static GOOD_OR_BAD OW_c_latch(const struct parsedname *pn)
{
	BYTE reset_string[] = { _1W_RESET_ACTIVITY_LATCHES, };
	BYTE read_back[1];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(reset_string),
		TRXN_READ1(read_back),
		TRXN_END,
	};

	//printf( "C_LATCH attempt\n");
	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}
	//printf( "C_LATCH transact\n");
	if (read_back[0] != 0xAA) {
		return gbBAD;
	}
	//printf( "C_LATCH 0xAA ok\n");

	return gbGOOD;
}

/* Write control/status */
static GOOD_OR_BAD OW_w_control(const BYTE data, const struct parsedname *pn)
{
	BYTE write_string[1 + 2 + 1] = { _1W_WRITE_CONDITIONAL_SEARCH_REGISTER,
		LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER), data,
	};
	BYTE check_string[1 + 2 + 3 + 2] = { _1W_READ_PIO_REGISTERS,
		LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER),
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(write_string, 4),
		/* Read registers */
		TRXN_START,
		TRXN_WR_CRC16(check_string, 3, 3),
		TRXN_END,
	};

	//printf( "W_CONTROL attempt\n");
	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}
	//printf( "W_CONTROL ok, now check %.2X -> %.2X\n",data,check_string[3]);

	return ((data & 0x0F) != (check_string[3] & 0x0F)) ? gbBAD : gbGOOD ;
}

/* write alarm settings */
static GOOD_OR_BAD OW_w_s_alarm(const BYTE * data, const struct parsedname *pn)
{
	BYTE old_register[6];
	BYTE new_register[6];
	BYTE control_value[1];
	BYTE alarm_access[] = { _1W_WRITE_CONDITIONAL_SEARCH_REGISTER,
		LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER),
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(alarm_access),
		TRXN_WRITE2(data),
		TRXN_WRITE1(control_value),
		TRXN_END,
	};

	// get the existing register contents
	if ( BAD( OW_r_reg(old_register, pn) ) ) {
		return gbBAD;
	}

	//printf("S_ALARM 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],data[2]) ;
	control_value[0] = (data[2] & 0x03) | (old_register[5] & 0x0C);
	//printf("S_ALARM adjusted 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],cr) ;

	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}

	/* Re-Read registers */
	if (OW_r_reg(new_register, pn)) {
		return gbBAD;
	}
	//printf("S_ALARM back 0x8B... = %.2X %.2X %.2X \n",d[3],d[4],d[5]) ;

	return (data[0] != new_register[3]) || (data[1] != new_register[4])
		|| (control_value[0] != (new_register[5] & 0x0F)) ? gbBAD : gbGOOD;
}
