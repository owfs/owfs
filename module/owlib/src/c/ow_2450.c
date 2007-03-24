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

/* ------- Structures ----------- */

struct aggregate A2450 = { 4, ag_numbers, ag_separate, };
struct aggregate A2450m = { 4, ag_letters, ag_mixed, };
struct aggregate A2450v = { 4, ag_letters, ag_aggregate, };
struct filetype DS2450[] = {
	F_STANDARD,
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 8, &A2450, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
  {"power", 1, NULL, ft_yesno, fc_stable, {o: FS_r_power}, {o: FS_w_power}, {v:NULL},},
  {"memory", 32, NULL, ft_binary, fc_stable, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"PIO", 1, &A2450m, ft_yesno, fc_stable, {o: FS_r_PIO}, {o: FS_w_PIO}, {i:0},},
  {"volt", 12, &A2450m, ft_float, fc_volatile, {o: FS_volts}, {v: NULL}, {i:1},},
  {"volt2", 12, &A2450m, ft_float, fc_volatile, {o: FS_volts}, {v: NULL}, {i:0},},
  {"set_alarm", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"set_alarm/volthigh", 12, &A2450v, ft_float, fc_stable, {o: FS_r_setvolt}, {o: FS_w_setvolt}, {i:3},},
  {"set_alarm/volt2high", 12, &A2450v, ft_float, fc_stable, {o: FS_r_setvolt}, {o: FS_w_setvolt}, {i:2},},
  {"set_alarm/voltlow", 12, &A2450v, ft_float, fc_stable, {o: FS_r_setvolt}, {o: FS_w_setvolt}, {i:1},},
  {"set_alarm/volt2low", 12, &A2450v, ft_float, fc_stable, {o: FS_r_setvolt}, {o: FS_w_setvolt}, {i:0},},
  {"set_alarm/high", 1, &A2450v, ft_yesno, fc_stable, {o: FS_r_high}, {o: FS_w_high}, {i:1},},
  {"set_alarm/low", 1, &A2450v, ft_yesno, fc_stable, {o: FS_r_high}, {o: FS_w_high}, {i:0},},
  {"set_alarm/unset", 1, NULL, ft_yesno, fc_stable, {o: FS_r_por}, {o: FS_w_por}, {v:NULL},},
  {"alarm", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"alarm/high", 1, &A2450v, ft_yesno, fc_volatile, {o: FS_r_flag}, {o: FS_w_flag}, {i:1},},
  {"alarm/low", 1, &A2450v, ft_yesno, fc_volatile, {o: FS_r_flag}, {o: FS_w_flag}, {i:0},},
};

DeviceEntryExtended(20, DS2450, DEV_volt | DEV_alarm | DEV_ovdr);

/* ------- Functions ------------ */

/* DS2450 */
static int OW_r_mem( BYTE * p,  size_t size,  off_t offset, struct parsedname *pn) ;
static int OW_w_mem( BYTE * p,  size_t size,  off_t offset,
					 struct parsedname *pn);
static int OW_volts(_FLOAT * f, const int resolution,
					struct parsedname *pn);
static int OW_1_volts(_FLOAT * f, const int element, const int resolution,
					  struct parsedname *pn);
static int OW_convert(struct parsedname *pn);
static int OW_r_pio(int *pio, struct parsedname *pn);
static int OW_r_1_pio(int *pio, const int element,
					  struct parsedname *pn);
static int OW_w_pio(const int *pio, struct parsedname *pn);
static int OW_w_1_pio(const int pio, const int element,
					  struct parsedname *pn);
static int OW_r_vset(_FLOAT * V, const int high, const int resolution,
					 struct parsedname *pn);
static int OW_w_vset(const _FLOAT * V, const int high,
					 const int resolution, struct parsedname *pn);
static int OW_r_high(int *y, const int high, struct parsedname *pn);
static int OW_w_high(const int *y, const int high,
					 struct parsedname *pn);
static int OW_r_flag(int *y, const int high, struct parsedname *pn);
static int OW_w_flag(const int *y, const int high,
					 struct parsedname *pn);
static int OW_r_por(int *y, struct parsedname *pn);
static int OW_w_por(const int por, struct parsedname *pn);

/* read a page of memory (8 bytes) */
static int FS_r_page(struct one_wire_query * owq)
{
	size_t pagesize = 8 ;
	if ( OWQ_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_r_mem_crc16_AA ) )
		return -EINVAL;
    printf("returning from FS_r_page length=%d\n",(int)OWQ_length(owq)) ;
    return 0 ;
}

