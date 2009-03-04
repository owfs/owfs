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

/* Stats are a pseudo-device -- they are a file-system entry and handled as such,
     but have a different caching type to distiguish their handling */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_interface.h"
#include "ow_connection.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
READ_FUNCTION(FS_name);
READ_FUNCTION(FS_port);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_r_overdrive);
WRITE_FUNCTION(FS_w_overdrive);
READ_FUNCTION(FS_r_flextime);
WRITE_FUNCTION(FS_w_flextime);
READ_FUNCTION(FS_r_reverse);
WRITE_FUNCTION(FS_w_reverse);
READ_FUNCTION(FS_r_serialspeed);
WRITE_FUNCTION(FS_w_serialspeed);
READ_FUNCTION(FS_r_reconnect);
WRITE_FUNCTION(FS_w_reconnect);
READ_FUNCTION(FS_r_checksum);
WRITE_FUNCTION(FS_w_checksum);
READ_FUNCTION(FS_r_ds2404_compliance);
WRITE_FUNCTION(FS_w_ds2404_compliance);
READ_FUNCTION(FS_r_pulldownslewrate);
WRITE_FUNCTION(FS_w_pulldownslewrate);
READ_FUNCTION(FS_r_writeonelowtime);
WRITE_FUNCTION(FS_w_writeonelowtime);
READ_FUNCTION(FS_r_datasampleoffset);
WRITE_FUNCTION(FS_w_datasampleoffset);
READ_FUNCTION(FS_r_APU);
WRITE_FUNCTION(FS_w_APU);
READ_FUNCTION(FS_r_PPM);
WRITE_FUNCTION(FS_w_PPM);
READ_FUNCTION(FS_r_baud);
WRITE_FUNCTION(FS_w_baud);
//#define DEBUG_DS2490
#ifdef DEBUG_DS2490
READ_FUNCTION(FS_r_ds2490status);
#endif

/* Statistics reporting */
READ_FUNCTION(FS_stat_p);
READ_FUNCTION(FS_time);
READ_FUNCTION(FS_time_p);
READ_FUNCTION(FS_elapsed);

#if OW_USB
int DS9490_getstatus(BYTE * buffer, int readlen, const struct parsedname *pn);
#endif

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
struct filetype interface_settings[] = {
  {"name", 128, NULL, ft_vascii, fc_static, FS_name, NO_WRITE_FUNCTION, {v:NULL},},
	// variable length
  {"address", 512, NULL, ft_vascii, fc_static, FS_port, NO_WRITE_FUNCTION, {v:NULL},},
	// variable length
  {"baudrate", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, FS_r_baud, FS_w_baud, {v:NULL},},
  {"checksum", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_checksum, FS_w_checksum, {v:NULL},},
  {"datasampleoffset", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, FS_r_datasampleoffset, FS_w_datasampleoffset, {v:NULL},},
  {"ds2404_compliance", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_ds2404_compliance, FS_w_ds2404_compliance, {v:NULL},},
  {"overdrive", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_overdrive, FS_w_overdrive, {v:NULL},},
  {"reconnect", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_reconnect, FS_w_reconnect, {v:NULL},},
  {"flexible_timing", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_flextime, FS_w_flextime, {v:NULL},},
  {"reverse_polarity", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_reverse, FS_w_reverse, {v:NULL},},
  {"serial_speed", PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, FS_r_serialspeed, FS_w_serialspeed, {v:NULL},},
  {"pulldownslewrate", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, FS_r_pulldownslewrate, FS_w_pulldownslewrate, {v:NULL},},
#ifdef DEBUG_DS2490
  {"ds2490status", 128, NULL, ft_vascii, fc_static, FS_r_ds2490status, NO_WRITE_FUNCTION, {v:NULL},},
#endif
  {"version", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, FS_version, NO_WRITE_FUNCTION, {v:NULL},},
  {"writeonelowtime", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, FS_r_writeonelowtime, FS_w_writeonelowtime, {v:NULL},},
  {"ActivePullUp", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_APU, FS_w_APU, {v:NULL},},
  {"PulsePresenceMask", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_static, FS_r_PPM, FS_w_PPM, {v:NULL},},
};
struct device d_interface_settings = { "settings", "settings", ePN_interface,
	COUNT_OF_FILETYPES(interface_settings), interface_settings
};

