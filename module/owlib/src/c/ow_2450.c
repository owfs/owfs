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
#include "ow_2450.h"

/* ------- Prototypes ----------- */

/* DS2450 A/D */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_volts);
READ_FUNCTION(FS_r_power);
WRITE_FUNCTION(FS_w_power);
READ_FUNCTION(FS_r_high);
WRITE_FUNCTION(FS_w_high);
READ_FUNCTION(FS_r_PIO);
WRITE_FUNCTION(FS_w_PIO);
READ_FUNCTION(FS_r_setvolt);
WRITE_FUNCTION(FS_w_setvolt);
READ_FUNCTION(FS_r_flag);
WRITE_FUNCTION(FS_w_flag);
READ_FUNCTION(FS_r_por);
WRITE_FUNCTION(FS_w_por);

READ_FUNCTION(FS_CO2_ppm);
READ_FUNCTION(FS_CO2_status);
READ_FUNCTION(FS_CO2_power);

/* ------- Structures ----------- */

struct aggregate A2450 = { 4, ag_numbers, ag_separate, };
struct aggregate A2450m = { 4, ag_letters, ag_mixed, };
struct aggregate A2450v = { 4, ag_letters, ag_aggregate, };
struct filetype DS2450[] = {
	F_STANDARD,
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", 8, &A2450, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_power, FS_w_power, VISIBLE, NO_FILETYPE_DATA,},
	{"memory", 32, NON_AGGREGATE, ft_binary, fc_stable, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA,},
	{"PIO", PROPERTY_LENGTH_YESNO, &A2450m, ft_yesno, fc_stable, FS_r_PIO, FS_w_PIO, VISIBLE, {i:0},},
	{"volt", PROPERTY_LENGTH_FLOAT, &A2450m, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:1},},
	{"volt2", PROPERTY_LENGTH_FLOAT, &A2450m, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:0},},
	{"set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"set_alarm/volthigh", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:3},},
	{"set_alarm/volt2high", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:2},},
	{"set_alarm/voltlow", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:1},},
	{"set_alarm/volt2low", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:0},},
	{"set_alarm/high", PROPERTY_LENGTH_YESNO, &A2450v, ft_yesno, fc_stable, FS_r_high, FS_w_high, VISIBLE, {i:1},},
	{"set_alarm/low", PROPERTY_LENGTH_YESNO, &A2450v, ft_yesno, fc_stable, FS_r_high, FS_w_high, VISIBLE, {i:0},},
	{"set_alarm/unset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_por, FS_w_por, VISIBLE, NO_FILETYPE_DATA,},
	{"alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"alarm/high", PROPERTY_LENGTH_YESNO, &A2450v, ft_yesno, fc_volatile, FS_r_flag, FS_w_flag, VISIBLE, {i:1},},
	{"alarm/low", PROPERTY_LENGTH_YESNO, &A2450v, ft_yesno, fc_volatile, FS_r_flag, FS_w_flag, VISIBLE, {i:0},},
	{"CO2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/ppm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_CO2_ppm, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/Vdd", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_yesno, fc_link, FS_CO2_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/status", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_float, fc_link, FS_CO2_status, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(20, DS2450, DEV_volt | DEV_alarm | DEV_ovdr);

#define _1W_READ_MEMORY 0x44
#define _1W_WRITE_MEMORY 0x55
#define _1W_CONVERT 0x3C

#define _1W_2450_POWERED 0x40
#define _1W_2450_UNPOWERED 0x00

#define _ADDRESS_CONVERSION_PAGE 0x00
#define _ADDRESS_CONTROL_PAGE 0x08
#define _ADDRESS_ALARM_PAGE 0x10
#define _ADDRESS_POWERED 0x1C

/* ------- Functions ------------ */

/* DS2450 */
static GOOD_OR_BAD OW_r_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_volts(_FLOAT * f, const int resolution, struct parsedname *pn);
static GOOD_OR_BAD OW_1_volts(_FLOAT * f, const int element, const int resolution, struct parsedname *pn);
static GOOD_OR_BAD OW_convert(struct parsedname *pn);
static GOOD_OR_BAD OW_r_pio(int *pio, struct parsedname *pn);
static GOOD_OR_BAD OW_r_1_pio(int *pio, const int element, struct parsedname *pn);
static GOOD_OR_BAD OW_w_pio(const int *pio, struct parsedname *pn);
static GOOD_OR_BAD OW_w_1_pio(const int pio, const int element, struct parsedname *pn);
static GOOD_OR_BAD OW_r_vset(_FLOAT * V, const int high, const int resolution, struct parsedname *pn);
static GOOD_OR_BAD OW_w_vset(const _FLOAT * V, const int high, const int resolution, struct parsedname *pn);
static GOOD_OR_BAD OW_r_high(int *y, const int high, struct parsedname *pn);
static GOOD_OR_BAD OW_w_high(const int *y, const int high, struct parsedname *pn);
static GOOD_OR_BAD OW_r_flag(int *y, const int high, struct parsedname *pn);
static GOOD_OR_BAD OW_w_flag(const int *y, const int high, struct parsedname *pn);
static GOOD_OR_BAD OW_r_por(int *y, struct parsedname *pn);
static GOOD_OR_BAD OW_w_por(const int por, struct parsedname *pn);

/* read a page of memory (8 bytes) */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, COMMON_read_memory_crc16_AA)) ;
}

