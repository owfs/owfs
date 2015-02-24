/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* interface is data and control of bus master */

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
READ_FUNCTION(FS_r_channel);
READ_FUNCTION(FS_r_yesno);
WRITE_FUNCTION(FS_w_yesno);
READ_FUNCTION(FS_r_reconnect);
WRITE_FUNCTION(FS_w_reconnect);
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
READ_FUNCTION(FS_r_templimit);
WRITE_FUNCTION(FS_w_templimit);
//#define DEBUG_DS2490
#ifdef DEBUG_DS2490
READ_FUNCTION(FS_r_ds2490status);
#endif

/* Statistics reporting */
READ_FUNCTION(FS_stat_p);
READ_FUNCTION(FS_bustime);
READ_FUNCTION(FS_elapsed);

#if OW_USB
int DS9490_getstatus(BYTE * buffer, int readlen, const struct parsedname *pn);
#endif

/* Elabnet functions */
READ_FUNCTION(FS_r_PBM_version);
READ_FUNCTION(FS_r_PBM_serial);
READ_FUNCTION(FS_r_PBM_channel);
READ_FUNCTION(FS_r_PBM_features);
READ_FUNCTION(FS_w_PBM_activationcode);

static enum e_visibility VISIBLE_DS2482( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_i2c:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_DS9490( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_usb:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_DS2480B( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_serial:
		case bus_xport:
		case bus_pbm:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_HA5( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_ha5:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_PBM( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_pbm:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_DS1WM( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_ds1wm:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_PSEUDO( const struct parsedname * pn )
{
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
static struct filetype interface_settings[] = {
	{"name", 128, NON_AGGREGATE, ft_vascii, fc_static, FS_name, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"address", 512, NON_AGGREGATE, ft_vascii, fc_static, FS_port, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"overdrive", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE, {.s=offsetof(struct connection_in,overdrive),}, },
	{"version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"ds2404_found", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE, {.s=offsetof(struct connection_in,ds2404_found),}, },
	{"reconnect", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_reconnect, FS_w_reconnect, VISIBLE, NO_FILETYPE_DATA, },

	{"usb", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_DS9490, NO_FILETYPE_DATA, },
	#ifdef DEBUG_DS2490
	{"usb/ds2490status", 128, NON_AGGREGATE, ft_vascii, fc_static, FS_r_ds2490status, NO_WRITE_FUNCTION, VISIBLE_DS9490, NO_FILETYPE_DATA, },
	#endif
	{"usb/pulldownslewrate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_pulldownslewrate, FS_w_pulldownslewrate, VISIBLE_DS9490, NO_FILETYPE_DATA, },
	{"usb/writeonelowtime", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_writeonelowtime, FS_w_writeonelowtime, VISIBLE_DS9490, NO_FILETYPE_DATA, },
	{"usb/datasampleoffset", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_datasampleoffset, FS_w_datasampleoffset, VISIBLE_DS9490, NO_FILETYPE_DATA, },
	{"usb/flexible_timing", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_DS9490, {.s=offsetof(struct connection_in,flex),}, },

	{"serial", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_DS2480B, NO_FILETYPE_DATA, },
	{"serial/reverse_polarity", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_DS2480B, {.s=offsetof(struct connection_in,master.serial.reverse_polarity),}, },
	{"serial/baudrate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_baud, FS_w_baud, VISIBLE_DS2480B, NO_FILETYPE_DATA, },
	{"serial/flexible_timing", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_DS2480B, {.s=offsetof(struct connection_in,flex),}, },

	{"ha5", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_HA5, NO_FILETYPE_DATA, },
	{"ha5/checksum", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_HA5, {.s=offsetof(struct connection_in,master.ha5.checksum), }, },
	{"ha5/channel", 1, NON_AGGREGATE, ft_ascii, fc_static, FS_r_channel, NO_WRITE_FUNCTION, VISIBLE_HA5, NO_FILETYPE_DATA, },

	/* Elabnet PBM */
	{"PBM", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"PBM/port", 1, NON_AGGREGATE, ft_ascii, fc_static, FS_r_PBM_channel, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"PBM/firmware_version", 64, NON_AGGREGATE, ft_ascii, fc_static, FS_r_PBM_version, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"PBM/serial", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_PBM_serial, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"PBM/features", 256, NON_AGGREGATE, ft_ascii, fc_static, FS_r_PBM_features, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"PBM/activation_code", 128, NON_AGGREGATE, ft_ascii, fc_static,  NO_READ_FUNCTION, FS_w_PBM_activationcode, VISIBLE_PBM, NO_FILETYPE_DATA, },

	{"DS1WM", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_PBM, NO_FILETYPE_DATA, },
	{"DS1WM/long_line_mode", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_DS1WM, {.s=offsetof(struct connection_in,master.ds1wm.longline),}, },
	{"DS1WM/pulse_presence_mask", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE_DS1WM, {.s=offsetof(struct connection_in,master.ds1wm.presence_mask),}, },

	{"i2c", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_DS2482, NO_FILETYPE_DATA, },
	{"i2c/ActivePullUp", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_APU, FS_w_APU, VISIBLE_DS2482, NO_FILETYPE_DATA, },
	{"i2c/PulsePresenceMask", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_PPM, FS_w_PPM, VISIBLE_DS2482, NO_FILETYPE_DATA, },

	{"simulated", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_PSEUDO, NO_FILETYPE_DATA, },
	{"simulated/templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE_PSEUDO, {.i=1}, },
	{"simulated/temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE_PSEUDO, {.i=0}, },
};
struct device d_interface_settings = { 
	"settings", 
	"settings", 
	ePN_interface,
	COUNT_OF_FILETYPES(interface_settings), 
	interface_settings,
	NO_GENERIC_READ,
	NO_GENERIC_WRITE
};

static struct filetype interface_statistics[] = {
	{"elapsed_time", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_elapsed, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"bus_time", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_statistic, FS_bustime, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"reconnects", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_reconnects}, },
	{"reconnect_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_reconnect_errors}, },
	{"locks", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_locks}, },
	{"unlocks", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_unlocks}, },
	{"errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_errors}, },
	{"resets", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_resets}, },
	{"program_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_program_errors}, },
	{"pullup_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_pullup_errors}, },
	{"reset_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_reset_errors}, },
	{"shorts", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_short_errors}, },
	{"read_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_read_errors}, },
	{"write_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_write_errors}, },
	{"open_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_open_errors}, },
	{"close_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_close_errors}, },
	{"detect_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_detect_errors}, },
	{"select_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_select_errors}, },
	{"status_errors", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_status_errors}, },
	{"timeouts", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_timeouts}, },

	{"search_errors", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"search_errors/error_pass_1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_search_errors1}, },
	{"search_errors/error_pass_2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_search_errors2}, },
	{"search_errors/error_pass_3", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_search_errors3}, },

	{"overdrive", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"overdrive/attempts", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_try_overdrive}, },
	{"overdrive/failures", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_statistic, FS_stat_p, NO_WRITE_FUNCTION, VISIBLE, {.i=e_bus_failed_overdrive}, },
};

struct device d_interface_statistics = { 
	"statistics", 
	"statistics", 
	0,
	COUNT_OF_FILETYPES(interface_statistics), 
	interface_statistics,
	NO_GENERIC_READ,
	NO_GENERIC_WRITE
};


/* ------- Functions ------------ */

/* Just some tests to support reconnection */
static ZERO_OR_ERROR FS_r_reconnect(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_Y(owq) = (pn->selected_connection->reconnect_state == reconnect_error);
	return 0;
}

/* Just some tests to support reconnection */
static ZERO_OR_ERROR FS_w_reconnect(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	pn->selected_connection->reconnect_state = OWQ_Y(owq) ? reconnect_error : reconnect_ok ;
	return 0;
}

/* DS2482 APU setting */
#define DS2482_REG_CFG_APU     0x01
static ZERO_OR_ERROR FS_r_APU(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_i2c:
			OWQ_Y(owq) = ( (pn->selected_connection->master.i2c.configreg & DS2482_REG_CFG_APU) != 0x00 ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static ZERO_OR_ERROR FS_w_APU(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_i2c:
			if ( OWQ_Y(owq) ) {
				pn->selected_connection->master.i2c.configreg |= DS2482_REG_CFG_APU ;
			} else {
				pn->selected_connection->master.i2c.configreg &= ~DS2482_REG_CFG_APU ;
			}
			break ;
		default:
			break ;
	}
	return 0 ;
}

static ZERO_OR_ERROR FS_r_yesno( struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	struct connection_in * in = pn->selected_connection ;
	struct filetype * ft = pn->selected_filetype ;
	size_t struct_offset = ft->data.s ;
	char * in_loc = (void *) in ;
	int * p_yes_no = (int *)(in_loc + struct_offset) ;

	switch( (ft->visible)(pn) ) {
		case visible_not_now:
		case visible_never:
			return -ENOTSUP ;
		default:
			break ;
	}

	OWQ_Y(owq) = p_yes_no[0] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_yesno( struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	struct connection_in * in = pn->selected_connection ;
	struct filetype * ft = pn->selected_filetype ;
	size_t struct_offset = ft->data.s ;
	char * in_loc = (void *) in ;
	int * p_yes_no = (int *)(in_loc + struct_offset) ;

	switch( (ft->visible)(pn) ) {
		case visible_not_now:
		case visible_never:
			return -ENOTSUP ;
		default:
			break ;
	}

	p_yes_no[0] = OWQ_Y(owq) ;
	in->changed_bus_settings |= CHANGED_USB_SPEED ;
	return 0 ;
}

/* fake adapter temperature limits */
static ZERO_OR_ERROR FS_r_templimit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_fake:
		case bus_mock:
		case bus_tester:
			OWQ_F(owq) = pn->selected_filetype->data.i ? pn->selected_connection->master.fake.templow : pn->selected_connection->master.fake.temphigh;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static ZERO_OR_ERROR FS_w_templimit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	switch ( get_busmode(pn->selected_connection) ) {
		case bus_fake:
		case bus_mock:
		case bus_tester:
			if (pn->selected_filetype->data.i) {
				pn->selected_connection->master.fake.templow = OWQ_F(owq);
			} else {
				pn->selected_connection->master.fake.temphigh = OWQ_F(owq);
			}
			return 0;
		default:
			break ;
	}
	return 0 ;
}

/* Serial baud rate */
static ZERO_OR_ERROR FS_r_baud(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	switch ( get_busmode(in) ) {
		case bus_serial:
		case bus_link:
		case bus_masterhub:
		case bus_ha5:
		case bus_ha7e:
		case bus_pbm:
			OWQ_U(owq) = COM_BaudRate( in->pown->baud ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static ZERO_OR_ERROR FS_w_baud(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	switch ( get_busmode(in) ) {
		case bus_serial:
		case bus_link:
		case bus_masterhub:
		case bus_pbm:
			in->pown->baud = COM_MakeBaud( (speed_t) OWQ_U(owq) ) ;
			++in->changed_bus_settings ;
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* DS2482 PPM setting */
#define DS2482_REG_CFG_PPM     0x02
static ZERO_OR_ERROR FS_r_PPM(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	switch ( get_busmode(in) ) {
		case bus_i2c:
			OWQ_Y(owq) = ( (in->master.i2c.configreg & DS2482_REG_CFG_PPM) != 0x00 ) ;
			return 0;
		default:
			return -ENOTSUP ;
	}
}

static ZERO_OR_ERROR FS_w_PPM(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	switch ( get_busmode(in) ) {
		case bus_i2c:
			if ( OWQ_Y(owq) ) {
				in->master.i2c.configreg |= DS2482_REG_CFG_PPM ;
			} else {
				in->master.i2c.configreg &= ~DS2482_REG_CFG_PPM ;
			}
			break ;
		default:
			break ;
	}
	return 0 ;
}

/* For HA5 channel -- a single letter */
static ZERO_OR_ERROR FS_r_channel(struct one_wire_query *owq)
{
	return OWQ_format_output_offset_and_size( (char *) &(PN(owq)->selected_connection->master.ha5.channel), 1, owq);
}

/* For PBM channel -- a single letter */
static ZERO_OR_ERROR FS_r_PBM_channel(struct one_wire_query *owq)
{
	char port = PN(owq)->selected_connection->master.pbm.channel + '1';
	return OWQ_format_output_offset_and_size(&port, 1, owq);
}

/* PBM Firmware version */
static ZERO_OR_ERROR FS_r_PBM_version(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	int majorvers = in->master.pbm.version >> 16;
	int minorvers = in->master.pbm.version & 0xffff;
	char res[64];
	res[0] = '\0';
	sprintf(res, "%d.%3.3d", majorvers, minorvers);
	return OWQ_format_output_offset_and_size_z(res, owq);
}

SIZE_OR_ERROR PBM_SendCMD(const BYTE * tx, size_t size, BYTE * rx, size_t rxsize, struct connection_in * in, int tout);

/* List available features */
static ZERO_OR_ERROR FS_r_PBM_features(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	char res[256] = {0};
	const BYTE cmd_listlics[] = "ks\n";

	// some "magic" numbers -- 3 must be command length
	// 500 meaning is unclear.
	PBM_SendCMD(cmd_listlics, 3, (BYTE *) res, sizeof(res), in, 500);
	return OWQ_format_output_offset_and_size_z(res, owq);
}

/* Add new license into device */
ZERO_OR_ERROR FS_w_PBM_activationcode(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	size_t size = OWQ_size(owq) ;
	BYTE * cmd_string = owmalloc( size+5 ) ;
					
	if ( cmd_string == NULL ) {
		return -ENOMEM ;
	}

	cmd_string[0] = 'k';
	cmd_string[1] = 'a';
	memcpy(&cmd_string[2], OWQ_buffer(owq), size ) ;
	cmd_string[size+2] = '\r';
	
	// Writes from and to cmd_string
	PBM_SendCMD(cmd_string, size + 3, cmd_string, size + 3, in, 500);

	owfree(cmd_string) ;
	return 0;
}

/* Read serialnumber */
static ZERO_OR_ERROR FS_r_PBM_serial(struct one_wire_query *owq)
{
	struct connection_in * in = PN(owq)->selected_connection ;
	OWQ_U(owq) = in->master.pbm.serial_number;
	
	return 0 ;
}

#ifdef DEBUG_DS2490
static ZERO_OR_ERROR FS_r_ds2490status(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	char res[256];
	char buffer[ DS9490_getstatus_BUFFER_LENGTH ];
	int ret;
	res[0] = '\0';
	if (get_busmode(pn->selected_connection) == bus_usb) {
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
	return OWQ_format_output_offset_and_size_z(res, owq);
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
static ZERO_OR_ERROR FS_r_pulldownslewrate(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb) {
		return -ENOTSUP;
#if OW_USB
	} else {
		OWQ_U(owq) = pn->selected_connection->master.usb.pulldownslewrate;
#endif /* OW_USB */
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_pulldownslewrate(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb) {
		return -ENOTSUP;
	}
#if OW_USB
	if (OWQ_U(owq) > 7) {
		return -ENOTSUP;
	}

	pn->selected_connection->master.usb.pulldownslewrate = OWQ_U(owq);
	pn->selected_connection->changed_bus_settings |= CHANGED_USB_SLEW ;	// force a reset

	LEVEL_DEBUG("Set slewrate to %d", pn->selected_connection->master.usb.pulldownslewrate);
#endif /* OW_USB */
	return 0;
}

/*
 * Value is between 8 and 15, which represents 8us and 15us.
 * Default value is 10us. (with altUSB)
 * Default value is 12us. (without altUSB)
 */
static ZERO_OR_ERROR FS_r_writeonelowtime(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb) {
		OWQ_U(owq) = 10;
#if OW_USB
	} else {
		OWQ_U(owq) = pn->selected_connection->master.usb.writeonelowtime + 8;
#endif /* OW_USB */
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_writeonelowtime(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb) {
		return -ENOTSUP;
	}

#if OW_USB
	if ((OWQ_U(owq) < 8) || (OWQ_U(owq) > 15)) {
		return -ENOTSUP;
	}

	pn->selected_connection->master.usb.writeonelowtime = OWQ_U(owq) - 8;
	pn->selected_connection->changed_bus_settings |= CHANGED_USB_LOW ;	// force a reset
#endif /* OW_USB */

	return 0;
}

/*
 * Value is between 3 and 10, which represents 3us and 10us.
 * Default value is 8us. (with altUSB)
 * Default value is 7us. (without altUSB)
 */
static ZERO_OR_ERROR FS_r_datasampleoffset(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb) {
		OWQ_U(owq) = 8;
#if OW_USB
	} else {
		OWQ_U(owq) = pn->selected_connection->master.usb.datasampleoffset + 3;
#endif /* OW_USB */
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_datasampleoffset(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (get_busmode(pn->selected_connection) != bus_usb){
		return -ENOTSUP;
	}

#if OW_USB
	if ((OWQ_U(owq) < 3) || (OWQ_U(owq) > 10)) {
		return -ENOTSUP;
	}

	pn->selected_connection->master.usb.datasampleoffset = OWQ_U(owq) - 3;
	pn->selected_connection->changed_bus_settings |= CHANGED_USB_OFFSET;	// force a reset
#endif /* OW_USB */

	return 0;
}

/* special check, -remote file length won't match local sizes */
static ZERO_OR_ERROR FS_name(struct one_wire_query *owq)
{
	char *name = "";
	struct parsedname *pn = PN(owq);
	//printf("NAME %d=%s\n",pn->selected_connection->index,pn->selected_connection->adapter_name);
	if (pn->selected_connection->adapter_name) {
		name = pn->selected_connection->adapter_name;
	}
	return OWQ_format_output_offset_and_size_z(name, owq);
}

/* special check, -remote file length won't match local sizes */
static ZERO_OR_ERROR FS_port(struct one_wire_query *owq)
{
	return OWQ_format_output_offset_and_size_z(
		SAFESTRING( DEVICENAME(PN(owq)->selected_connection)),
		owq);
}

/* special check, -remote file length won't match local sizes */
static ZERO_OR_ERROR FS_version(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_U(owq) = pn->selected_connection->Adapter;
	return 0;
}

static ZERO_OR_ERROR FS_stat_p(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	OWQ_U(owq) = pn->selected_connection->bus_stat[pn->selected_filetype->data.i];
	return 0;
}

static ZERO_OR_ERROR FS_bustime(struct one_wire_query *owq)
{
	OWQ_F(owq) = TVfloat( &(PN(owq)->selected_connection->bus_time) ) ;
	return 0;
}

static ZERO_OR_ERROR FS_elapsed(struct one_wire_query *owq)
{
	OWQ_U(owq) = NOW_TIME - StateInfo.start_time;
	return 0;
}
