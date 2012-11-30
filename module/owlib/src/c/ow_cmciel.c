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

/* ------- Structures ----------- */
#define mTS017_type      0x4000  // bit 14
#define mTS017_object    0x0000  // bit 14
#define mTS017_ambient   0x4000  // bit 14
#define mTS017_plex      0x8000  // bit 15
#define mTS017_single    0x0000  // bit 15
#define mTS017_multi     0x8000  // bit 15

/* Infrared temperature sensor */
static struct filetype mTS017[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data17, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"object", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_temperature, NO_WRITE_FUNCTION, VISIBLE, { u:mTS017_object}, },
	{"ambient", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_temperature, NO_WRITE_FUNCTION, VISIBLE, { u:mTS017_ambient}, },
};

DeviceEntry(A1, mTS017, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* AC current meter */
static struct filetype mCM001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(A2, mCM001, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Analog Imput meter */
static struct filetype mAM001[] = {
	F_STANDARD,
	{"reading", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_AM001_a, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volts", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_AM001_v, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(B2, mAM001, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_READ_SCRATCHPAD 0xBE
#define _1W_WRITE_CONFIG    0x4E
#define _1W_READ_CONFIG     0x4F

#define _CMCIEL_CONFIG_OBJECT     0x01
#define _CMCIEL_CONFIG_AMBIENT    0x02
#define _CMCIEL_CONFIG_MULTIPLEX  0x04
#define _CMCIEL_CONFIG_UNPROTECT  0x08
#define _CMCIEL_CONFIG_PARAMETERS 0x20
#define _CMCIEL_CONFIG_EMISSIVITY 0x40

#define _CMCIEL_ADDRESS_CONFIG                0x55
#define _CMCIEL_ADDRESS_EMISSIVITY_LOW        0xA1
#define _CMCIEL_ADDRESS_EMISSIVITY_HI         0xA2
#define _CMCIEL_ADDRESS_HAND_DETECT_THRESHOLD 0xA3
#define _CMCIEL_ADDRESS_HAND_DETECT_HOLD      0xA4
#define _CMCIEL_ADDRESS_AVG_NUMBER            0xA5
#define _CMCIEL_ADDRESS_AVG_OVERRIDE          0xA6

/* ------- Functions ------------ */

/*  */
static GOOD_OR_BAD OW_reading( BYTE * data, struct parsedname *pn);

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
	return OW_w_config( _CMCIEL_ADDRESS_CONFIG, _CMCIEL_CONFIG_UNPROTECT, pn ) ;
}

static GOOD_OR_BAD OW_set_read_mode(BYTE read_mode, struct parsedname *pn)
{
	// 0=object 1=ambient 4= multiplex
	RETURN_BAD_IF_BAD( OW_unprotect(pn) ) ;
	return OW_w_config( _CMCIEL_ADDRESS_CONFIG, read_mode, pn ) ;
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