/* write an 8-byte page */
static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem)) ;
}

/* read powered flag */
static ZERO_OR_ERROR FS_r_power(struct one_wire_query *owq)
{
	BYTE p;
	RETURN_ERROR_IF_BAD( OW_r_mem(&p, 1, _ADDRESS_POWERED, PN(owq)) ) ;
//printf("Cont %d\n",p) ;
	OWQ_Y(owq) = (p == _1W_2450_POWERED);
	return 0;
}

/* write powered flag */
static ZERO_OR_ERROR FS_w_power(struct one_wire_query *owq)
{
	BYTE p = _1W_2450_POWERED;	/* powered */
	BYTE q = _1W_2450_UNPOWERED;	/* parasitic */
	if (OW_w_mem(OWQ_Y(owq) ? &p : &q, 1, _ADDRESS_POWERED, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* read "unset" PowerOnReset flag */
static ZERO_OR_ERROR FS_r_por(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_por(&OWQ_Y(owq), PN(owq))) ;
}

/* write "unset" PowerOnReset flag */
static ZERO_OR_ERROR FS_w_por(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_por(OWQ_Y(owq) & 0x01, PN(owq))) ;
}

/* read high/low voltage enable alarm flags */
static ZERO_OR_ERROR FS_r_high(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int Y[4];
	RETURN_ERROR_IF_BAD( OW_r_high(Y, pn->selected_filetype->data.i & 0x01, pn) ) ;
	OWQ_array_Y(owq, 0) = Y[0];
	OWQ_array_Y(owq, 1) = Y[1];
	OWQ_array_Y(owq, 2) = Y[2];
	OWQ_array_Y(owq, 3) = Y[3];
	return 0;
}

/* write high/low voltage enable alarm flags */
static ZERO_OR_ERROR FS_w_high(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int Y[4] = {
		OWQ_array_Y(owq, 0),
		OWQ_array_Y(owq, 1),
		OWQ_array_Y(owq, 2),
		OWQ_array_Y(owq, 3),
	};
	return GB_to_Z_OR_E(OW_w_high(Y, pn->selected_filetype->data.i & 0x01, pn)) ;
}

/* read high/low voltage triggered state alarm flags */
static ZERO_OR_ERROR FS_r_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int Y[4];
	RETURN_ERROR_IF_BAD( OW_r_flag(Y, pn->selected_filetype->data.i & 0x01, pn) ) ;
	OWQ_array_Y(owq, 0) = Y[0];
	OWQ_array_Y(owq, 1) = Y[1];
	OWQ_array_Y(owq, 2) = Y[2];
	OWQ_array_Y(owq, 3) = Y[3];
	return 0;
}

