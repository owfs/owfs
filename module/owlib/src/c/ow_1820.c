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
#include "ow_1820.h"

/* ------- Prototypes ----------- */
/* DS18S20&2 Temperature */
// READ_FUNCTION( FS_tempdata ) ;
READ_FUNCTION(FS_10temp);
READ_FUNCTION(FS_10temp_link);
READ_FUNCTION(FS_22temp);
READ_FUNCTION(FS_MAXtemp);
READ_FUNCTION(FS_fasttemp);
READ_FUNCTION(FS_slowtemp);
READ_FUNCTION(FS_power);
READ_FUNCTION(FS_r_templimit);
WRITE_FUNCTION(FS_w_templimit);
READ_FUNCTION(FS_r_die);
READ_FUNCTION(FS_r_trim);
WRITE_FUNCTION(FS_w_trim);
READ_FUNCTION(FS_r_trimvalid);
READ_FUNCTION(FS_r_blanket);
WRITE_FUNCTION(FS_w_blanket);
READ_FUNCTION(FS_r_ad);
READ_FUNCTION(FS_r_bit7);
READ_FUNCTION(FS_r_piostate);
READ_FUNCTION(FS_r_pio);
READ_FUNCTION(FS_r_latch);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_scratchpad);

static enum e_visibility VISIBLE_DS1825( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_MAX31826( const struct parsedname * pn ) ;


/* -------- Structures ---------- */
static struct filetype DS18S20[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_10temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_10temp_link, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_10temp_link, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_10temp_link, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_10temp_link, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"fasttemp",      PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_10temp_link, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1}, },
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0}, },
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"scratchpad", 9, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_scratchpad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/die", 2, NON_AGGREGATE, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:1}, },
	{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA, },

};

