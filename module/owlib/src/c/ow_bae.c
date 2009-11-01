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

/* Pascal Baerten's BAE device -- preliminary */

#include <config.h>
#include "owfs_config.h"
#include "ow_bae.h"

/* ------- Prototypes ----------- */

/* BAE */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_flash);
WRITE_FUNCTION(FS_w_flash);
WRITE_FUNCTION(FS_w_extended);
WRITE_FUNCTION(FS_writebyte);

WRITE_FUNCTION(FS_w_date);
READ_FUNCTION(FS_r_date);

READ_FUNCTION(FS_version_state) ;
READ_FUNCTION(FS_version) ;
READ_FUNCTION(FS_version_device) ;
READ_FUNCTION(FS_version_bootstrap) ;

READ_FUNCTION(FS_type_state) ;
READ_FUNCTION(FS_localtype) ;
READ_FUNCTION(FS_type_device) ;
READ_FUNCTION(FS_type_chip) ;

READ_FUNCTION(FS_r_8) ;
WRITE_FUNCTION(FS_w_8) ;
READ_FUNCTION(FS_r_16) ;
WRITE_FUNCTION(FS_w_16) ;
READ_FUNCTION(FS_r_32) ;
WRITE_FUNCTION(FS_w_32) ;

/* ------- Structures ----------- */

#define _FC02_MEMORY_SIZE 192
#define _FC02_FUNCTION_FLASH_SIZE 0x1000
#define _FC02_FUNCTION_FLASH_OFFSET 0xE200

#define _FC02_MAX_WRITE_GULP	32
#define _FC02_MAX_READ_GULP	32

/* BAE registers */
#define _FC02_ADC  50    /* u8 */
#define _FC02_ADCAN  24    /* u16 */
#define _FC02_ADCAP  22    /* u16 */
#define _FC02_ADCC  2    /* u8 */
#define _FC02_ADCTOTN  36    /* u32 */
#define _FC02_ADCTOTP  32    /* u32 */
#define _FC02_ALAN  66    /* u16 */
#define _FC02_ALAP  64    /* u16 */
#define _FC02_ALARM  52    /* u8 */
#define _FC02_ALARMC  6    /* u8 */
#define _FC02_ALCD  68    /* u16 */
#define _FC02_ALCT  70    /* u32 */
#define _FC02_ALRT  74    /* u32 */
#define _FC02_CNT  51    /* u8 */
#define _FC02_CNTC  3    /* u8 */
#define _FC02_COUNT  44    /* u32 */
#define _FC02_DUTY1  14    /* u16 */
#define _FC02_DUTY2  16    /* u16 */
#define _FC02_DUTY3  18    /* u16 */
#define _FC02_DUTY4  20    /* u16 */
#define _FC02_MAXAN  28    /* u16 */
#define _FC02_MAXAP  26    /* u16 */
#define _FC02_MOD1  10    /* u16 */
#define _FC02_MOD2  12    /* u16 */
#define _FC02_MSIZE  0    /* u16 */
#define _FC02_OUT  48    /* u8 */
#define _FC02_OUTC  4    /* u8 */
#define _FC02_PIO  49    /* u8 */
#define _FC02_PIOC  5    /* u8 */
#define _FC02_RTC  40    /* u32 */
#define _FC02_RTCC  7    /* u8 */
#define _FC02_TPM1C  8    /* u8 */
#define _FC02_TPM2C  9    /* u8 */


