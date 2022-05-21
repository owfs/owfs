/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_cmciel.h"

/* ------- Prototypes ----------- */

/* CMCIEL sensors */
READ_FUNCTION(FS_r_data);
READ_FUNCTION(FS_r_data17);
READ_FUNCTION(FS_r_temperature);
READ_FUNCTION(FS_r_current);
READ_FUNCTION(FS_r_AM001_v);
READ_FUNCTION(FS_r_AM001_a);
READ_FUNCTION(FS_r_RPM);
READ_FUNCTION(FS_r_vibration);
WRITE_FUNCTION(FS_w_vib_mode);

/* ------- Structures ----------- */
#define mTS017_type      0x4000  // bit 14
#define mTS017_object    0x0000  // bit 14
#define mTS017_ambient   0x4000  // bit 14
#define mTS017_plex      0x8000  // bit 15
#define mTS017_single    0x0000  // bit 15
#define mTS017_multi     0x8000  // bit 15

enum e_mVM001 { e_mVM001_peak=1, e_mVM001_rms=2, e_mVM001_multi=4, } ;

/* Infrared temperature sensor */
static struct filetype mTS017[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data17, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"object", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_temperature, fc_link, FS_r_temperature, NO_WRITE_FUNCTION, VISIBLE, {.u=mTS017_object}, },
	{"ambient", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_temperature, fc_link, FS_r_temperature, NO_WRITE_FUNCTION, VISIBLE, {.u=mTS017_ambient}, },
};