/* write high/low voltage triggered state alarm flags */
static ZERO_OR_ERROR FS_w_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int Y[4] = {
		OWQ_array_Y(owq, 0),
		OWQ_array_Y(owq, 1),
		OWQ_array_Y(owq, 2),
		OWQ_array_Y(owq, 3),
	};
	return GB_to_Z_OR_E(OW_w_flag(Y, pn->selected_filetype->data.i & 0x01, pn)) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_r_PIO(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int element = pn->extension;
	GOOD_OR_BAD pio_error;
	if (element == EXTENSION_ALL) {
		int y[4] = { 0, 0, 0, 0, };
		pio_error = OW_r_pio(y, pn);
		OWQ_array_Y(owq, 0) = y[0];
		OWQ_array_Y(owq, 1) = y[1];
		OWQ_array_Y(owq, 2) = y[2];
		OWQ_array_Y(owq, 3) = y[3];
	} else {
		pio_error = OW_r_1_pio(&OWQ_Y(owq), element, pn);
	}
	return GB_to_Z_OR_E(pio_error) ;
}

/* 2450 A/D */
/* pio status -- special code for ag_mixed */
static ZERO_OR_ERROR FS_w_PIO(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int element = pn->extension;
	GOOD_OR_BAD pio_error;
	if (element == EXTENSION_ALL) {
		int y[4] = {
			OWQ_array_Y(owq, 0),
			OWQ_array_Y(owq, 1),
			OWQ_array_Y(owq, 2),
			OWQ_array_Y(owq, 3),
		};
		pio_error = OW_w_pio(y, pn);
	} else {
		pio_error = OW_w_1_pio(OWQ_Y(owq), element, pn);
	}
	return GB_to_Z_OR_E(pio_error) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_crc16_AA)) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_w_mem)) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_volts(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int element = pn->extension;
	GOOD_OR_BAD volt_error;
	if (element == EXTENSION_ALL) {
		_FLOAT V[4] = { 0., 0., 0., 0., };
		volt_error = OW_volts(V, pn->selected_filetype->data.i, pn);
		OWQ_array_F(owq, 0) = V[0];
		OWQ_array_F(owq, 1) = V[1];
		OWQ_array_F(owq, 2) = V[2];
		OWQ_array_F(owq, 3) = V[3];
	} else {
		volt_error = OW_1_volts(&OWQ_F(owq), element, pn->selected_filetype->data.i, pn);
	}
	return GB_to_Z_OR_E(volt_error) ;
}

static ZERO_OR_ERROR FS_r_setvolt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	_FLOAT V[4];
	RETURN_ERROR_IF_BAD( OW_r_vset(V, (pn->selected_filetype->data.i) >> 1, (pn->selected_filetype->data.i) & 0x01, pn) ) ;
	OWQ_array_F(owq, 0) = V[0];
	OWQ_array_F(owq, 1) = V[1];
	OWQ_array_F(owq, 2) = V[2];
	OWQ_array_F(owq, 3) = V[3];
	return 0;
}

static ZERO_OR_ERROR FS_w_setvolt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	_FLOAT V[4] = {
		OWQ_array_F(owq, 0),
		OWQ_array_F(owq, 1),
		OWQ_array_F(owq, 2),
		OWQ_array_F(owq, 3),
	};
	return GB_to_Z_OR_E(OW_w_vset(V, (pn->selected_filetype->data.i) >> 1, (pn->selected_filetype->data.i) & 0x01, pn)) ;
}

