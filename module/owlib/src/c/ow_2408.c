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
WRITE_FUNCTION(FS_Hmessage);

/* ------- Structures ----------- */

struct aggregate A2408 = { 8, ag_numbers, ag_aggregate, };
struct filetype DS2408[] = {
	F_STANDARD,
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_power, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"PIO",PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_stable,   FS_r_pio, FS_w_pio, {v:NULL},} ,
  {"sensed",PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_volatile,   FS_sense, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"latch",PROPERTY_LENGTH_BITFIELD, &A2408, ft_bitfield, fc_volatile,   FS_r_latch, FS_w_latch, {v:NULL},} ,
  {"strobe",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   FS_r_strobe, FS_w_strobe, {v:NULL},} ,
  {"set_alarm",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,   FS_r_s_alarm, FS_w_s_alarm, {v:NULL},} ,
  {"por",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   FS_r_por, FS_w_por, {v:NULL},} ,
  {"LCD_M",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"LCD_M/clear",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_Mclear, {v:NULL},} ,
  {"LCD_M/home",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_Mhome, {v:NULL},} ,
  {"LCD_M/screen", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_Mscreen, {v:NULL},} ,
  {"LCD_M/message", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_Mmessage, {v:NULL},} ,
  {"LCD_H",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"LCD_H/clear",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_Hclear, {v:NULL},} ,
  {"LCD_H/home",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_Hhome, {v:NULL},} ,
  {"LCD_H/screen", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_Hscreen, {v:NULL},} ,
  {"LCD_H/message", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_Hmessage, {v:NULL},} ,
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
static struct internal_prop ip_init = { "INI", fc_stable };

/* Nibbles for LCD controller */
#define NIBBLE_CTRL( x )    ((x)&0xF0) , (((x)<<4)&0xF0)
#define NIBBLE_DATA( x )    ((x)&0xF0)|0x08 , (((x)<<4)&0xF0)|0x08

/* ------- Functions ------------ */

/* DS2408 */
static int OW_w_control(const BYTE data, const struct parsedname *pn);
static int OW_c_latch(const struct parsedname *pn);
static int OW_w_pio(const BYTE data, const struct parsedname *pn);
static int OW_r_reg(BYTE * data, const struct parsedname *pn);
static int OW_w_s_alarm(const BYTE * data, const struct parsedname *pn);
static int OW_w_pios(const BYTE * data, const size_t size,
					 const struct parsedname *pn);

/* 2408 switch */
/* 2408 switch -- is Vcc powered?*/
static int FS_power(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data[5], 7);
	return 0;
}

static int FS_r_strobe(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data[5], 2);
	return 0;
}

static int FS_w_strobe(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[6];
	if (OW_r_reg(data, pn))
		return -EINVAL;
    UT_setbit(&data[5], 2, OWQ_Y(owq));
	return OW_w_control(data[5], pn) ? -EINVAL : 0;
}

static int FS_Mclear(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int init = 1;

	if (Cache_Get_Internal_Strict(&init, sizeof(init), &ip_init, pn)) {
        OWQ_Y(owq) = 1 ;
		if (FS_r_strobe(owq)	// set reset pin to strobe mode
			|| OW_w_pio(0x30, pn))
			return -EINVAL;
		UT_delay(100);
		// init
		if (OW_w_pio(0x38, pn))
			return -EINVAL;
		UT_delay(10);
		// Enable Display, Cursor, and Blinking
		// Entry-mode: auto-increment, no shift
		if (OW_w_pio(0x0F, pn) || OW_w_pio(0x06, pn))
			return -EINVAL;
		Cache_Add_Internal(&init, sizeof(init), &ip_init, pn);
	}
	// clear
	if (OW_w_pio(0x01, pn))
		return -EINVAL;
	UT_delay(2);
	return FS_Mhome(owq);
}

static int FS_Mhome(struct one_wire_query * owq)
{
// home
    if (OW_w_pio(0x02, PN(owq)))
		return -EINVAL;
	UT_delay(2);
	return 0;
}

static int FS_Mscreen(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    size_t size = OWQ_size(owq) ;
    BYTE data[size];
	size_t i;
	for (i = 0; i < size; ++i) {
        if (OWQ_buffer(owq)[i] & 0x80)
			return -EINVAL;
        data[i] = OWQ_buffer(owq)[i] | 0x80;
	}
	return OW_w_pios(data, size, pn);
}

static int FS_Mmessage(struct one_wire_query * owq)
{
	if (FS_Mclear(owq))
		return -EINVAL;
	return FS_Mscreen(owq);
}

/* 2408 switch PIO sensed*/
/* From register 0x88 */
static int FS_sense(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data[0];
	return 0;
}

/* 2408 switch PIO set*/
/* From register 0x89 */
static int FS_r_pio(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data[1] ^ 0xFF;		/* reverse bits */
	return 0;
}