DeviceEntry(A6, mTS017, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Vibration Sensor */
static struct filetype mVM001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"RMS", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_vibration, NO_WRITE_FUNCTION, VISIBLE, {.i=e_mVM001_rms, }, },
	{"peak", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_vibration, NO_WRITE_FUNCTION, VISIBLE, {.i=e_mVM001_peak, }, },
	{"configure", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"configure/read_mode", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_w_vib_mode, VISIBLE, NO_FILETYPE_DATA, },
	{"configure/peak_interval", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"configure/multiplex_interval", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(A1, mVM001, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* AC current meter */
static struct filetype mCM001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(A2, mCM001, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Analog Input meter */
static struct filetype mAM001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_AM001_a, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volts", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_AM001_v, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(B2, mAM001, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Rotation Sensor */
static struct filetype mRS001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"RPM", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_RPM, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(A0, mRS001, NO_GENERIC_READ, NO_GENERIC_WRITE);

// struct bitfield { "alias_link", number_of_bits, shift_left, }
static struct bitfield mDI001_switch  = { "reading", 4, 0, } ;
static struct bitfield mDI001_shorted = { "reading", 4, 4, } ;
static struct bitfield mDI001_open    = { "reading", 8, 0, } ;
static struct bitfield mDI001_switch_closed  = { "switch", 1, 0, } ;
static struct bitfield mDI001_loop_shorted   = { "shorted", 1, 0, } ;
static struct bitfield mDI001_loop_open      = { "open", 1, 0, } ;

/* Digital Input Sensor */
static struct aggregate mDI001_state = { 4, ag_numbers, ag_aggregate, }; // 4 inputs
static struct filetype mDI001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"switch", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, INVISIBLE, {.v= &mDI001_switch,}, },
	{"shorted", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, INVISIBLE, {.v= &mDI001_shorted,}, },
	{"open", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, INVISIBLE, {.v= &mDI001_open,}, },
	{"switch_closed", PROPERTY_LENGTH_BITFIELD, &mDI001_state, ft_bitfield, fc_volatile, FS_r_bit_array, NO_WRITE_FUNCTION, VISIBLE, {.v= &mDI001_switch_closed,}, },
	{"loop_shorted", PROPERTY_LENGTH_BITFIELD, &mDI001_state, ft_bitfield, fc_volatile, FS_r_bit_array, NO_WRITE_FUNCTION, VISIBLE, {.v= &mDI001_loop_shorted,}, },
	{"loop_open", PROPERTY_LENGTH_BITFIELD, &mDI001_state, ft_bitfield, fc_volatile, FS_r_bit_array, NO_WRITE_FUNCTION, VISIBLE, {.v= &mDI001_loop_open,}, },
};

DeviceEntry(A5, mDI001, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_READ_SCRATCHPAD 0xBE
#define _1W_WRITE_CONFIG    0x4E
#define _1W_READ_CONFIG     0x4F

#define _mTS017_CONFIG_OBJECT     0x01
#define _mTS017_CONFIG_AMBIENT    0x02
#define _mTS017_CONFIG_MULTIPLEX  0x04
#define _mTS017_CONFIG_UNPROTECT  0x08
#define _mTS017_CONFIG_PARAMETERS 0x20
#define _mTS017_CONFIG_EMISSIVITY 0x40

#define _mTS017_ADDRESS_CONFIG                0x55
#define _mTS017_ADDRESS_EMISSIVITY_LOW        0xA1
#define _mTS017_ADDRESS_EMISSIVITY_HI         0xA2
#define _mTS017_ADDRESS_HAND_DETECT_THRESHOLD 0xA3
#define _mTS017_ADDRESS_HAND_DETECT_HOLD      0xA4
#define _mTS017_ADDRESS_AVG_NUMBER            0xA5
#define _mTS017_ADDRESS_AVG_OVERRIDE          0xA6

#define _mVM001_CONFIG_OBJECT     0x01
#define _mVM001_CONFIG_AMBIENT    0x02
#define _mVM001_CONFIG_MULTIPLEX  0x04
#define _mVM001_CONFIG_UNPROTECT  0x10

#define _mVM001_ADDRESS_CONFIG                0x55
#define _mVM001_ADDRESS_READ_MODE             0x80
#define _mVM001_ADDRESS_PEAK_INTERVAL         0x81
#define _mVM001_ADDRESS_MULTIPLEX_INTERVAL    0x82

#define _mDI001_ADDRESS_CONFIG                0x55
#define _mDI001_ADDRESS_READ_PARAMETER        0x01
#define _mDI001_CONFIG_UNPROTECT              0x08
#define _mDI001_CONFIG_PROTECT                0x10

#define _mDI001_CONFIG_MASK                   0xF000
#define _mDI001_CONFIG_TIME                   0x6000
#define _mDI001_CONFIG_STATUS                 0x8000
#define _mDI001_CONFIG_RAW1                   0x9000
#define _mDI001_CONFIG_RAW2                   0xA000
#define _mDI001_CONFIG_RAW3                   0xB000
#define _mDI001_CONFIG_RAW4                   0xC000
#define _mDI001_CONFIG_DELAY                  0xD000


/* Internal properties */
Make_SlaveSpecificTag(VIB, fc_stable);	// vibration mode

/* ------- Functions ------------ */

/*  */
static GOOD_OR_BAD OW_w_config(BYTE config0, BYTE config1, struct parsedname *pn);
static GOOD_OR_BAD OW_reading( BYTE * data, struct parsedname *pn);
static GOOD_OR_BAD OW_set_vib_mode(BYTE read_mode, struct parsedname *pn) ;
static GOOD_OR_BAD OW_unprotect(struct parsedname *pn) ;
#if 0
static GOOD_OR_BAD OW_set_read_mode(BYTE read_mode, struct parsedname *pn);
#endif

/* mTS017 */
static ZERO_OR_ERROR FS_r_temperature(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT reading ;
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	// Is this the correct type?
	if ( (reading & mTS017_type) == pn->selected_filetype->data.u ) {
		// Pare off top 2 bits and sign extend
		OWQ_F(owq) = ( ( (int16_t)(reading << 2) ) / 4 ) / 10. ;
		return 0 ;
	}
	
	// Is it a multiplex? Then try again
	if ( (reading & mTS017_plex) == mTS017_multi ) {
		// reread
		if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
			return -EINVAL ;
		}
		// Is this the correct type?
		if ( (reading & mTS017_type) == pn->selected_filetype->data.u ) {
			// Pare off top 2 bits and sign extend
			OWQ_F(owq) = ( ( (int16_t)(reading << 2) ) / 4 ) / 10. ;
			return 0 ;
		}
	} else {
		// switch mode if we knew how
	}
	
	return -EINVAL ;
}

/* mvM001 Vibration */
static ZERO_OR_ERROR FS_r_vibration(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT reading ;
	BYTE vib_mode = pn->selected_filetype->data.i ;
	
	if ( FS_w_sibling_U( vib_mode, "configure/read_mode", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	OWQ_F(owq) = .01 * reading ;

	return -EINVAL ;
}

static ZERO_OR_ERROR FS_w_vib_mode(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT vib_request = OWQ_U(owq) ;
	BYTE vib_stored ;

	switch (vib_request) {
		case e_mVM001_peak:
		case e_mVM001_rms:
		case e_mVM001_multi:
			break ;
		default:
			return -EINVAL ;
	}
	
	if ( BAD( Cache_Get_SlaveSpecific(&vib_stored, sizeof(vib_stored), SlaveSpecificTag(VIB), pn)) ) {
		if ( vib_stored == vib_request ) {
			return 0 ;
		}
	}
	
	if ( BAD( OW_set_vib_mode( vib_request, pn ) ) ) {
		return -EINVAL ;
	}

	Cache_Add_SlaveSpecific(&vib_request, sizeof(vib_request), SlaveSpecificTag(VIB), pn);
	
	return 0 ;
}

//; Special version for the mTS017 that knows about multiplexing for the cache
static ZERO_OR_ERROR FS_r_data17(struct one_wire_query *owq)
{
	if ( FS_r_data(owq) < 0 ) {
		return -EINVAL ;
	}
	
	// Is it a multiplex? Then kill cache
	if ( (OWQ_U(owq) & mTS017_plex) == mTS017_multi ) {
		OWQ_Cache_Del(owq) ;
	}
	
	return 0 ;
}

static ZERO_OR_ERROR FS_r_data(struct one_wire_query *owq)
{
	BYTE data[2];
	
	if ( BAD( OW_reading( data, PN(owq) ) ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = data[1] * 256 + data[0] ;
	return 0 ;
}

// uses CRC8
static GOOD_OR_BAD OW_reading(BYTE * data, struct parsedname *pn)
{
	BYTE p[1] = { _1W_READ_SCRATCHPAD, };
	BYTE q[3];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_WRITE2(q),
		TRXN_READ1(&q[2]),
		TRXN_CRC8(q,3),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	memcpy(data, q, 2);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_config(BYTE config0, BYTE config1, struct parsedname *pn)
{
	BYTE p[4] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(p,4),
		TRXN_END,
	};

	p[0] = _1W_WRITE_CONFIG ;
	p[1] = config0 ;
	p[2] = config1 ;
	p[3] = CRC8( &p[1], 2 ) ;

	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_unprotect(struct parsedname *pn)
{
	return OW_w_config( _mTS017_ADDRESS_CONFIG, _mTS017_CONFIG_UNPROTECT, pn ) ;
}

#if 0
static GOOD_OR_BAD OW_set_read_mode(BYTE read_mode, struct parsedname *pn)
{
	// 1=object 2=ambient 4= multiplex
	RETURN_BAD_IF_BAD( OW_unprotect(pn) ) ;
	return OW_w_config( _mTS017_ADDRESS_CONFIG, read_mode, pn ) ;
}
#endif

static GOOD_OR_BAD OW_set_vib_mode(BYTE read_mode, struct parsedname *pn)
{
	// 1=peak 2=rms 4= multiplex
	RETURN_BAD_IF_BAD( OW_unprotect(pn) ) ;
	return OW_w_config( _mVM001_ADDRESS_READ_MODE, read_mode, pn ) ;
}

/* mCM001 AC current */
static ZERO_OR_ERROR FS_r_current(struct one_wire_query *owq)
{
	UINT reading ;
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	OWQ_F(owq) = reading / 100. ;
	
	return 0 ;
}

/* mAM001 Analog input */
static ZERO_OR_ERROR FS_r_AM001_a(struct one_wire_query *owq)
{
	UINT reading ;
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	OWQ_F(owq) = .02 * reading / 10000. ;
	
	return 0 ;
}

static ZERO_OR_ERROR FS_r_AM001_v(struct one_wire_query *owq)
{
	UINT reading ;
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	OWQ_F(owq) = 10. * reading / 10000. ;
	
	return 0 ;
}

/* mRS001 Rotation Sensor */
static ZERO_OR_ERROR FS_r_RPM(struct one_wire_query *owq)
{
	UINT reading ;
	
	if ( FS_r_sibling_U( &reading, "reading", owq ) < 0 ) {
		return -EINVAL ;
	}
	
	OWQ_I(owq) = (int16_t) reading ;
	
	return 0 ;
}