static GOOD_OR_BAD OW_r_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn)
{
	size_t pagesize = 8;
	OWQ_allocate_struct_and_pointer(owq_read);

	OWQ_create_temporary(owq_read, (char *) p, size, offset, pn);
	return COMMON_read_memory_crc16_AA(owq_read, 0, pagesize);
}

/* write to 2450 */
static GOOD_OR_BAD OW_w_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn)
{
	// command, address(2) , data , crc(2), databack
	BYTE buf[6] = { _1W_WRITE_MEMORY, LOW_HIGH_ADDRESS(offset), p[0], };
	BYTE echo[1];
	size_t i;
	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WR_CRC16(buf, 4, 0),
		TRXN_READ1(echo),
		TRXN_COMPARE(echo, p, 1),
		TRXN_END,
	};
	struct transaction_log trest[] = {	// note no TRXN_START
		TRXN_WRITE1(buf),
		TRXN_READ2(&buf[1]),
		TRXN_READ1(echo),
		TRXN_END,
	};
//printf("2450 W mem size=%d location=%d\n",size,location) ;

	if (size == 0) {
		return gbGOOD;
	}

	/* Send the first byte (handled differently) */
	RETURN_BAD_IF_BAD(BUS_transaction(tfirst, pn)) ;
	/* rest of the bytes */
	for (i = 1; i < size; ++i) {
		buf[0] = p[i];
		RETURN_BAD_IF_BAD( BUS_transaction(trest, pn) ) ;
		if ( CRC16seeded(buf, 3, offset + i) || (echo[0] != p[i]) ) {
			return gbBAD;
		}
	}
	return gbGOOD;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