struct filetype interface_statistics[] = {
  {"elapsed_time", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_elapsed, NO_WRITE_FUNCTION, {v:NULL},},
  {"bus_time", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_statistic, FS_time_p, NO_WRITE_FUNCTION, {i:0},},
  {"reconnects", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_reconnects},},
  {"reconnect_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_reconnect_errors},},
  {"locks", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_locks},},
  {"unlocks", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_unlocks},},
  {"errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_errors},},
  {"resets", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_resets},},
  {"program_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_program_errors},},
  {"pullup_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_pullup_errors},},
  {"reset_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_reset_errors},},
  {"shorts", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_short_errors},},
  {"read_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_read_errors},},
  {"write_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_write_errors},},
  {"open_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_open_errors},},
  {"close_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_close_errors},},
  {"detect_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_detect_errors},},
  {"search_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_search_errors},},
  {"status_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_status_errors},},
  {"select_errors", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_select_errors},},
  {"timeouts", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_timeouts},},
  {"overdrive", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"overdrive/attempts", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_try_overdrive},},
  {"overdrive/failures", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_failed_overdrive},},
  {"total_bus_time", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_statistic, FS_time, NO_WRITE_FUNCTION, {v:&total_bus_time},},
  {"timeouts", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, {i:e_bus_timeouts},},
};
struct device d_interface_statistics = { "statistics", "statistics", 0,
	COUNT_OF_FILETYPES(interface_statistics), interface_statistics
};


/* ------- Functions ------------ */


/* Just some tests to support change of extra delay */
static int FS_r_ds2404_compliance(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_Y(owq) = pn->selected_connection->ds2404_compliance;
	return 0;
}

static int FS_w_ds2404_compliance(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	pn->selected_connection->ds2404_compliance = OWQ_Y(owq);
	return 0;
}

/* Just some tests to support overdrive */
static int FS_r_reconnect(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_Y(owq) = (pn->selected_connection->reconnect_state == reconnect_error);
	return 0;
}

/* Just some tests to support overdrive */
static int FS_w_reconnect(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	pn->selected_connection->reconnect_state = OWQ_Y(owq) ? reconnect_error : reconnect_ok ;
	return 0;
}

/* Just some tests to support flextime */
static int FS_r_flextime(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    OWQ_Y(owq) = (pn->selected_connection->flex==bus_yes_flex);
    return 0;
}

static int FS_w_flextime(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    pn->selected_connection->flex = OWQ_Y(owq) ? bus_yes_flex : bus_no_flex ;
    ++pn->selected_connection->changed_bus_settings ;
    return 0;
}

/* DS2482 APU setting */
#define DS2482_REG_CFG_APU     0x01
static int FS_r_APU(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_i2c:
			OWQ_Y(owq) = ( (pn->selected_connection->connin.i2c.configreg & DS2482_REG_CFG_APU) != 0x00 ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static int FS_w_APU(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_i2c:
			if ( OWQ_Y(owq) ) {
				pn->selected_connection->connin.i2c.configreg |= DS2482_REG_CFG_APU ;
			} else {
				pn->selected_connection->connin.i2c.configreg &= ~DS2482_REG_CFG_APU ;
			}
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* DS2480B reverse_polarity setting */
static int FS_r_reverse(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_serial:
			OWQ_Y(owq) = ( pn->selected_connection->connin.serial.reverse_polarity ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static int FS_w_reverse(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_serial:
			pn->selected_connection->connin.serial.reverse_polarity = OWQ_Y(owq) ;
			++pn->selected_connection->changed_bus_settings ;
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* Serial baud rate */
static int FS_r_baud(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_serial:
		case bus_link:
		case bus_ha5:
		case bus_ha7e:
			OWQ_U(owq) = OW_BaudRate( pn->selected_connection->baud ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static int FS_w_baud(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_serial:
		case bus_link:
			pn->selected_connection->baud = OW_MakeBaud( (speed_t) OWQ_U(owq) ) ;
			++pn->selected_connection->changed_bus_settings ;
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* DS2482 PPM setting */
#define DS2482_REG_CFG_PPM     0x02
static int FS_r_PPM(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_i2c:
			OWQ_Y(owq) = ( (pn->selected_connection->connin.i2c.configreg & DS2482_REG_CFG_PPM) != 0x00 ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static int FS_w_PPM(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_i2c:
			if ( OWQ_Y(owq) ) {
				pn->selected_connection->connin.i2c.configreg |= DS2482_REG_CFG_PPM ;
			} else {
				pn->selected_connection->connin.i2c.configreg &= ~DS2482_REG_CFG_PPM ;
			}
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* Just some tests for ha5 checksum */
static int FS_r_checksum(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_ha5:
			OWQ_Y(owq) = pn->selected_connection->connin.ha5.checksum;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static int FS_w_checksum(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( pn->selected_connection->busmode ) {
		case bus_ha5:
			pn->selected_connection->connin.ha5.checksum = OWQ_Y(owq) ;
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* baud rate choice */
static int FS_r_serialspeed(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch (pn->selected_connection->busmode) {
		case bus_serial:
			OWQ_I(owq) = (pn->selected_connection->speed);
			break ;
		default:
			return -ENOTSUP ;
	}
	return 0;
}

static int FS_w_serialspeed(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch (pn->selected_connection->busmode) {
		case bus_serial:
			pn->selected_connection->speed = OWQ_I(owq) ;
			++pn->selected_connection->changed_bus_settings;
			break ;
		default:
			break ;
	}
	return 0;
}

/* Just some tests to support overdrive */
static int FS_r_overdrive(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_Y(owq) = (pn->selected_connection->speed == bus_speed_overdrive);
	return 0;
}

static int FS_w_overdrive(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	pn->selected_connection->speed = OWQ_Y(owq) ? bus_speed_overdrive : bus_speed_slow;
	++pn->selected_connection->changed_bus_settings;
	return 0;
}

#ifdef DEBUG_DS2490
static int FS_r_ds2490status(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	char res[256];
	char buffer[32 + 1];
	int ret;
	res[0] = '\0';
	if (pn->selected_connection->busmode == bus_usb) {
#if OW_USB
		ret = DS9490_getstatus(buffer, 0, PN(owq));
		if (ret < 0) {
			sprintf(res, "DS9490_getstatus failed: %d\n", ret);
		} else {
			sprintf(res,
					"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
					buffer[0], buffer[1], buffer[2], buffer[3],
					buffer[4], buffer[5], buffer[6], buffer[7],
					buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);
		}
		/*
		   uchar    EnableFlags;
		   uchar    OneWireSpeed;
		   uchar    StrongPullUpDuration;
		   uchar    ProgPulseDuration;
		   uchar    PullDownSlewRate;
		   uchar    Write1LowTime;
		   uchar    DSOW0RecoveryTime;
		   uchar    Reserved1;
		   uchar    StatusFlags;
		   uchar    CurrentCommCmd1;
		   uchar    CurrentCommCmd2;
		   uchar    CommBufferStatus;  // Buffer for COMM commands
		   uchar    WriteBufferStatus; // Buffer we write to
		   uchar    ReadBufferStatus;  // Buffer we read from
		 */
#endif
	}
	Fowq_output_offset_and_size_z(res, owq);
	return 0;
}
#endif

/*
 * Value is between 0 and 7.
 * Default value is 3.
 *
 * PARMSET_Slew15Vus   0x0
 * PARMSET_Slew2p20Vus 0x1
 * PARMSET_Slew1p65Vus 0x2
 * PARMSET_Slew1p37Vus 0x3 (default with altUSB)
 * PARMSET_Slew1p10Vus 0x4
 * PARMSET_Slew0p83Vus 0x5 (default without altUSB)
 * PARMSET_Slew0p70Vus 0x6
 * PARMSET_Slew0p55Vus 0x7
 */
static int FS_r_pulldownslewrate(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb) {
		OWQ_U(owq) = 3;
	} else {
		OWQ_U(owq) = pn->selected_connection->connin.usb.pulldownslewrate;
	}
	return 0;
}

static int FS_w_pulldownslewrate(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb) {
		return -ENOTSUP;
	}

	LEVEL_DEBUG("FS_w_pulldownslewrate\n");

	if (OWQ_U(owq) > 7) {
		return -ENOTSUP;
	}

	pn->selected_connection->connin.usb.pulldownslewrate = OWQ_U(owq);
	++pn->selected_connection->changed_bus_settings;	// force a reset

	LEVEL_DEBUG("Set slewrate to %d\n", pn->selected_connection->connin.usb.pulldownslewrate);

	return 0;
}

/*
 * Value is between 8 and 15, which represents 8us and 15us.
 * Default value is 10us. (with altUSB)
 * Default value is 12us. (without altUSB)
 */
static int FS_r_writeonelowtime(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb) {
		OWQ_U(owq) = 10;
	} else {
		OWQ_U(owq) = pn->selected_connection->connin.usb.writeonelowtime + 8;
	}
	return 0;
}

static int FS_w_writeonelowtime(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb) {
		return -ENOTSUP;
	}

	if ((OWQ_U(owq) < 8) || (OWQ_U(owq) > 15)) {
		return -ENOTSUP;
	}

	pn->selected_connection->connin.usb.writeonelowtime = OWQ_U(owq) - 8;
	++pn->selected_connection->changed_bus_settings;	// force a reset

	return 0;
}

/*
 * Value is between 3 and 10, which represents 3us and 10us.
 * Default value is 8us. (with altUSB)
 * Default value is 7us. (without altUSB)
 */
static int FS_r_datasampleoffset(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb) {
		OWQ_U(owq) = 8;
	} else {
		OWQ_U(owq) = pn->selected_connection->connin.usb.datasampleoffset + 3;
	}
	return 0;
}

static int FS_w_datasampleoffset(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->busmode != bus_usb){
		return -ENOTSUP;
	}

	if ((OWQ_U(owq) < 3) || (OWQ_U(owq) > 10)) {
		return -ENOTSUP;
	}

	pn->selected_connection->connin.usb.datasampleoffset = OWQ_U(owq) - 3;
	++pn->selected_connection->changed_bus_settings;	// force a reset

	return 0;
}

/* special check, -remote file length won't match local sizes */
static int FS_name(struct one_wire_query *owq)
{
	char *name = "";
	struct parsedname *pn = PN(owq);
	//printf("NAME %d=%s\n",pn->selected_connection->index,pn->selected_connection->adapter_name);
	if (pn->selected_connection->adapter_name) {
		name = pn->selected_connection->adapter_name;
	}
	Fowq_output_offset_and_size_z(name, owq);
	return 0;
}

/* special check, -remote file length won't match local sizes */
static int FS_port(struct one_wire_query *owq)
{
	char *name = "";
	struct parsedname *pn = PN(owq);
	if (pn->selected_connection->name) {
		name = pn->selected_connection->name;
	}
	Fowq_output_offset_and_size_z(name, owq);
	return 0;
}

/* special check, -remote file length won't match local sizes */
static int FS_version(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_U(owq) = pn->selected_connection->Adapter;
	return 0;
}

static int FS_stat_p(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	OWQ_U(owq) = pn->selected_connection->bus_stat[pn->selected_filetype->data.i];
	return 0;
}

static int FS_time_p(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	struct timeval *tv;

	if (pn->selected_filetype == NULL) {
		return -ENOENT;
	}
	switch (pn->selected_filetype->data.i) {
	case 0:
		tv = &(pn->selected_connection->bus_time);
		break;
	default:
		return -ENOENT;
	}
	OWQ_F(owq) = (_FLOAT) tv->tv_sec + ((_FLOAT) (tv->tv_usec / 1000)) / 1000.0;
	return 0;
}

static int FS_time(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int dindex = pn->extension;
	struct timeval *tv;
	if (dindex < 0) {
		dindex = 0;
	}
	if (pn->selected_filetype == NULL) {
		return -ENOENT;
	}
	tv = (struct timeval *) pn->selected_filetype->data.v;
	if (tv == NULL) {
		return -ENOENT;
	}

	OWQ_F(owq) = (_FLOAT) tv[dindex].tv_sec + ((_FLOAT) (tv[dindex].tv_usec / 1000)) / 1000.0;
	return 0;
}

static int FS_elapsed(struct one_wire_query *owq)
{
	OWQ_U(owq) = time(NULL) - StateInfo.start_time;
	return 0;
}