/* 2408 switch PIO change*/
static int FS_w_pio(struct one_wire_query * owq)
{
    /* reverse bits */
    if (OW_w_pio((OWQ_U(owq) & 0xFF) ^ 0xFF, PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2408 read activity latch */
/* From register 0x8A */
static int FS_r_latch(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data[2];
	return 0;
}

/* 2408 write activity latch */
/* Actually resets them all */
static int FS_w_latch(struct one_wire_query * owq)
{
    if (OW_c_latch(PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2408 alarm settings*/
/* From registers 0x8B-0x8D */
static int FS_r_s_alarm(struct one_wire_query * owq)
{
    BYTE d[6];
	int i, p;
    UINT U ;
    if (OW_r_reg(d, PN(owq)))
		return -EINVAL;
	/* register 0x8D */
    U = (d[5] & 0x03) * 100000000;
	/* registers 0x8B and 0x8C */
	for (i = 0, p = 1; i < 8; ++i, p *= 10)
        U += UT_getbit(&d[3], i) | (UT_getbit(&d[4], i) << 1) * p;
    OWQ_U(owq) = U ;
	return 0;
}

/* 2408 alarm settings*/
/* First digit source and logic data[2] */
/* next 8 channels */
/* data[1] polarity */
/* data[0] selection  */
static int FS_w_s_alarm(struct one_wire_query * owq)
{
    BYTE data[3];
	int i;
	UINT p;
    UINT U = OWQ_U(owq) ;
	for (i = 0, p = 1; i < 8; ++i, p *= 10) {
		UT_setbit(&data[1], i, ((int) (U / p) % 10) & 0x01);
		UT_setbit(&data[0], i, (((int) (U / p) % 10) & 0x02) >> 1);
	}
	data[2] = ((U / 100000000) % 10) & 0x03;
    if (OW_w_s_alarm(data, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_por(struct one_wire_query * owq)
{
    BYTE data[6];
    if (OW_r_reg(data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data[5], 3);
	return 0;
}

static int FS_w_por(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[6];
	if (OW_r_reg(data, pn))
		return -EINVAL;
    UT_setbit(&data[5], 3, OWQ_Y(owq));
	return OW_w_control(data[5], pn) ? -EINVAL : 0;
}

static int FS_Hclear(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int init = 1;
	// clear, display on, mode
	BYTE start[] = { 0x30, };
	BYTE next[] = { 0x30, 0x30, 0x20, NIBBLE_CTRL(0x28), };
	// 20 -- 4bit interface
	// 28 -- 4bit, 2 line
	BYTE clear[] =
		{ NIBBLE_CTRL(0x01), NIBBLE_CTRL(0x0C), NIBBLE_CTRL(0x06), };
	// 01 -- display clear
	// 0C -- display on
	// 06 -- entry mode set

	if (Cache_Get_Internal_Strict(&init, sizeof(init), &ip_init, pn)) {
		BYTE data[6];
		if (OW_w_control(0x04, pn)	// strobe
			|| OW_r_reg(data, pn)
			|| (data[5] != 0x84)	// not powered
			|| OW_c_latch(pn)	// clear PIOs
			|| OW_w_pios(start, 1, pn))
			return -EINVAL;
		UT_delay(5);
		if (OW_w_pios(next, 5, pn))
			return -EINVAL;
		Cache_Add_Internal(&init, sizeof(init), &ip_init, pn);
	}
	if (OW_w_pios(clear, 6, pn))
		return -EINVAL;
	return 0;
}

static int FS_Hhome(struct one_wire_query * owq)
{
    BYTE home[] = { 0x80, 0x00 };
	// home
    if (OW_w_pios(home, 2, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_Hscreen(struct one_wire_query * owq)
{
    char * buf = OWQ_buffer(owq) ;
    size_t size = OWQ_size(owq) ;
    BYTE data[2 * size + 2];
	size_t i, j = 0;
	
    data[0] = 0x80;
	data[1] = 0x00;
	//printf("Hscreen test<%*s>\n",(int)size,buf) ;
	for (i = 0, j = 2; i < size; ++i) {
		if (buf[i]) {
			data[j++] = (buf[i] & 0xF0) | 0x08;
			data[j++] = ((buf[i] << 4) & 0xF0) | 0x08;
		} else {				//null byte becomes space
			data[j++] = 0x28;
			data[j++] = 0x08;
		}
	}
    return OW_w_pios(data, j, PN(owq)) ? -EINVAL : 0;
}

static int FS_Hmessage(struct one_wire_query * owq)
{
	if (FS_Hclear(owq) || FS_Hhome(owq)
		|| FS_Hscreen(owq))
		return -EINVAL;
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
static int OW_r_reg(BYTE * data, const struct parsedname *pn)
{
    BYTE p[3 + 8 + 2] = { _1W_READ_PIO_REGISTERS, LOW_HIGH_ADDRESS(_ADDRESS_PIO_LOGIC_STATE), };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 3, trxn_match},
		{NULL, &p[3], 8 + 2, trxn_read},
		{p, NULL, 3 + 8 + 2, trxn_crc16},
		TRXN_END,
	};

	//printf( "R_REG read attempt\n");
	if (BUS_transaction(t, pn))
		return 1;
	//printf( "R_REG read ok\n");

	memcpy(data, &p[3], 6);
	return 0;
}

static int OW_w_pio(const BYTE data, const struct parsedname *pn)
{
    BYTE p[] = { _1W_CHANNEL_ACCESS_WRITE, data, ~data, };
	BYTE r[2];
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 3, trxn_match},
		{NULL, r, 2, trxn_read},
		TRXN_END,
	};

	//printf( "W_PIO attempt\n");
	if (BUS_transaction(t, pn))
		return 1;
	//printf( "W_PIO attempt\n");
	//printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],r[0],r[1]) ;
	if (r[0] != 0xAA)
		return 1;
	//printf( "W_PIO 0xAA ok\n");
	/* Ignore byte 5 r[1] the PIO status byte */
	return 0;
}

/* Send several bytes to the channel */
static int OW_w_pios(const BYTE * data, const size_t size,
					 const struct parsedname *pn)
{
	BYTE p[4 * size + 1];
	BYTE *q;
	struct transaction_log t[] = {
		TRXN_START,
		{p, p, 1, trxn_read},
		TRXN_END,
	};
	size_t n = 1 + 4 * size;
	size_t i;
	int ret = 0;

	t[1].size = n;
    p[0] = _1W_CHANNEL_ACCESS_WRITE;
	// setup the array
	for (i = 0, q = p; i < size; ++i) {
		*(++q) = data[i];
		*(++q) = ~data[i];
		*(++q) = 0xFF;
		*(++q) = 0xFF;
	}
	//{ int j ; printf("IN  "); for (j=0 ; j<n ; ++j ) printf("%.2X ",p[j]); printf("\n") ; }
	if (BUS_transaction(t, pn)) {
		ret = 1;
	} else {
		//{ int j ; printf("OUT "); for (j=0 ; j<n ; ++j ) printf("%.2X ",p[j]); printf("\n") ; }
		// check the array
		for (i = 0, q = p; i < size; ++i) {
			//printf("WPIOS %d\n",(int)i) ;
			if ((*(++q) != data[i]) || (*(++q) != (BYTE) (~data[i]))
				|| (*(++q) != 0xAA) || (*(++q) != data[i])) {
				//printf("WPIOS problem\n") ;
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

/* Reset activity latch */
static int OW_c_latch(const struct parsedname *pn)
{
    BYTE p[] = { _1W_RESET_ACTIVITY_LATCHES, };
	BYTE r;
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 1, trxn_match},
		{NULL, &r, 1, trxn_read},
		TRXN_END,
	};

	//printf( "C_LATCH attempt\n");
	if (BUS_transaction(t, pn))
		return 1;
	//printf( "C_LATCH transact\n");
	if (r != 0xAA)
		return 1;
	//printf( "C_LATCH 0xAA ok\n");

	return 0;
}

/* Write control/status */
static int OW_w_control(const BYTE data, const struct parsedname *pn)
{
    BYTE p[] = { _1W_WRITE_CONDITIONAL_SEARCH_REGISTER, LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER), data, };
    BYTE q[3 + 3 + 2] = { _1W_READ_PIO_REGISTERS, LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER), };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 4, trxn_match},
		/* Read registers */
		TRXN_START,
		{q, NULL, 3, trxn_match},
		{NULL, &q[3], 3 + 2, trxn_read},
		{q, NULL, 3 + 3 + 2, trxn_crc16},
		TRXN_END,
	};

	//printf( "W_CONTROL attempt\n");
	if (BUS_transaction(t, pn))
		return 1;
	//printf( "W_CONTROL ok, now check %.2X -> %.2X\n",data,q[3]);

	return ((data & 0x0F) != (q[3] & 0x0F));
}

/* write alarm settings */
static int OW_w_s_alarm(const BYTE * data, const struct parsedname *pn)
{
	BYTE d[6], cr;
    BYTE a[] = { _1W_WRITE_CONDITIONAL_SEARCH_REGISTER, LOW_HIGH_ADDRESS(_ADDRESS_CONTROL_STATUS_REGISTER), };
	struct transaction_log t[] = {
		TRXN_START,
		{a, NULL, 3, trxn_match},
		{data, NULL, 2, trxn_match},
		{&cr, NULL, 1, trxn_match},
		TRXN_END,
	};

	// get the existing register contents
	if (OW_r_reg(d, pn))
		return -EINVAL;

	//printf("S_ALARM 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],data[2]) ;
	cr = (data[2] & 0x03) | (d[5] & 0x0C);
	//printf("S_ALARM adjusted 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],cr) ;

	if (BUS_transaction(t, pn))
		return 1;

	/* Re-Read registers */
	if (OW_r_reg(d, pn))
		return 1;
	//printf("S_ALARM back 0x8B... = %.2X %.2X %.2X \n",d[3],d[4],d[5]) ;

	return (data[0] != d[3]) || (data[1] != d[4]) || (cr != (d[5] & 0x0F));
}