/* resolution is 1->5.10V 0->2.55V */
static GOOD_OR_BAD OW_volts(_FLOAT * f, const int resolution, struct parsedname *pn)
{
	BYTE control[8];
	BYTE data[8];
	int i;
	int writeback = 0;			/* write control back? */
	//printf("Volts res=%d\n",resolution);OW_r_mem
	// Get control registers and set to A/D 16 bits
	RETURN_BAD_IF_BAD( OW_r_mem(control, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;
	for (i = 0; i < 8; i++) {	// warning, counter in incremented in loop, too
		if (control[i] & 0x0F) {
			control[i] &= 0xF0;	// 16bit, A/D
			writeback = 1;
		}
		if ((control[++i] & 0x01) != (resolution & 0x01)) {
			UT_setbit(&control[i], 0, resolution);
			writeback = 1;
		}
	}
	//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;

	// Set control registers
	if (writeback) {
		RETURN_BAD_IF_BAD( OW_w_mem(control, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	}
	//printf("writeback=%d\n",writeback);
	// Start A/D process if needed
	RETURN_BAD_IF_BAD( OW_convert(pn) ) ;

	// read data
	RETURN_BAD_IF_BAD( OW_r_mem(data, 8, _ADDRESS_CONVERSION_PAGE, pn) ) ;

	// data conversions
	f[0] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * ((((UINT) data[1]) << 8) | data[0]);
	f[1] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * ((((UINT) data[3]) << 8) | data[2]);
	f[2] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * ((((UINT) data[5]) << 8) | data[4]);
	f[3] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * ((((UINT) data[7]) << 8) | data[6]);
	return gbGOOD;
}


/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution on a single channel */
/* resolution is 1->5.10V 0->2.55V */
static GOOD_OR_BAD OW_1_volts(_FLOAT * f, const int element, const int resolution, struct parsedname *pn)
{
	BYTE control[2];
	BYTE data[2];
	int writeback = 0;			/* write control back? */

	// Get control registers and set to A/D 16 bits
	RETURN_BAD_IF_BAD( OW_r_mem(control, 2, _ADDRESS_CONTROL_PAGE + (element << 1), pn) ) ;
	if (control[0] & 0x0F) {
		control[0] &= 0xF0;		// 16bit, A/D
		writeback = 1;
	}
	if ((control[1] & 0x01) != (resolution & 0x01)) {
		UT_setbit(&control[1], 0, resolution);
		writeback = 1;
	}
	// Set control registers
	if (writeback) {
		RETURN_BAD_IF_BAD( OW_w_mem(control, 2, _ADDRESS_CONTROL_PAGE + (element << 1), pn) ) ;
	}
	// Start A/D process
	RETURN_BAD_IF_BAD( OW_convert(pn) ) ;

	// read data
	RETURN_BAD_IF_BAD( OW_r_mem(data, 2, _ADDRESS_CONVERSION_PAGE + (element << 1), pn) ) ;

	// data conversions
	//f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[1])<<8)|data[0]) ;
	f[0] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * UT_uint16(data);
	return gbGOOD;
}

/* send A/D conversion command */
static GOOD_OR_BAD OW_convert(struct parsedname *pn)
{
	BYTE convert[] = { _1W_CONVERT, 0x0F, 0x00, 0xFF, 0xFF, };
	BYTE power;
	UINT delay = 6 ;
	struct transaction_log tpower[] = {
		TRXN_START,
		TRXN_WR_CRC16(convert, 3, 0),
		TRXN_END,
	};
	struct transaction_log tdead[] = {
		TRXN_START,
		TRXN_WRITE3(convert),
		TRXN_READ1(&convert[3]),
		{&convert[4], &convert[4], delay, trxn_power},
		TRXN_CRC16(convert, 5),
		TRXN_END,
	};

	/* get power flag -- to see if pullup can be avoided */
	RETURN_BAD_IF_BAD( OW_r_mem(&power, 1, 0x1C, pn) ) ;

	/* See if a conversion was globally triggered */
	if ((power == _1W_2450_POWERED) && GOOD(FS_Test_Simultaneous( simul_volt, delay, pn))) {
		return gbGOOD;
	}

	// Start conversion
	// 6 msec for 16bytex4channel (5.2)
	if (power == _1W_2450_POWERED) {	//powered
		RETURN_BAD_IF_BAD(BUS_transaction(tpower, pn));
		UT_delay(delay);			/* don't need to hold line for conversion! */
	} else {
		RETURN_BAD_IF_BAD(BUS_transaction(tdead, pn)) ;
	}
	return gbGOOD;
}

/* read all the pio registers */
static GOOD_OR_BAD OW_r_pio(int *pio, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	pio[0] = ((p[0] & 0xC0) != 0x80);
	pio[1] = ((p[2] & 0xC0) != 0x80);
	pio[2] = ((p[4] & 0xC0) != 0x80);
	pio[3] = ((p[6] & 0xC0) != 0x80);
	return gbGOOD;
}

/* read one pio register */
static GOOD_OR_BAD OW_r_1_pio(int *pio, const int element, struct parsedname *pn)
{
	BYTE p[2];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 2, _ADDRESS_CONTROL_PAGE + (element << 1), pn) ) ;
	pio[0] = ((p[0] & 0xC0) != 0x80);
	return gbGOOD;
}

/* Write all the pio registers */
static GOOD_OR_BAD OW_w_pio(const int *pio, struct parsedname *pn)
{
	BYTE p[8];
	int i;
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	for (i = 0; i <= 3; ++i) {
		p[i << 1] &= 0x3F;
		p[i << 1] |= pio[i] ? 0xC0 : 0x80;
	}
	return OW_w_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn);
}

/* write just one pio register */
static GOOD_OR_BAD OW_w_1_pio(const int pio, const int element, struct parsedname *pn)
{
	BYTE p[2];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 2, _ADDRESS_CONTROL_PAGE + (element << 1), pn) ) ;
	p[0] &= 0x3F;
	p[0] |= pio ? 0xC0 : 0x80;
	return OW_w_mem(p, 2, _ADDRESS_CONTROL_PAGE + (element << 1), pn);
}

