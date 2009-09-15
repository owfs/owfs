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
#include "ow_eeef.h"
#include <math.h>

/* ------- Prototypes ----------- */

/* Hobby boards UVI and colleagues */
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_type_number);
READ_FUNCTION(FS_localtype);
READ_FUNCTION(FS_r_temperature_offset);
WRITE_FUNCTION(FS_w_temperature_offset);
READ_FUNCTION(FS_r_UVI_offset);
WRITE_FUNCTION(FS_w_UVI_offset);
READ_FUNCTION(FS_r_in_case);
WRITE_FUNCTION(FS_w_in_case);
READ_FUNCTION(FS_UVI);
READ_FUNCTION(FS_UVI_valid);

/* ------- Structures ----------- */

struct filetype HobbyBoards_EE[] = {
    F_STANDARD_NO_TYPE,
    {"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"temperature_offset", PROPERTY_LENGTH_TEMPGAP, NON_AGGREGATE, ft_tempgap, fc_stable, FS_r_temperature_offset, FS_w_temperature_offset, NO_FILETYPE_DATA,},
    {"UVI/in_case", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_in_case, FS_w_in_case, NO_FILETYPE_DATA,},
    {"version", 5, NON_AGGREGATE, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/UVI", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_UVI, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/valid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_UVI_valid, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_UVI_offset, FS_w_UVI_offset, NO_FILETYPE_DATA,},
};

DeviceEntry(EE, HobbyBoards_EE);

struct filetype HobbyBoards_EF[] = {
    F_STANDARD_NO_TYPE,
    {"UVI/in_case", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_in_case, FS_w_in_case, NO_FILETYPE_DATA,},
    {"version", 5, NON_AGGREGATE, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/UVI", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_UVI, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/valid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_UVI_valid, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
    {"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_UVI_offset, FS_w_UVI_offset, NO_FILETYPE_DATA,},
};

DeviceEntry(EE, HobbyBoards_EF);


#define _EEEF_READ_VERSION 0x11
#define _EEEF_READ_TYPE 0x12
#define _EEEF_READ_TEMPERATURE 0x21
#define _EEEF_SET_TEMPERATURE_OFFSET 0x22
#define _EEEF_READ_TEMPERATURE_OFFSET 0x23
#define _EEEF_READ_UVI 0x24
#define _EEEF_SET_UVI_OFFSET 0x25
#define _EEEF_READ_UVI_OFFSET 0x26
#define _EEEF_SET_IN_CASE 0x27
#define _EEEF_READ_IN_CASE 0x28

/* ------- Functions ------------ */

/* prototypes */
static int OW_version(BYTE * major, BYTE * minor, struct parsedname * pn) ;
static int OW_type(BYTE * type_number, struct parsedname * pn) ;

static int OW_UVI(_FLOAT * UVI, struct parsedname * pn) ;
static int OW_r_UVI_offset(_FLOAT * UVI, struct parsedname * pn) ;

static int OW_temperature(_FLOAT * T, struct parsedname * pn) ;
static int OW_r_temperature_offset(_FLOAT * T, struct parsedname * pn) ;

static int OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn) ;
static int OW_write(BYTE command, BYTE byte, struct parsedname * pn);

// returns major/minor as 2 hex bytes (ascii)
static int FS_version(struct one_wire_query *owq)
{
    char v[6];
    BYTE major, minor ;

    if (OW_version(&major,&minor,PN(owq))) {
        return -EINVAL ;
    }

    UCLIBCLOCK;
    snprintf(v,6,"%.2X.%.2X",major,minor);
    UCLIBCUNLOCK;

    return Fowq_output_offset_and_size(v, 5, owq);
}

static int FS_type_number(struct one_wire_query *owq)
{
    BYTE type_number ;

    if (OW_type(&type_number,PN(owq))) {
        return -EINVAL ;
    }

    OWQ_U(owq) = type_number ;

    return 0;
}

static int FS_temperature(struct one_wire_query *owq)
{
    _FLOAT T ;

    if (OW_temperature(&T,PN(owq))) {
        return -EINVAL ;
    }

    OWQ_F(owq) = T ;

    return 0;
}

static int FS_UVI(struct one_wire_query *owq)
{
    _FLOAT UVI ;

    if (OW_UVI(&UVI,PN(owq))) {
        return -EINVAL ;
    }

    OWQ_F(owq) = UVI ;

    return 0;
}

static int FS_r_UVI_offset(struct one_wire_query *owq)
{
    _FLOAT UVI ;

    if (OW_r_UVI_offset(&UVI,PN(owq))) {
        return -EINVAL ;
    }

    OWQ_F(owq) = UVI ;

    return 0;
}

static int FS_r_temperature_offset(struct one_wire_query *owq)
{
    _FLOAT T ;

    if (OW_r_temperature_offset(&T,PN(owq))) {
        return -EINVAL ;
    }

    OWQ_F(owq) = T ;

    return 0;
}

static int FS_w_temperature_offset(struct one_wire_query *owq)
{
    signed char c ;

#ifdef HAVE_LRINT
    c = lrint(2.0*OWQ_F(owq));    // round off
#else
    c = 2.0*OWQ_F(owq);
#endif

    if (OW_write(_EEEF_SET_TEMPERATURE_OFFSET,(BYTE)c,PN(owq))) {
        return -EINVAL ;
    }

    return 0;
}

static int FS_w_UVI_offset(struct one_wire_query *owq)
{
    unsigned char c ;

#ifdef HAVE_LRINT
    c = lrint(10.0*OWQ_F(owq));    // round off
#else
    c = 10.0*OWQ_F(owq)+.49;
#endif

    if (OW_write(_EEEF_SET_UVI_OFFSET,(BYTE)c,PN(owq))) {
        return -EINVAL ;
    }

    return 0;
}

static int FS_localtype(struct one_wire_query *owq)
{
    UINT type_number ;
    if ( FS_r_sibling_U( &type_number, "type_number", owq ) ) {
        return -EINVAL ;
    }

    switch (type_number) {
        case 0x01:
            return Fowq_output_offset_and_size_z("Hobby_Boards_UVI", owq) ;
        default:
            return FS_type(owq);
    }
}

static int FS_UVI_valid(struct one_wire_query *owq)
{
    UINT type_number ;
    if ( FS_r_sibling_U( &type_number, "type_number", owq ) ) {
        return -EINVAL ;
    }

    switch (type_number) {
        case 0x01:
            OWQ_Y(owq) = 1 ;
            break ;
        default:
            OWQ_Y(owq) = 0 ;
            break ;
    }
    return 0 ;
}

static int FS_r_in_case(struct one_wire_query *owq)
{
    BYTE in_case ;
    if ( OW_read(_EEEF_READ_IN_CASE,&in_case,1,PN(owq))) {
        return -EINVAL ;
    }
    switch(in_case) {
        case 0x00:
            OWQ_Y(owq) = 0 ;
            break ;
        case 0xFF:
            OWQ_Y(owq) = 1 ;
            break ;
        default:
            return -EINVAL ;
    }
    return 0 ;
}

static int FS_w_in_case(struct one_wire_query *owq)
{
    BYTE in_case = OWQ_Y(owq) ? 0xFF : 0x00 ;
    if ( OW_write(_EEEF_SET_IN_CASE,in_case,PN(owq))) {
        return -EINVAL ;
    }
    return 0 ;
}

static int OW_version(BYTE * major, BYTE * minor, struct parsedname * pn)
{
    BYTE response[2] ;
    if (OW_read(_EEEF_READ_VERSION, response, 2, pn)) {
        return 1;
    }
    *minor = response[0] ;
    *major = response[1] ;
    return 0 ;
}

static int OW_type(BYTE * type_number, struct parsedname * pn)
{
    if (OW_read(_EEEF_READ_TYPE, type_number, 1, pn)) {
        return 1;
    }
    return 0 ;
}

static int OW_UVI(_FLOAT * UVI, struct parsedname * pn)
{
    BYTE u[1] ;
    if (OW_read(_EEEF_READ_UVI, u, 1, pn)) {
        return 1;
    }
    if (u[0]==0xFF) {
        return 1;
    }
    UVI[0] = 0.1 * ((_FLOAT) u[0]) ;
    return 0 ;
}

static int OW_r_UVI_offset(_FLOAT * UVI, struct parsedname * pn)
{
    BYTE u[1] ;
    if (OW_read(_EEEF_READ_UVI_OFFSET, u, 1, pn)) {
        return 1;
    }
    if (u[0]==0xFF) {
        return 1;
    }
    UVI[0] = 0.1 * ((_FLOAT) ((signed char) u[0])) ;
    return 0 ;
}

static int OW_r_temperature_offset(_FLOAT * T, struct parsedname * pn)
{
    BYTE t[1] ;
    if (OW_read(_EEEF_READ_TEMPERATURE_OFFSET, t, 1, pn)) {
        return 1;
    }
    T[0] = 0.5 * ((_FLOAT) ((signed char)t[0])) ;
    return 0 ;
}

static int OW_temperature(_FLOAT * temperature, struct parsedname * pn)
{
    BYTE t[2] ;
    if (OW_read(_EEEF_READ_TEMPERATURE, t, 2, pn)) {
        return 1;
    }
    temperature[0] = (_FLOAT) 0.5 * UT_int16(t) ;
    return 0 ;
}

static int OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn)
{
    BYTE c[] = { command,} ;
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE1(c),
        TRXN_READ(bytes,size),
        TRXN_END,
    };
    return BUS_transaction(t, pn) ;
}

static int OW_write(BYTE command, BYTE byte, struct parsedname * pn)
{
    BYTE c[] = { command, byte, } ;
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE2(c),
        TRXN_END,
    };
    return  BUS_transaction(t, pn) ;
}