struct filetype BAE[] = {
	F_STANDARD,
	{"memory", _FC02_MEMORY_SIZE, NULL, ft_binary, fc_read_stable, FS_r_mem, FS_w_mem, NO_FILETYPE_DATA,},
	{"command", 32, NULL, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_extended, NO_FILETYPE_DATA,},
  {"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, NO_READ_FUNCTION, FS_writebyte, NO_FILETYPE_DATA, },
  {"versionstate", PROPERTY_LENGTH_HIDDEN, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_version_state, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
  {"version", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_version, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"device_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_device, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"bootstrap_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_bootstrap, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"typestate", PROPERTY_LENGTH_HIDDEN, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_type_state, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
  {"localtype", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"device_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_device, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"chip_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_chip, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},

  {"firmware", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"firmware/function", _FC02_FUNCTION_FLASH_SIZE, NULL, ft_binary, fc_stable, FS_r_flash, FS_w_flash, NO_FILETYPE_DATA,},
  
  {"910", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"910/adcc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_ADCC,}, },
  {"910/cntc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_CNTC,}, },
  {"910/outs", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_OUTC,}, },
  {"910/pioc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_PIOC,}, },
  {"910/alarmc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_ALARMC,}, },
  {"910/rtcc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_RTCC,}, },
  {"910/tpm1c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_TPM1C,}, },
  {"910/tpm2c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_TPM2C,}, },
  {"910/mod1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_MOD1,}, },
  {"910/mod2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_MOD2,}, },
  {"910/duty1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_DUTY1,}, },
  {"910/duty2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_DUTY2,}, },
  {"910/duty3", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_DUTY3,}, },
  {"910/duty4", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_DUTY4,}, },
  {"910/alap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_ALAP,}, },
  {"910/alan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_ALAN,}, },
  {"910/alcd", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, {u:_FC02_ALCD,}, },
  {"910/alct", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_32, FS_w_32, {u:_FC02_ALCT,}, },
  {"910/alrt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_32, FS_w_32, {u:_FC02_ALRT,}, },
  {"910/adcap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, {u:_FC02_ADCAP,}, },
  {"910/adcan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, {u:_FC02_ADCAN,}, },
  {"910/maxap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, {u:_FC02_MAXAP,}, },
  {"910/maxan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, {u:_FC02_MAXAN,}, },
  {"910/adctotp", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, {u:_FC02_ADCTOTP,}, },
  {"910/adctotn", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, {u:_FC02_ADCTOTN,}, },
  {"910/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, FS_w_32, {u:_FC02_RTC,}, },
  {"910/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, FS_r_date, FS_w_date, NO_FILETYPE_DATA,},
  {"910/count", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, {u:_FC02_COUNT,}, },
  {"910/out", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, {u:_FC02_OUT,}, },
  {"910/pio", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, {u:_FC02_PIO,}, },
  {"910/adc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, {u:_FC02_ADC,}, },
  {"910/cnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, {u:_FC02_CNT,}, },
  {"910/alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, {u:_FC02_ALARM,}, },
};

DeviceEntryExtended(FC, BAE, DEV_resume | DEV_alarm );

/* <AE command codes */
#define _1W_ERASE_FIRMWARE 0xBB
#define _1W_FLASH_FIRMWARE 0xBA

#define _1W_READ_VERSION 0x11
#define _1W_READ_TYPE 0x12
#define _1W_EXTENDED_COMMAND 0x13
#define _1W_READ_BLOCK_WITH_LEN 0x14
#define _1W_WRITE_BLOCK_WITH_LEN 0x15

#define _1W_CONFIRM_WRITE 0xBC

/* ------- Functions ------------ */

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static int OW_w_extended(BYTE * data, size_t size, UINT * return_code, struct parsedname *pn);
static int OW_version( UINT * version, struct parsedname * pn ) ;
static int OW_type( UINT * localtype, struct parsedname * pn ) ;
static int OW_r_mem(BYTE *bytes, size_t size, off_t offset, struct parsedname * pn);
static int OW_r_mem_small(BYTE *bytes, size_t size, off_t offset, struct parsedname * pn);

static int OW_initiate_flash(BYTE * data, struct parsedname *pn);
static int OW_write_flash(BYTE * data, struct parsedname *pn);

static uint16_t BAE_uint16(BYTE * p);
static uint32_t BAE_uint32(BYTE * p);
static void BAE_uint16_to_bytes( uint16_t num, unsigned char * p );
static void BAE_uint32_to_bytes( uint32_t num, unsigned char * p );

/* BAE memory functions */
static int FS_r_mem(struct one_wire_query *owq)
{
	if (OW_r_mem( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq))) {
		return -EINVAL;
	}
	OWQ_length(owq) = OWQ_size(owq) ;
	return 0;
}

static int FS_w_mem(struct one_wire_query *owq)
{
	size_t remain = OWQ_size(owq) ;
	BYTE * data = (BYTE *) OWQ_buffer(owq) ;
	off_t location = OWQ_offset(owq) ;
	
	// Write data 32 bytes (_FC02_MAX_WRITE_GULP) at a time ignoring page boundaries
	while ( remain > 0 ) {
		size_t bolus = remain ;
		if ( bolus > _FC02_MAX_WRITE_GULP ) {
			bolus = _FC02_MAX_WRITE_GULP ;
		}
		if ( OW_w_mem(data, bolus, location, PN(owq)  ) ) {
			return -EINVAL ;
		}
		remain -= bolus ;
		data += bolus ;
		location += bolus ;
	}
	
	return 0;
}

/* BAE flash functions */
static int FS_w_flash(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE * rom_image = (BYTE *) OWQ_buffer(owq) ;

	size_t rom_offset ;

	// test size
	if ( OWQ_size(owq) != _FC02_FUNCTION_FLASH_SIZE ) {
		LEVEL_DEBUG("Flash size of %d is not the expected %d.\n", (int)OWQ_size(owq), (int)_FC02_FUNCTION_FLASH_SIZE ) ;
		return -EBADMSG ;
	}

	if ( OWQ_offset(owq) != 0 ) {
		LEVEL_DEBUG("Can only write flash from start of buffer.\n") ;
		return -ERANGE ;
	}

	// start flash process
	if ( OW_initiate_flash( rom_image, pn ) ) {
		LEVEL_DEBUG("Unsuccessful flash initialization\n");
		return -EFAULT ;
	}

	// loop though pages, up to 5 attempts for each page
	for ( rom_offset=0 ; rom_offset<_FC02_FUNCTION_FLASH_SIZE ; rom_offset += _FC02_MAX_WRITE_GULP ) {
		int tries = 0 ;
		LEVEL_DEBUG("Flash up to %d bytes.\n",rom_offset);
		while ( OW_write_flash( &rom_image[rom_offset], pn ) ) {
			++tries ;
			if ( tries > 4 ) {
				LEVEL_DEBUG( "Too many failures writing flash at offset %d.\n", rom_offset ) ;
				return -EIO ;
			}
		}
	}
	
	LEVEL_DEBUG("Successfully flashed full rom.\n") ;
	return 0;
}

static int FS_r_flash( struct one_wire_query *owq)
{
	if ( OW_r_mem( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq)+_FC02_FUNCTION_FLASH_OFFSET, PN(owq) ) ) {
		return -EINVAL ;
	}
	OWQ_length(owq) = OWQ_size(owq) ;
	return 0 ;
}

/* BAE date/counter functions */
static int FS_r_date(struct one_wire_query *owq)
{
	UINT u ;
	if ( FS_r_sibling_U( &u, "910/udate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_D(owq) = (_DATE) u ;
	return 0;
}

static int FS_w_date(struct one_wire_query *owq)
{
	UINT u = (UINT) OWQ_D(owq) ; //register representation

	return FS_w_sibling_U( u, "910/udate", owq ) ;
}

/* BAE extended command */
static int FS_w_extended(struct one_wire_query *owq)
{
	UINT ret ;
	// Write data 32 bytes maximum
	if ( OW_w_extended( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), &ret, PN(owq)  ) ) {
		return -EINVAL ;
	}

	if ( ret == 0xFFFF ) {
		LEVEL_DEBUG("Bad return code for extended command\n") ;
		return -EINVAL ;
	}
	
	return 0;
}

static int FS_writebyte(struct one_wire_query *owq)
{
	off_t location = OWQ_U(owq)>>8 ;
	BYTE data = OWQ_U(owq) & 0xFF ;
	
	// Write 1 byte ,
	if ( OW_w_mem( &data, 1, location, PN(owq)  ) ) {
		return -EINVAL ;
	}
	
	return 0;
}

/* BAE version */
static int FS_version_state(struct one_wire_query *owq)
{
	UINT v ;
	if ( OW_version( &v, PN(owq) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = v ;
	return 0 ;
}

static int FS_version(struct one_wire_query *owq)
{
	char v[6];
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(v,6,"%.2X.%.2X",version&0xFF, (version>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return Fowq_output_offset_and_size(v, 5, owq);
}

static int FS_version_device(struct one_wire_query *owq)
{
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = (version>>8) & 0xFF ;
	
	return 0 ;
}

static int FS_version_bootstrap(struct one_wire_query *owq)
{
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = version & 0xFF ;
	
	return 0 ;
}

/* BAE type */
static int FS_type_state(struct one_wire_query *owq)
{
	UINT t ;
	if ( OW_type( &t, PN(owq) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = t ;
	return 0 ;
}

static int FS_localtype(struct one_wire_query *owq)
{
	char t[6];
	UINT localtype ;
	
	if ( FS_r_sibling_U( &localtype, "typestate", owq ) ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(t,6,"%.2X.%.2X",localtype&0xFF, (localtype>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return Fowq_output_offset_and_size(t, 5, owq);
}

static int FS_type_device(struct one_wire_query *owq)
{
	UINT t ;
	
	if ( FS_r_sibling_U( &t, "typestate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = (t>>8) & 0xFF ;
	
	return 0 ;
}

static int FS_type_chip(struct one_wire_query *owq)
{
	UINT t ;
	
	if ( FS_r_sibling_U( &t, "typestate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = t & 0xFF ;
	
	return 0 ;
}

/* read an 8 bit value from a register stored in filetype.data */
static int FS_r_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[1] ; // 8/8 = 1
	if ( OW_r_mem_small(data, 1, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = data[0] ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data */
static int FS_w_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[1] ; // 8/8 = 1
	
	data[0] = BYTE_MASK( OWQ_U(owq) ) ;
	if ( OW_w_mem(data, 1, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	return 0 ;
}

/* read a 16 bit value from a register stored in filetype.data */
static int FS_r_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[2] ; // 16/8 = 2
	if ( OW_r_mem_small(data, 2, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = BAE_uint16(data) ;
	return 0 ;
}

/* write a 16 bit value from a register stored in filetype.data */
static int FS_w_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[2] ; // 16/8 = 2

	BAE_uint16_to_bytes( OWQ_U(owq), data ) ;
	if ( OW_w_mem(data, 2, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	return 0 ;
}

/* read a 32 bit value from a register stored in filetype.data */
static int FS_r_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[4] ; // 32/8 = 4
	if ( OW_r_mem_small(data, 4, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = BAE_uint32(data) ;
	return 0 ;
}

/* write a 32 bit value from a register stored in filetype.data */
static int FS_w_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[4] ; // 32/8 = 2
	
	BAE_uint32_to_bytes( OWQ_U(owq), data ) ;
	if ( OW_w_mem(data, 4, pn->selected_filetype->data.u, pn ) ) {
		return -EINVAL ;
	}
	return 0 ;
}

/* Lower level functions */
/* size in already constrained to 32 bytes (_FC02_MAX_WRITE_GULP) */
static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 1 + _FC02_MAX_WRITE_GULP + 2] = { _1W_WRITE_BLOCK_WITH_LEN, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 2 + 1 + size, 0),
		TRXN_WRITE1(q),
		TRXN_END,
	};
	
	/* Copy to scratchpad */
	memcpy(&p[4], data, size);
	
	return BUS_transaction(t, pn) ;
}

static int OW_w_extended(BYTE * data, size_t size, UINT * return_code, struct parsedname *pn)
{
	BYTE p[1 + 1 + _FC02_MAX_WRITE_GULP + 2] = { _1W_EXTENDED_COMMAND, BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	BYTE r[2] ;
	int ret ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 1 + size, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(10),
		TRXN_READ2(r),
		TRXN_END,
	};
	
	/* Copy to scratchpad */
	memcpy(&p[4], data, size);
	
	ret = BUS_transaction(t, pn) ;
	if (ret) {
		return 1 ;
	}
	return_code[0] = (r[1]<<8) + r[0] ;
	return 0 ;
}

//read bytes[size] from position
static int OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	size_t remain = size ;
	off_t location = 0 ;
	
	while ( remain > 0 ) {
		size_t bolus = remain ;
		if ( bolus > _FC02_MAX_READ_GULP ) {
			bolus = _FC02_MAX_READ_GULP ;
		}
		if ( OW_r_mem_small( &data[location], bolus, offset+location, pn )) {
			return 1 ;
		}
		remain -= bolus ;
		location += bolus ;
	}
	return 0 ;	
}

/* Already constrained to _FC02_MAX_READ_GULP aliquots */
static int OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	BYTE p[1+2+1 + _FC02_MAX_READ_GULP + 2] = { _1W_READ_BLOCK_WITH_LEN, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, size),
		TRXN_END,
	};
		
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	memcpy(data, &p[4], size);
	return 0;
}

static int OW_version( UINT * version, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_VERSION, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
		
	if (BUS_transaction(t, pn)) {
		return 1;
	}

	version[0] = BAE_uint16(&p[1]) ;
	return 0 ;
};

static int OW_type( UINT * localtype, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_TYPE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
	
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	
	localtype[0] = BAE_uint16(&p[1]) ;
	return 0 ;
};

/* Routines to play with byte <-> integer */
static uint16_t BAE_uint16(BYTE * p)
{
	return (((uint16_t) p[0]) << 8) | ((uint16_t) p[1]);
}

static uint32_t BAE_uint32(BYTE * p)
{
	return (((uint32_t) p[0]) << 24) | (((uint32_t) p[1]) << 16) | (((uint32_t) p[2]) << 8) | ((uint32_t) p[3]);
}

static void BAE_uint16_to_bytes( uint16_t num, unsigned char * p )
{
	p[1] = num&0xFF ;
	p[0] = (num>>8)&0xFF ;
}

static void BAE_uint32_to_bytes( uint32_t num, unsigned char * p )
{
	p[3] = num&0xFF ;
	p[2] = (num>>8)&0xFF ;
	p[1] = (num>>16)&0xFF ;
	p[0] = (num>>24)&0xFF ;
}

static int OW_initiate_flash( BYTE * data, struct parsedname * pn )
{
	BYTE p[1+1+1+4+2] = { _1W_EXTENDED_COMMAND, _1W_ERASE_FIRMWARE, 4, } ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+1+4, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(180),
		TRXN_END,
	} ;

	memcpy(&p[3], data, 4 ) ;
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	return 0 ;
}

static int OW_write_flash( BYTE * data, struct parsedname * pn )
{
	BYTE p[1+1+1+32+2] = { _1W_EXTENDED_COMMAND, _1W_FLASH_FIRMWARE, 32,  } ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+1+32, 0),
		TRXN_WRITE1(q),
		TRXN_END,
	} ;
	
	memcpy(&p[3], data, 32 ) ;
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	return 0 ;
}