static GOOD_OR_BAD OW_r_vset(_FLOAT * V, const int high, const int resolution, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_ALARM_PAGE, pn) ) ;
	V[0] = (resolution ? .02 : .01) * p[0 + high];
	V[1] = (resolution ? .02 : .01) * p[2 + high];
	V[2] = (resolution ? .02 : .01) * p[4 + high];
	V[3] = (resolution ? .02 : .01) * p[6 + high];
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_vset(const _FLOAT * V, const int high, const int resolution, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_ALARM_PAGE, pn) ) ;
	p[0 + high] = V[0] * (resolution ? 50. : 100.);
	p[2 + high] = V[1] * (resolution ? 50. : 100.);
	p[4 + high] = V[2] * (resolution ? 50. : 100.);
	p[6 + high] = V[3] * (resolution ? 50. : 100.);
	RETURN_BAD_IF_BAD( OW_w_mem(p, 8, _ADDRESS_ALARM_PAGE, pn) ) ;
	/* turn POR off */
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	UT_setbit(p, 15, 0);
	UT_setbit(p, 31, 0);
	UT_setbit(p, 47, 0);
	UT_setbit(p, 63, 0);
	return OW_w_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn);
}

static GOOD_OR_BAD OW_r_high(int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	y[0] = UT_getbit(p, 8 + 2 + high);
	y[1] = UT_getbit(p, 24 + 2 + high);
	y[2] = UT_getbit(p, 40 + 2 + high);
	y[3] = UT_getbit(p, 56 + 2 + high);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_high(const int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	UT_setbit(p, 8 + 2 + high, y[0] & 0x01);
	UT_setbit(p, 24 + 2 + high, y[1] & 0x01);
	UT_setbit(p, 40 + 2 + high, y[2] & 0x01);
	UT_setbit(p, 56 + 2 + high, y[3] & 0x01);
	/* turn POR off */
	UT_setbit(p, 15, 0);
	UT_setbit(p, 31, 0);
	UT_setbit(p, 47, 0);
	UT_setbit(p, 63, 0);
	return OW_w_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn);
}

static GOOD_OR_BAD OW_r_flag(int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	y[0] = UT_getbit(p, 8 + 4 + high);
	y[1] = UT_getbit(p, 24 + 4 + high);
	y[2] = UT_getbit(p, 40 + 4 + high);
	y[3] = UT_getbit(p, 56 + 4 + high);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_flag(const int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	UT_setbit(p, 8 + 4 + high, y[0] & 0x01);
	UT_setbit(p, 24 + 4 + high, y[1] & 0x01);
	UT_setbit(p, 40 + 4 + high, y[2] & 0x01);
	UT_setbit(p, 56 + 4 + high, y[3] & 0x01);
	return OW_w_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn);
}

static GOOD_OR_BAD OW_r_por(int *y, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	y[0] = UT_getbit(p, 15) || UT_getbit(p, 31) || UT_getbit(p, 47)
		|| UT_getbit(p, 63);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_por(const int por, struct parsedname *pn)
{
	BYTE p[8];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn) ) ;
	UT_setbit(p, 15, por);
	UT_setbit(p, 31, por);
	UT_setbit(p, 47, por);
	UT_setbit(p, 63, por);
	return OW_w_mem(p, 8, _ADDRESS_CONTROL_PAGE, pn);
}


/* Functions for the CO2 sensor */
static ZERO_OR_ERROR FS_CO2_power( struct one_wire_query *owq)
{
	_FLOAT P = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&P,"volt.D",owq) ;

	OWQ_U(owq) = P ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_CO2_ppm( struct one_wire_query *owq)
{
	_FLOAT P = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&P,"volt.A",owq) ;

	OWQ_U(owq) = P*1000. ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_CO2_status( struct one_wire_query *owq)
{
	_FLOAT V = 0.;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&V,"volt.B",owq) ;

	OWQ_Y(owq) = (V>3.0) && (V<3.4) ;
	return z_or_e ;
}