/* write an 8-byte page */
static int FS_w_page(struct one_wire_query * owq)
{
	size_t pagesize = 8 ;
    if ( OW_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_w_mem ) )
		return -EINVAL;
	return 0;
}

/* read powered flag */
static int FS_r_power(struct one_wire_query * owq)
{
	BYTE p;
    if (OW_r_mem(&p, 1, 0x1C, PN(owq)))
		return -EINVAL;
//printf("Cont %d\n",p) ;
    OWQ_Y(owq) = (p == 0x40);
	return 0;
}

/* write powered flag */
static int FS_w_power(struct one_wire_query * owq)
{
	BYTE p = 0x40;				/* powered */
	BYTE q = 0x00;				/* parasitic */
    if (OW_w_mem(OWQ_Y(owq) ? &p : &q, 1, 0x1C, PN(owq)))
		return -EINVAL;
	return 0;
}

/* read "unset" PowerOnReset flag */
static int FS_r_por(struct one_wire_query * owq)
{
    if (OW_r_por(&OWQ_Y(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* write "unset" PowerOnReset flag */
static int FS_w_por(struct one_wire_query * owq)
{
    if (OW_w_por(OWQ_Y(owq) & 0x01, PN(owq)))
		return -EINVAL;
	return 0;
}

/* read high/low voltage enable alarm flags */
static int FS_r_high(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int Y[4] ;
    if (OW_r_high(Y, pn->ft->data.i & 0x01, pn))
		return -EINVAL;
    OWQ_array_Y(owq,0) = Y[0] ;
    OWQ_array_Y(owq,1) = Y[1] ;
    OWQ_array_Y(owq,2) = Y[2] ;
    OWQ_array_Y(owq,3) = Y[3] ;
    return 0;
}

/* write high/low voltage enable alarm flags */
static int FS_w_high(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int Y[4] = {
        OWQ_array_Y(owq,0),
        OWQ_array_Y(owq,1),
        OWQ_array_Y(owq,2),
        OWQ_array_Y(owq,3),
    } ;
    if (OW_w_high(Y, pn->ft->data.i & 0x01, pn))
		return -EINVAL;
	return 0;
}

/* read high/low voltage triggered state alarm flags */
static int FS_r_flag(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int Y[4] ;
    if (OW_r_flag(Y, pn->ft->data.i & 0x01, pn))
		return -EINVAL;
    OWQ_array_Y(owq,0) = Y[0] ;
    OWQ_array_Y(owq,1) = Y[1] ;
    OWQ_array_Y(owq,2) = Y[2] ;
    OWQ_array_Y(owq,3) = Y[3] ;
    return 0;
}

/* write high/low voltage triggered state alarm flags */
static int FS_w_flag(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int Y[4] = {
        OWQ_array_Y(owq,0),
        OWQ_array_Y(owq,1),
        OWQ_array_Y(owq,2),
        OWQ_array_Y(owq,3),
    } ;
    if (OW_w_flag(Y, pn->ft->data.i & 0x01, pn))
		return -EINVAL;
	return 0;
}

/* 2450 A/D */
static int FS_r_PIO(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int element = pn->extension;
    int pio_error ;
    if ( element == EXTENSION_ALL ) {
        int y[4]  = {0,0,0,0,} ;
        pio_error = OW_r_pio(y, pn) ;
        OWQ_array_Y(owq,0) = y[0] ;
        OWQ_array_Y(owq,1) = y[1] ;
        OWQ_array_Y(owq,2) = y[2] ;
        OWQ_array_Y(owq,3) = y[3] ;
    } else {
        pio_error = OW_r_1_pio(&OWQ_Y(owq), element, pn);
    }
    if ( pio_error ) return -EINVAL;
    return 0;
}

/* 2450 A/D */
/* pio status -- special code for ag_mixed */
static int FS_w_PIO(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int element = pn->extension;
    int pio_error ;
    if ( element == EXTENSION_ALL ) {
        int y[4] = {
            OWQ_array_Y(owq,0),
            OWQ_array_Y(owq,1),
            OWQ_array_Y(owq,2),
            OWQ_array_Y(owq,3),
        };
        pio_error = OW_w_pio(y, pn) ;
    } else {
        pio_error = OW_w_1_pio(OWQ_Y(owq), element, pn);
    }
    if ( pio_error ) return -EINVAL;
	return 0;
}

/* 2450 A/D */
static int FS_r_mem(struct one_wire_query * owq)
{
	size_t pagesize = 8 ;
	if ( OWQ_readwrite_paged( owq, 0, pagesize, OW_r_mem_crc16_AA ) )
		return -EINVAL;
    return 0 ;
}

/* 2450 A/D */
static int FS_w_mem(struct one_wire_query * owq)
{
	size_t pagesize = 8 ;
    if ( OW_readwrite_paged( owq, 0, pagesize, OW_w_mem ) )
		return -EINVAL;
	return 0;
}

/* 2450 A/D */
static int FS_volts(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int element = pn->extension;
    int volt_error ;
    if ( element == EXTENSION_ALL ) {
        _FLOAT V[4] = {0.,0.,0.,0.,} ;
        volt_error = OW_volts(V, pn->ft->data.i, pn) ;
        OWQ_array_F(owq,0) = V[0] ;
        OWQ_array_F(owq,1) = V[1] ;
        OWQ_array_F(owq,2) = V[2] ;
        OWQ_array_F(owq,3) = V[3] ;
    } else {
        volt_error = OW_1_volts(&OWQ_F(owq), element, pn->ft->data.i, pn) ;
    }
    if (volt_error ) return -EINVAL;
	return 0;
}

static int FS_r_setvolt(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    _FLOAT V[4] ;
    if (OW_r_vset(V, (pn->ft->data.i) >> 1, (pn->ft->data.i) & 0x01, pn))
		return -EINVAL;
    OWQ_array_F(owq,0) = V[0] ;
    OWQ_array_F(owq,1) = V[1] ;
    OWQ_array_F(owq,2) = V[2] ;
    OWQ_array_F(owq,3) = V[3] ;
    return 0;
}

static int FS_w_setvolt(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    _FLOAT V[4] = {
        OWQ_array_F(owq,0),
        OWQ_array_F(owq,1),
        OWQ_array_F(owq,2),
        OWQ_array_F(owq,3),
    } ;
    if (OW_w_vset(V, (pn->ft->data.i) >> 1, (pn->ft->data.i) & 0x01, pn))
		return -EINVAL;
	return 0;
}

static int OW_r_mem( BYTE * p,  size_t size,  off_t offset, struct parsedname *pn)
{
    size_t pagesize = 8 ;
    OWQ_make( owq_read ) ;

    OWQ_create_temporary( owq_read, (char *) p, size, offset, pn ) ;
    return OW_r_mem_crc16_AA( owq_read, 0, pagesize ) ;
}

/* write to 2450 */
static int OW_w_mem( BYTE * p,  size_t size,  off_t offset,
					struct parsedname *pn)
{
	// command, address(2) , data , crc(2), databack
	BYTE buf[7] = { 0x55, offset & 0xFF, (offset >> 8) & 0xFF, p[0], };
	size_t i;
	struct transaction_log tfirst[] = {
		TRXN_START,
		{buf, NULL, 4, trxn_match},
		{NULL, &buf[4], 3, trxn_read},
		TRXN_END,
	};
	struct transaction_log trest[] = {	// note no TRXN_START
		{buf, NULL, 1, trxn_match},
		{NULL, &buf[1], 3, trxn_read},
		TRXN_END,
	};
//printf("2450 W mem size=%d location=%d\n",size,location) ;

	if (size == 0)
		return 0;

	/* Send the first byte (handled differently) */
	if (BUS_transaction(tfirst, pn) || CRC16(buf, 6) || (buf[6] != p[0]))
		return 1;
	/* rest of the bytes */
	for (i = 1; i < size; ++i) {
		buf[0] = p[i];
		if (BUS_transaction(trest, pn) || CRC16seeded(buf, 3, offset + i)
			|| (buf[3] != p[i]))
			return 1;
	}
	return 0;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
/* resolution is 1->5.10V 0->2.55V */
static int OW_volts(_FLOAT * f, const int resolution,
					struct parsedname *pn)
{
	BYTE control[8];
	BYTE data[8];
	int i;
	int writeback = 0;			/* write control back? */
	//printf("Volts res=%d\n",resolution);OW_r_mem
	// Get control registers and set to A/D 16 bits
	if (OW_r_mem(control, 8, 1 << 3, pn))
		return 1;
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
	if (writeback)
		if (OW_w_mem(control, 8, 1 << 3, pn))
			return 1;
	//printf("writeback=%d\n",writeback);
	// Start A/D process if needed
	if (OW_convert(pn)) {
		LEVEL_DEFAULT("OW_volts: Failed to start conversion\n");
		return 1;
	}
	// read data
	if (OW_r_mem(data, 8, 0 << 3, pn)) {
		LEVEL_DEFAULT("OW_volts: OW_r_mem_crc16_AA Failed\n");
		return 1;
	}
	//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

	// data conversions
	f[0] =
		(resolution ? 7.8126192E-5 : 3.90630961E-5) *
		((((UINT) data[1]) << 8) | data[0]);
	f[1] =
		(resolution ? 7.8126192E-5 : 3.90630961E-5) *
		((((UINT) data[3]) << 8) | data[2]);
	f[2] =
		(resolution ? 7.8126192E-5 : 3.90630961E-5) *
		((((UINT) data[5]) << 8) | data[4]);
	f[3] =
		(resolution ? 7.8126192E-5 : 3.90630961E-5) *
		((((UINT) data[7]) << 8) | data[6]);
	return 0;
}


/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution on a single channel */
/* resolution is 1->5.10V 0->2.55V */
static int OW_1_volts(_FLOAT * f, const int element, const int resolution,
					  struct parsedname *pn)
{
	BYTE control[2];
	BYTE data[2];
	int writeback = 0;			/* write control back? */

	// Get control registers and set to A/D 16 bits
	if (OW_r_mem(control, 2, (1 << 3) + (element << 1), pn))
		return 1;
	if (control[0] & 0x0F) {
		control[0] &= 0xF0;		// 16bit, A/D
		writeback = 1;
	}
	if ((control[1] & 0x01) != (resolution & 0x01)) {
		UT_setbit(&control[1], 0, resolution);
		writeback = 1;
	}
	// Set control registers
	if (writeback)
		if (OW_w_mem(control, 2, (1 << 3) + (element << 1), pn))
			return 1;

	// Start A/D process
	if (OW_convert(pn)) {
		LEVEL_DEFAULT("OW_1_volts: Failed to start conversion\n");
		return 1;
	}
	// read data
	if (OW_r_mem(data, 2, (0 << 3) + (element << 1), pn)) {
		LEVEL_DEFAULT("OW_volts: OW_r_mem_crc16_AA failed\n");
		return 1;
	}
//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

	// data conversions
	//f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[1])<<8)|data[0]) ;
	f[0] = (resolution ? 7.8126192E-5 : 3.90630961E-5) * UT_uint16(data);
	return 0;
}

/* send A/D conversion command */
static int OW_convert(struct parsedname *pn)
{
	BYTE convert[] = { 0x3C, 0x0F, 0x00, 0xFF, 0xFF, };
	BYTE power;
	struct transaction_log tpower[] = {
		TRXN_START,
		{convert, NULL, 3, trxn_match},
		{NULL, &convert[3], 2, trxn_read},
		TRXN_END,
	};
	struct transaction_log tdead[] = {
		TRXN_START,
		{convert, NULL, 3, trxn_match},
		{NULL, &convert[3], 1, trxn_read},
		{&convert[4], &convert[4], 6, trxn_power},
		TRXN_END,
	};

	/* get power flag -- to see if pullup can be avoided */
	if (OW_r_mem(&power, 1, 0x1C, pn))
		return 1;

	/* See if a conversion was globally triggered */
	if (power == 0x40 && Simul_Test(simul_volt, pn) == 0)
		return 0;

	// Start conversion
	// 6 msec for 16bytex4channel (5.2)
	if (power == 0x40) {		//powered
		if (BUS_transaction(tpower, pn) || CRC16(convert, 5))
			return 1;
		UT_delay(6);			/* don't need to hold line for conversion! */
	} else {
		if (BUS_transaction(tdead, pn) || CRC16(convert, 5))
			return 1;
	}
	return 0;
}

/* read all the pio registers */
static int OW_r_pio(int *pio, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	pio[0] = ((p[0] & 0xC0) != 0x80);
	pio[1] = ((p[2] & 0xC0) != 0x80);
	pio[2] = ((p[4] & 0xC0) != 0x80);
	pio[3] = ((p[6] & 0xC0) != 0x80);
	return 0;
}

/* read one pio register */
static int OW_r_1_pio(int *pio, const int element,
					  struct parsedname *pn)
{
	BYTE p[2];
	if (OW_r_mem(p, 2, (1 << 3) + (element << 1), pn))
		return 1;
	pio[0] = ((p[0] & 0xC0) != 0x80);
	return 0;
}

/* Write all the pio registers */
static int OW_w_pio(const int *pio, struct parsedname *pn)
{
	BYTE p[8];
	int i;
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	for (i = 0; i <= 3; ++i) {
		p[i << 1] &= 0x3F;
		p[i << 1] |= pio[i] ? 0xC0 : 0x80;
	}
	return OW_w_mem(p, 8, 1 << 3, pn);
}

/* write just one pio register */
static int OW_w_1_pio(const int pio, const int element,
					  struct parsedname *pn)
{
	BYTE p[2];
	if (OW_r_mem(p, 2, (1 << 3) + (element << 1), pn))
		return 1;
	p[0] &= 0x3F;
	p[0] |= pio ? 0xC0 : 0x80;
	return OW_w_mem(p, 2, (1 << 3) + (element << 1), pn);
}

static int OW_r_vset(_FLOAT * V, const int high, const int resolution,
					 struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 2 << 3, pn))
		return 1;
	V[0] = (resolution ? .02 : .01) * p[0 + high];
	V[1] = (resolution ? .02 : .01) * p[2 + high];
	V[2] = (resolution ? .02 : .01) * p[4 + high];
	V[3] = (resolution ? .02 : .01) * p[6 + high];
	return 0;
}

static int OW_w_vset(const _FLOAT * V, const int high,
					 const int resolution, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 2 << 3, pn))
		return 1;
	p[0 + high] = V[0] * (resolution ? 50. : 100.);
	p[2 + high] = V[1] * (resolution ? 50. : 100.);
	p[4 + high] = V[2] * (resolution ? 50. : 100.);
	p[6 + high] = V[3] * (resolution ? 50. : 100.);
	if (OW_w_mem(p, 8, 2 << 3, pn))
		return 1;
	/* turn POR off */
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	UT_setbit(p, 15, 0);
	UT_setbit(p, 31, 0);
	UT_setbit(p, 47, 0);
	UT_setbit(p, 63, 0);
	return OW_w_mem(p, 8, 1 << 3, pn);
}

static int OW_r_high(int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	y[0] = UT_getbit(p, 8 + 2 + high);
	y[1] = UT_getbit(p, 24 + 2 + high);
	y[2] = UT_getbit(p, 40 + 2 + high);
	y[3] = UT_getbit(p, 56 + 2 + high);
	return 0;
}

static int OW_w_high(const int *y, const int high,
					 struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	UT_setbit(p, 8 + 2 + high, y[0] & 0x01);
	UT_setbit(p, 24 + 2 + high, y[1] & 0x01);
	UT_setbit(p, 40 + 2 + high, y[2] & 0x01);
	UT_setbit(p, 56 + 2 + high, y[3] & 0x01);
	/* turn POR off */
	UT_setbit(p, 15, 0);
	UT_setbit(p, 31, 0);
	UT_setbit(p, 47, 0);
	UT_setbit(p, 63, 0);
	return OW_w_mem(p, 8, 1 << 3, pn);
}

static int OW_r_flag(int *y, const int high, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	y[0] = UT_getbit(p, 8 + 4 + high);
	y[1] = UT_getbit(p, 24 + 4 + high);
	y[2] = UT_getbit(p, 40 + 4 + high);
	y[3] = UT_getbit(p, 56 + 4 + high);
	return 0;
}

static int OW_w_flag(const int *y, const int high,
					 struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	UT_setbit(p, 8 + 4 + high, y[0] & 0x01);
	UT_setbit(p, 24 + 4 + high, y[1] & 0x01);
	UT_setbit(p, 40 + 4 + high, y[2] & 0x01);
	UT_setbit(p, 56 + 4 + high, y[3] & 0x01);
	return OW_w_mem(p, 8, 1 << 3, pn);
}

static int OW_r_por(int *y, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	y[0] = UT_getbit(p, 15) || UT_getbit(p, 31) || UT_getbit(p, 47)
		|| UT_getbit(p, 63);
	return 0;
}

static int OW_w_por(const int por, struct parsedname *pn)
{
	BYTE p[8];
	if (OW_r_mem(p, 8, 1 << 3, pn))
		return 1;
	UT_setbit(p, 15, por);
	UT_setbit(p, 31, por);
	UT_setbit(p, 47, por);
	UT_setbit(p, 63, por);
	return OW_w_mem(p, 8, 1 << 3, pn);
}