DeviceEntryExtended(10, DS18S20, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS18B20[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10}, },
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11}, },
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1}, },
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0}, },
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"scratchpad", 9, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_scratchpad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/die", 2, NON_AGGREGATE, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:2}, },
	{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(28, DS18B20, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS1822[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10}, },
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11}, },
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1}, },
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0}, },
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"scratchpad", 9, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_scratchpad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/die", 2, NON_AGGREGATE, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:0}, },
	{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(22, DS1822, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate AMAX = { 16, ag_numbers, ag_separate, };
static struct filetype DS1825[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_MAXtemp, NO_WRITE_FUNCTION, VISIBLE_DS1825, {i:9}, },
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_MAXtemp, NO_WRITE_FUNCTION, VISIBLE_DS1825, {i:10}, },
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_MAXtemp, NO_WRITE_FUNCTION, VISIBLE_DS1825, {i:11}, },
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_MAXtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE_DS1825, {i:9}, },
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE_DS1825, {i:1}, },
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE_DS1825, {i:0}, },
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"prog_addr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_ad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"bit7", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_bit7, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE_MAX31826, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_MAX31826, NO_FILETYPE_DATA, },
	{"pages/page", 8, &AMAX, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE_MAX31826, NO_FILETYPE_DATA, },
	{"scratchpad", 9, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_scratchpad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(3B, DS1825, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate A28EA00 = { 2, ag_letters, ag_aggregate, };
static struct filetype DS28EA00[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10}, },
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11}, },
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12}, },
	{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9}, },
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1}, },
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0}, },
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"piostate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_piostate, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"PIO", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA, },
	{"latch", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_r_latch, FS_w_pio, VISIBLE, NO_FILETYPE_DATA, },
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(42, DS28EA00, DEV_temp | DEV_alarm | DEV_chain, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Internal properties */
Make_SlaveSpecificTag(RES, fc_stable);	// resolution
Make_SlaveSpecificTag(POW, fc_stable);	// power status

struct tempresolution {
	int bits;
	BYTE config;
	UINT delay;
	BYTE mask;
};
struct tempresolution Resolution9  = { 9, 0x1F, 110, 0xF8} ;			/*  9 bit */
struct tempresolution Resolution10 = {10, 0x3F, 200, 0xFC} ;			/* 10 bit */
struct tempresolution Resolution11 = {11, 0x5F, 400, 0xFE} ;			/* 11 bit */
struct tempresolution Resolution12 = {12, 0x7F, 1000, 0xFF};			/* 12 bit */
struct tempresolution ResolutionMAX = {12, 0x7F, 150, 0xFF};			/* 12 bit */

struct die_limits {
	BYTE B7[6];
	BYTE C2[6];
};

enum eDie { eB6, eB7, eC2, eC3, };

// ID ranges for the different chip dies
struct die_limits DIE[] = {
	{							// DS1822 Family code 22
	 {0x00, 0x00, 0x00, 0x08, 0x97, 0x8A},
	 {0x00, 0x00, 0x00, 0x0C, 0xB8, 0x1A},
	 },
	{							// DS18S20 Family code 10
	 {0x00, 0x08, 0x00, 0x59, 0x1D, 0x20},
	 {0x00, 0x08, 0x00, 0x80, 0x88, 0x60},
	 },
	{							// DS18B20 Family code 28
	 {0x00, 0x00, 0x00, 0x54, 0x50, 0x10},
	 {0x00, 0x00, 0x00, 0x66, 0x2B, 0x50},
	 },
};

/* Intermediate cached values  -- start with unlikely asterisk */
/* RES -- resolution
   POW -- power
*/

#define _1W_WRITE_SCRATCHPAD      0x4E
#define _1W_READ_SCRATCHPAD       0xBE
#define _1W_COPY_SCRATCHPAD       0x48
#define _1W_CONVERT_T             0x44
#define _1W_READ_POWERMODE        0xB4
#define _1W_RECALL_EEPROM         0xB8
#define _1W_PIO_ACCESS_READ       0xF5
#define _1W_PIO_ACCESS_WRITE      0xA5
#define _1W_CHAIN_COMMAND         0x99
#define _1W_CHAIN_SUBCOMMAND_OFF  0x3C
#define _1W_CHAIN_SUBCOMMAND_ON   0x5A
#define _1W_CHAIN_SUBCOMMAND_DONE 0x96

#define _1W_READ_TRIM_1           0x93
#define _1W_READ_TRIM_2           0x68
#define _1W_WRITE_TRIM_1          0x95
#define _1W_WRITE_TRIM_2          0x63
#define _1W_ACTIVATE_TRIM_1       0x94
#define _1W_ACTIVATE_TRIM_2       0x64
#define _DEFAULT_BLANKET_TRIM_1   0x9D
#define _DEFAULT_BLANKET_TRIM_2   0xBB

#define _1W_WRITE_SCRATCHPAD2     0x0F
#define _1W_READ_SCRATCHPAD2      0xAA
#define _1W_COPY_SCRATCHPAD2      0x55
#define _1W_COPY_SCRATCHPAD2_DO   0xA5
#define _1W_READ_MEMORY           0xF0

enum temperature_problem_flag { allow_85C, deny_85C, } ;

/* ------- Functions ------------ */

/* DS1820&2*/
static GOOD_OR_BAD OW_10temp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, const struct parsedname *pn);
static GOOD_OR_BAD OW_22temp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, struct tempresolution * Resolution, const struct parsedname *pn);
static GOOD_OR_BAD OW_MAXtemp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, struct tempresolution * Resolution, const struct parsedname *pn);
static GOOD_OR_BAD OW_power(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_scratchpad(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_scratchpad(const BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_store_scratchpad(const BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_trim(BYTE * trim, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_trim(const BYTE * trim, const struct parsedname *pn);
static enum eDie OW_die(const struct parsedname *pn);
static GOOD_OR_BAD OW_w_pio(BYTE pio, const struct parsedname *pn);
static GOOD_OR_BAD OW_read_piostate(UINT * piostate, const struct parsedname *pn) ;
static _FLOAT OW_masked_temperature( BYTE * data, BYTE mask ) ;

static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_w_mem( BYTE * data, size_t size, off_t offset, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_w_page( BYTE * data, size_t size, off_t offset, struct parsedname * pn ) ;

/* finds the visibility value (0x0071 ...) either cached, or computed via the device_id (then cached) */
static int VISIBLE_3B( const struct parsedname * pn )
{
	int bit7 = -1 ;
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &bit7, pn ) ) ) {
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		if ( owq != NULL) {
			UINT U_bit7 ;
			if ( FS_r_sibling_U( &U_bit7, "bit7", owq ) == 0 ) {
				bit7 = U_bit7 ;
				SetVisibilityCache( bit7, pn ) ;
			}
			OWQ_destroy(owq) ;
		}
	}
	return bit7 ;
}

static enum e_visibility VISIBLE_DS1825( const struct parsedname * pn )
{
	switch ( VISIBLE_3B(pn) ) {
		case 0x0000:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_MAX31826( const struct parsedname * pn )
{
	switch ( VISIBLE_3B(pn) ) {
		case 0x80:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static ZERO_OR_ERROR FS_10temp(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	
	// triple try temperatures
	// first pass include simultaneous
	if ( GOOD( OW_10temp(&OWQ_F(owq), deny_85C, OWQ_SIMUL_TEST(owq), pn)) ) {
		return 0 ;
	}
	// second pass no simultaneous
	if ( GOOD( OW_10temp(&OWQ_F(owq), deny_85C, 0, pn)) ) {
		return 0 ;
	}
	// third pass, accept 85C
	return GB_to_Z_OR_E(OW_10temp(&OWQ_F(owq), allow_85C, 0, pn));
}

static ZERO_OR_ERROR FS_10temp_link(struct one_wire_query *owq)
{
	return FS_r_sibling_F(&OWQ_F(owq),"temperature",owq) ;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static ZERO_OR_ERROR FS_22temp(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	struct tempresolution * Resolution ;

	switch (pn->selected_filetype->data.i) { // bits
		case 9:
			Resolution = &Resolution9 ;
			break ;
		case 10:
			Resolution = &Resolution10 ;
			break ;
		case 11:
			Resolution = &Resolution11 ;
			break ;
		case 12:
			Resolution = &Resolution12 ;
			break ;
		default:
			return -ENODEV ;
	}

	// triple try temperatures
	// first pass include simultaneous
	if ( GOOD( OW_22temp(&OWQ_F(owq), deny_85C, OWQ_SIMUL_TEST(owq), Resolution, pn) ) ) {
		return 0 ;
	}
	// second pass no simultaneous
	if ( GOOD( OW_22temp(&OWQ_F(owq), deny_85C, 0, Resolution, pn) ) ) {
		return 0 ;
	}
	// third pass, accept 85C
	return GB_to_Z_OR_E(OW_22temp(&OWQ_F(owq), allow_85C, 0, Resolution, pn));
}

/* For MAX31826 */
static ZERO_OR_ERROR FS_MAXtemp(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;

	switch( VISIBLE_3B( pn ) ) {
		case 0x80 :
			break ;
		case -1:
			LEVEL_DEBUG( "Cannot distiguish between the DS1825 and MAX31826 for "SNformat, SNvar(pn->sn) ) ;
		// fall through
		default:
			return FS_22temp( owq ) ;
	}
	
	// triple try temperatures
	// first pass include simultaneous
	if ( GOOD( OW_MAXtemp(&OWQ_F(owq), deny_85C, OWQ_SIMUL_TEST(owq), &ResolutionMAX, pn) ) ) {
		return 0 ;
	}
	// second pass no simultaneous
	if ( GOOD( OW_MAXtemp(&OWQ_F(owq), deny_85C, 0, &ResolutionMAX, pn) ) ) {
		return 0 ;
	}
	// third pass, accept 85C
	return GB_to_Z_OR_E(OW_MAXtemp(&OWQ_F(owq), allow_85C, 0, &ResolutionMAX, pn));
}

// use sibling function for fasttemp to keep cache value consistent
static ZERO_OR_ERROR FS_fasttemp(struct one_wire_query *owq)
{
	_FLOAT temperature = 0.;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F( &temperature, "temperature9", owq ) ;

	OWQ_F(owq) = temperature ;
	return z_or_e ;
}

// use sibling function for temperature to keep cache value consistent
static ZERO_OR_ERROR FS_slowtemp(struct one_wire_query *owq)
{
	_FLOAT temperature = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F( &temperature, "temperature12", owq ) ;

	OWQ_F(owq) = temperature ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_power(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD(OW_power(&data, PN(owq))) ;
	OWQ_Y(owq) = (data != 0x00);
	return 0;
}


/* 28EA00 switch */
static ZERO_OR_ERROR FS_w_pio(struct one_wire_query *owq)
{
	BYTE data = BYTE_INVERSE(OWQ_U(owq) & 0x03);	/* reverse bits, set unused to 1s */
	//printf("Write pio raw=%X, stored=%X\n",OWQ_U(owq),data) ;
	FS_del_sibling( "piostate", owq ) ;
	return GB_to_Z_OR_E(OW_w_pio(data, PN(owq)));
}

static ZERO_OR_ERROR FS_sense(struct one_wire_query *owq)
{
	UINT piostate = 0;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &piostate, "piostate", owq ) ;

	// bits 0->0 and 2->1
	OWQ_U(owq) = ( (piostate & 0x01) | ((piostate & 0x04)>>1) ) & 0x03  ;
	return z_or_e;
}

static ZERO_OR_ERROR FS_r_pio(struct one_wire_query *owq)
{
	UINT piostate = 0;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &piostate, "piostate", owq ) ;

	// bits 0->0 and 2->1
	OWQ_U(owq) = BYTE_INVERSE( (piostate & 0x01) | ((piostate & 0x04)>>1) ) & 0x03  ;
	return z_or_e;
}

static ZERO_OR_ERROR FS_r_piostate(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_read_piostate(&(OWQ_U(owq)), PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_r_latch(struct one_wire_query *owq)
{
	UINT piostate = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &piostate, "piostate", owq ) ;

	// bits 1->0 and 3->1
	OWQ_U(owq) = BYTE_INVERSE( ((piostate & 0x02)>>1) | ((piostate & 0x08)>>2) ) & 0x03  ;
	return z_or_e;
}

static ZERO_OR_ERROR FS_r_templimit(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_r_templimit(&OWQ_F(owq), PN(owq)->selected_filetype->data.i, PN(owq)) );
}

/* DS1825 hardware programmable address */
static ZERO_OR_ERROR FS_r_ad(struct one_wire_query *owq)
{
	BYTE data[9];
	RETURN_ERROR_IF_BAD(OW_r_scratchpad(data, PN(owq))) ;
	OWQ_U(owq) = data[4] & 0x0F;
	return 0;
}

static ZERO_OR_ERROR FS_r_bit7(struct one_wire_query *owq)
{
	BYTE data[9];
	RETURN_ERROR_IF_BAD(OW_r_scratchpad(data, PN(owq))) ;
	OWQ_U(owq) = (data[4] & 0x80) ;
	return 0;
}

static ZERO_OR_ERROR FS_w_templimit(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_templimit(OWQ_F(owq), PN(owq)->selected_filetype->data.i, PN(owq)));
}

static ZERO_OR_ERROR FS_r_die(struct one_wire_query *owq)
{
	char *d;
	switch (OW_die(PN(owq))) {
	case eB6:
		d = "B6";
		break;
	case eB7:
		d = "B7";
		break;
	case eC2:
		d = "C2";
		break;
	case eC3:
		d = "C3";
		break;
	default:
		return -EINVAL;
	}
	return OWQ_format_output_offset_and_size(d, 2, owq);
}

static ZERO_OR_ERROR FS_r_scratchpad(struct one_wire_query *owq)
{
	BYTE s[9] ;
	
	if ( BAD(OW_r_scratchpad( s, PN(owq) ) ) ) {
		return -EINVAL ;
	}

	return OWQ_format_output_offset_and_size(s, 9, owq);
}

static ZERO_OR_ERROR FS_r_trim(struct one_wire_query *owq)
{
	BYTE t[2];
	RETURN_ERROR_IF_BAD(OW_r_trim(t, PN(owq))) ;
	OWQ_U(owq) = (t[1] << 8) | t[0];
	return 0;
}

static ZERO_OR_ERROR FS_w_trim(struct one_wire_query *owq)
{
	UINT trim = OWQ_U(owq);
	BYTE t[2] = { LOW_HIGH_ADDRESS(trim), };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		return GB_to_Z_OR_E(OW_w_trim(t, PN(owq)));
	default:
		return -EINVAL;
	}
}

/* Are the trim values valid-looking? */
static ZERO_OR_ERROR FS_r_trimvalid(struct one_wire_query *owq)
{
	BYTE trim[2];
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		RETURN_ERROR_IF_BAD(OW_r_trim(trim, PN(owq))) ;
		OWQ_Y(owq) = (((trim[0] & 0x07) == 0x05)
					  || ((trim[0] & 0x07) == 0x03))
			&& (trim[1] == 0xBB);
		break;
	default:
		OWQ_Y(owq) = 1;			/* Assume true */
	}
	return 0;
}

/* Put in a blank trim value if non-zero */
static ZERO_OR_ERROR FS_r_blanket(struct one_wire_query *owq)
{
	BYTE trim[2];
	BYTE blanket[] = { _DEFAULT_BLANKET_TRIM_1, _DEFAULT_BLANKET_TRIM_2 };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		RETURN_ERROR_IF_BAD(OW_r_trim(trim, PN(owq))) ;
		OWQ_Y(owq) = (memcmp(trim, blanket, 2) == 0);
		return 0;
	default:
		return -EINVAL;
	}
}

/* Put in a blank trim value if non-zero */
static ZERO_OR_ERROR FS_w_blanket(struct one_wire_query *owq)
{
	BYTE blanket[] = { _DEFAULT_BLANKET_TRIM_1, _DEFAULT_BLANKET_TRIM_2 };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		if (OWQ_Y(owq)) {
			RETURN_ERROR_IF_BAD(OW_w_trim(blanket, PN(owq)) ) ;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E( OW_r_mem( (BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq),PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E( OW_w_mem( (BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq),PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	struct parsedname * pn = PN(owq) ;

	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E( OW_r_mem( (BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq)+pn->extension*pagesize,pn) ) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	struct parsedname * pn = PN(owq) ;

	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E( OW_w_mem( (BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq)+pn->extension*pagesize,pn) ) ;
}

/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static GOOD_OR_BAD OW_10temp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	UINT delay = 1100;			// hard wired
	BYTE pow;
	struct transaction_log tunpowered[] = {
		TRXN_START,
		TRXN_POWER( convert, delay),
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_END,
	};

	if (OW_power(&pow, pn)) {
		pow = 0x00;				/* assume unpowered if cannot tell */
	}

	/* Select particular device and start conversion */
	if (!pow) {					// unpowered, deliver power, no communication allowed
		RETURN_BAD_IF_BAD(BUS_transaction(tunpowered, pn)) ;
	} else if ( !simul_good ) {	// powered
		// Simultaneous not valid, so do a conversion
		GOOD_OR_BAD ret;
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		RETURN_BAD_IF_BAD(ret) ;
	} else {
		// simultaneous -- delay if needed
		RETURN_BAD_IF_BAD( FS_Test_Simultaneous( simul_temp, delay, pn)) ;
	}

	RETURN_BAD_IF_BAD(OW_r_scratchpad(data, pn)) ;

	// Correction thanks to Nathan D. Holmes
	//temp[0] = (_FLOAT) ((int16_t)(data[1]<<8|data[0])) * .5 ; // Main conversion
	// Further correction, using "truncation" thanks to Wim Heirman
	//temp[0] = (_FLOAT) ((int16_t)(data[1]<<8|data[0])>>1); // Main conversion
	temp[0] = (_FLOAT) ((UT_int16(data)) >> 1);	// Main conversion -- now with helper function
	if (data[7]) {				// only if COUNT_PER_C non-zero (supposed to be!)
	//        temp[0] += (_FLOAT)(data[7]-data[6]) / (_FLOAT)data[7] - .25 ; // additional precision
		temp[0] += .75 - (_FLOAT) data[6] / (_FLOAT) data[7];	// additional precision
	}

	if ( accept_85C==allow_85C || data[0] != 0x50 || data[1] != 0x05 ) {
		return gbGOOD;
	}
	return gbBAD ;
}

static GOOD_OR_BAD OW_power(BYTE * data, const struct parsedname *pn)
{
	if (IsUncachedDir(pn)
		|| BAD( Cache_Get_SlaveSpecific(data, sizeof(BYTE), SlaveSpecificTag(POW), pn)) ) {
		BYTE b4[] = { _1W_READ_POWERMODE, };
		struct transaction_log tpower[] = {
			TRXN_START,
			TRXN_WRITE1(b4),
			TRXN_READ1(data),
			TRXN_END,
		};
	
		RETURN_BAD_IF_BAD(BUS_transaction(tpower, pn)) ;
		//LEVEL_DEBUG("TEST cannot read power");
		Cache_Add_SlaveSpecific(data, sizeof(BYTE), SlaveSpecificTag(POW), pn);
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_22temp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, struct tempresolution * Resolution, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	BYTE pow;
	UINT delay = Resolution->delay;
	UINT longdelay = delay * 1.5 ; // failsafe
	int stored_resolution ;
	int must_convert = 0 ;

	struct transaction_log tunpowered[] = {
		TRXN_START,
		TRXN_POWER(convert, delay),
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_END,
	};
	// failsafe
	struct transaction_log tunpowered_long[] = {
		TRXN_START,
		TRXN_POWER(convert, longdelay),
		TRXN_END,
	};

	//LEVEL_DEBUG("TEST Getting temperature");
	/* Resolution */
	if ( BAD( Cache_Get_SlaveSpecific(&stored_resolution, sizeof(stored_resolution), SlaveSpecificTag(RES), pn))
		|| stored_resolution != Resolution->bits) {
		BYTE resolution_register = Resolution->config;
		/* Get existing settings */
		RETURN_BAD_IF_BAD(OW_r_scratchpad(data, pn)) ;
		/* Put in new settings (if different) */
		if ((data[4] | 0x1F) != resolution_register) {	// ignore lower 5 bits
			//LEVEL_DEBUG("TEST resolution changed");
			must_convert = 1 ; // resolution has changed
			data[4] = (resolution_register & 0x60) | 0x1F ;
			/* only store in scratchpad, not EEPROM */
			RETURN_BAD_IF_BAD(OW_w_scratchpad(&data[2], pn)) ;
			Cache_Add_SlaveSpecific(&(Resolution->bits), sizeof(int), SlaveSpecificTag(RES), pn);
		}
	}

	/* Conversion */
	// first time
	/* powered? */
	if (OW_power(&pow, pn)) {
		//LEVEL_DEBUG("TEST unpowered");
		pow = 0x00;				/* assume unpowered if cannot tell */
	}

	if ( accept_85C == allow_85C ) {
		// must be desperate
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec", longdelay);
		// If not powered, no Simultaneous for this chip
		RETURN_BAD_IF_BAD(BUS_transaction(tunpowered_long, pn)) ;	
	} else if (!pow) {					// unpowered, deliver power, no communication allowed
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec", delay);
		// If not powered, no Simultaneous for this chip
		RETURN_BAD_IF_BAD(BUS_transaction(tunpowered, pn)) ;
	} else if ( must_convert || !simul_good ) {
		// No Simultaneous active, so need to "convert"
		// powered, so release bus immediately after issuing convert
		GOOD_OR_BAD ret;
		LEVEL_DEBUG("Powered temperature conversion");
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		RETURN_BAD_IF_BAD(ret)
	} else {
		// valid simultaneous, just delay if needed
		RETURN_BAD_IF_BAD( FS_Test_Simultaneous( simul_temp, delay, pn)) ;
	}

	RETURN_BAD_IF_BAD(OW_r_scratchpad(data, pn)) ;

	temp[0] = OW_masked_temperature( data, Resolution->mask) ;

	if ( accept_85C==allow_85C || data[0] != 0x50 || data[1] != 0x05 ) {
		return gbGOOD;
	}
	return gbBAD ;
}

static GOOD_OR_BAD OW_MAXtemp(_FLOAT * temp, enum temperature_problem_flag accept_85C, int simul_good, struct tempresolution * Resolution, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	BYTE pow;
	UINT delay = Resolution->delay;
	UINT longdelay = delay * 1.5 ; // failsafe
	int must_convert = 0 ;

	struct transaction_log tunpowered[] = {
		TRXN_START,
		TRXN_POWER(convert, delay),
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_END,
	};
	// failsafe
	struct transaction_log tunpowered_long[] = {
		TRXN_START,
		TRXN_POWER(convert, longdelay),
		TRXN_END,
	};

	/* Conversion */
	// first time
	/* powered? */
	if (OW_power(&pow, pn)) {
		//LEVEL_DEBUG("TEST unpowered");
		pow = 0x00;				/* assume unpowered if cannot tell */
	}

	if ( accept_85C == allow_85C ) {
		// must be desperate
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec", longdelay);
		// If not powered, no Simultaneous for this chip
		RETURN_BAD_IF_BAD(BUS_transaction(tunpowered_long, pn)) ;	
	} else if (!pow) {					// unpowered, deliver power, no communication allowed
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec", delay);
		// If not powered, no Simultaneous for this chip
		RETURN_BAD_IF_BAD(BUS_transaction(tunpowered, pn)) ;
	} else if ( must_convert || !simul_good ) {
		// No Simultaneous active, so need to "convert"
		// powered, so release bus immediately after issuing convert
		GOOD_OR_BAD ret;
		LEVEL_DEBUG("Powered temperature conversion");
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		RETURN_BAD_IF_BAD(ret)
	} else {
		// valid simultaneous, just delay if needed
		RETURN_BAD_IF_BAD( FS_Test_Simultaneous( simul_temp, delay, pn)) ;
	}

	RETURN_BAD_IF_BAD(OW_r_scratchpad(data, pn)) ;

	temp[0] = OW_masked_temperature( data, Resolution->mask) ;

	if ( accept_85C==allow_85C || data[0] != 0x50 || data[1] != 0x05 ) {
		return gbGOOD;
	}
	return gbBAD ;
}

/* Limits Tindex=0 high 1=low */
static GOOD_OR_BAD OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { _1W_RECALL_EEPROM, };
	struct transaction_log trecall[] = {
		TRXN_START,
		TRXN_WRITE1(recall),
		TRXN_DELAY(10),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(trecall, pn)) ;

	RETURN_BAD_IF_BAD(OW_r_scratchpad(data, pn)) ;
	T[0] = (_FLOAT) ((int8_t) data[2 + Tindex]);
	return gbGOOD;
}

/* Limits Tindex=0 high 1=low */
static GOOD_OR_BAD OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn)
{
	BYTE data[9];

	if (OW_r_scratchpad(data, pn)) {
		return gbBAD;
	}
	data[2 + Tindex] = (int8_t) T;
	return OW_w_store_scratchpad(&data[2], pn);
}

/* read 9 bytes, includes CRC8 which is checked */
static GOOD_OR_BAD OW_r_scratchpad(BYTE * data, const struct parsedname *pn)
{
	/* data is 9 bytes long */
	BYTE be[] = { _1W_READ_SCRATCHPAD, };
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE1(be),
		TRXN_READ(data, 9),
		TRXN_CRC8(data, 9),
		TRXN_END,
	};
	return BUS_transaction(tread, pn);
}

/* write 3 bytes (byte2,3,4 of register) */
/* Write the scratchpad but not back to static */
static GOOD_OR_BAD OW_w_scratchpad(const BYTE * data, const struct parsedname *pn)
{
	/* data is 3 bytes long */
	BYTE d[4] = { _1W_WRITE_SCRATCHPAD, data[0], data[1], data[2], };

	/* different processing for DS18S20 and others */
	int scratch_length = (pn->sn[0] == 0x10) ? 2 : 3 ;

	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(d, scratch_length+1),
		TRXN_END,
	};

	return BUS_transaction(twrite, pn);
}

/* write 3 bytes (byte2,3,4 of register) */
/* write to scratchpad and store in static */
static GOOD_OR_BAD OW_w_store_scratchpad(const BYTE * data, const struct parsedname *pn)
{
	/* data is 3 bytes long */
	BYTE d[4] = { _1W_WRITE_SCRATCHPAD, data[0], data[1], data[2], };
	BYTE pow[] = { _1W_COPY_SCRATCHPAD, };

	/* different processing for DS18S20 and others */
	int scratch_length = (pn->sn[0] == 0x10) ? 2 : 3 ;

	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(d, scratch_length+1),
		TRXN_START,
		TRXN_POWER(pow, 10),
		TRXN_END,
	};

	return BUS_transaction(twrite, pn);
}

/* Trim values -- undocumented except in AN247.pdf */
static GOOD_OR_BAD OW_r_trim(BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { _1W_READ_TRIM_1, };
	BYTE cmd1[] = { _1W_READ_TRIM_2, };
	struct transaction_log t0[] = {
		TRXN_START,
		TRXN_WRITE1(cmd0),
		TRXN_READ1(&trim[0]),
		TRXN_END,
	};
	struct transaction_log t1[] = {
		TRXN_START,
		TRXN_WRITE1(cmd1),
		TRXN_READ1(&trim[1]),
		TRXN_END,
	};
	
	RETURN_BAD_IF_BAD(BUS_transaction(t0, pn)) ;
	return BUS_transaction(t1, pn);
}

static GOOD_OR_BAD OW_w_trim(const BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { _1W_WRITE_TRIM_1, trim[0], };
	BYTE cmd1[] = { _1W_WRITE_TRIM_2, trim[1], };
	BYTE cmd2[] = { _1W_ACTIVATE_TRIM_1, };
	BYTE cmd3[] = { _1W_ACTIVATE_TRIM_2, };
	struct transaction_log t0[] = {
		TRXN_START,
		TRXN_WRITE2(cmd0),
		TRXN_END,
	};
	struct transaction_log t1[] = {
		TRXN_START,
		TRXN_WRITE2(cmd1),
		TRXN_END,
	};
	struct transaction_log t2[] = {
		TRXN_START,
		TRXN_WRITE1(cmd2),
		TRXN_END,
	};
	struct transaction_log t3[] = {
		TRXN_START,
		TRXN_WRITE1(cmd3),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD( BUS_transaction(t0, pn) );
	RETURN_BAD_IF_BAD( BUS_transaction(t1, pn) );
	RETURN_BAD_IF_BAD( BUS_transaction(t2, pn) );
	RETURN_BAD_IF_BAD( BUS_transaction(t3, pn) );
	return gbGOOD;
}

/* This is the CMOS die for the circuit, not program death */
static enum eDie OW_die(const struct parsedname *pn)
{
	BYTE die[6] = { pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1],
	};
	// data gives index into die matrix
	if (memcmp(die, DIE[pn->selected_filetype->data.i].C2, 6) > 0) {
		return eC2;
	}
	if (memcmp(die, DIE[pn->selected_filetype->data.i].B7, 6) > 0) {
		return eB7;
	}
	return eB6;
}

/* Powered temperature measurements -- need to poll line since it is held low during measurement */
/* We check every 10 msec (arbitrary) up to 1.25 seconds */
GOOD_OR_BAD FS_poll_convert(const struct parsedname *pn)
{
	int i;
	BYTE p[1];
	struct transaction_log t[] = {
		{NULL, NULL, 10, trxn_delay,},
		TRXN_READ1(p),
		TRXN_END,
	};

	// the first test is faster for just DS2438 (10 msec)
	// subsequent polling is slower since the DS18x20 is a slower converter
	for (i = 0; i < 22; ++i) {
		//LEVEL_DEBUG("TEST polling %d",i);
		if ( BAD( BUS_transaction_nolock(t, pn) )) {
			LEVEL_DEBUG("BUS_transaction failed");
			break;
		}
		if (p[0] != 0) {
			LEVEL_DEBUG("BUS_transaction done after %dms", (i + 1) * 10);
			return gbGOOD;
		}
		t[0].size = 50;			// 50 msec for rest of delays
	}
	LEVEL_DEBUG("Temperature measurement failed");
	return gbBAD;
}

/* read PIO pins for the DS28EA00 */
static GOOD_OR_BAD OW_read_piostate(UINT * piostate, const struct parsedname *pn)
{
	BYTE data[1];
	BYTE cmd[] = { _1W_PIO_ACCESS_READ, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(cmd),
		TRXN_READ1(data),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	/* compare lower and upper nibble to be complements */
	// High nibble the complement of low nibble?
	// Fix thanks to josef_heiler
	if ((data[0] & 0x0F) != ((~data[0] >> 4) & 0x0F)) {
		return gbBAD;
	}

	piostate[0] = data[0] & 0x0F ;

	return gbGOOD;
}
/* Write to PIO -- both channels. Already inverted and other fields set to 1 */
static GOOD_OR_BAD OW_w_pio(BYTE pio, const struct parsedname *pn)
{
	BYTE cmd[] = { _1W_PIO_ACCESS_WRITE, pio, BYTE_INVERSE(pio) };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(cmd),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}

static _FLOAT OW_masked_temperature( BYTE * data, BYTE mask )
{
	// Torsten Godau <tg@solarlabs.de> found a problem with 9-bit resolution
	return (_FLOAT) ((int16_t) ((data[1] << 8) | (data[0] & mask))) * .0625 ;
}

static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[] = { _1W_READ_MEMORY, offset & 0x7F, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_READ(data,size),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}

// write into eeprom
// need to split into pages
static GOOD_OR_BAD OW_w_mem( BYTE * data, size_t size, off_t offset, struct parsedname * pn )
{
	int pagesize = 8 ;
	BYTE * dataloc = data ;
	size_t left_to_write = size ;
	off_t current_off = offset ;
	off_t page_boundary_next = offset + pagesize - (offset % pagesize ) ;
	
	// loop through pages
	while ( left_to_write > 0 ) {
		off_t this_write = left_to_write ;
		
		// trim length of write to fit in page
		if ( current_off + this_write > page_boundary_next ) {
			this_write = page_boundary_next - current_off ;
		}

		// write this page
		RETURN_BAD_IF_BAD( OW_w_page( dataloc, this_write, current_off, pn ) ) ;
		
		// update pointers and counters
		dataloc += this_write ;
		left_to_write -= this_write ;
		current_off += this_write ;
		page_boundary_next += pagesize ;
	}
	
	return gbGOOD ;
}

// write into a "page"
// we assume that the "size" won't cross page boundary
static GOOD_OR_BAD OW_w_page( BYTE * data, size_t size, off_t offset, struct parsedname * pn )
{
	size_t pagesize = 8 ;
	off_t page_offset = offset - (offset % pagesize ) ;

	BYTE p_write[2+pagesize+1] ;
	BYTE p_read[2+pagesize+1] ;
	BYTE p_copy[2] ;

	struct transaction_log t_write[] = {
		TRXN_START,
		TRXN_WRITE( p_write, 2+pagesize ),
		TRXN_READ1( &p_write[2+pagesize] ),
		TRXN_CRC8( p_write, 2+pagesize+1 ),
		TRXN_END,
	} ;
	struct transaction_log t_read[] = {
		TRXN_START,
		TRXN_WRITE2( p_read ),
		TRXN_READ( &p_read[2], pagesize+1 ),
		TRXN_CRC8( p_read, 2+pagesize+1 ),
		TRXN_COMPARE( &p_write[2+pagesize], &p_read[2+pagesize], pagesize ) ,
		TRXN_END,
	} ;
	struct transaction_log t_copy[] = {
		TRXN_START,
		TRXN_WRITE1( p_copy ),
		TRXN_POWER( &p_copy[1], 25 ), // 25 msec
		TRXN_END,
	} ;

	if ( size == 0 ) {
		return gbGOOD ;
	}

	// is this a partial page (need to read the full page first to fill the buffer)
	if ( size != pagesize ) {
		RETURN_BAD_IF_BAD( OW_r_mem( &p_write[2], pagesize, page_offset, pn ) ) ;
	}

	// Copy data into loaded buffer
	memcpy( &p_write[2 + (offset - page_offset)], data, size ) ;

	// write to scratchpad 2
	p_write[0] = _1W_WRITE_SCRATCHPAD2 ;
	p_write[1] = page_offset ;
	RETURN_BAD_IF_BAD( BUS_transaction( t_write, pn ) ) ;

	// read from scratchpad 2 and compare
	p_read[0] = _1W_READ_SCRATCHPAD2 ;
	p_read[1] = page_offset ;
	RETURN_BAD_IF_BAD( BUS_transaction( t_read, pn ) ) ;
	
	// copy scratchpad 2 to eeprom
	p_copy[0] = _1W_COPY_SCRATCHPAD2 ;
	p_copy[1] = _1W_COPY_SCRATCHPAD2_DO ;
	return BUS_transaction( t_copy, pn ) ;
}	 
