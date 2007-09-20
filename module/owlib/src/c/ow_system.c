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
#include "ow_system.h"
#include "ow_pid.h"
#include "ow_connection.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
READ_FUNCTION(FS_name);
READ_FUNCTION(FS_port);
READ_FUNCTION(FS_pidfile);
READ_FUNCTION(FS_pid);
READ_FUNCTION(FS_in);
READ_FUNCTION(FS_out);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_r_overdrive);
WRITE_FUNCTION(FS_w_overdrive);
READ_FUNCTION(FS_r_ds2404_compliance);
WRITE_FUNCTION(FS_w_ds2404_compliance);
READ_FUNCTION(FS_r_pulldownslewrate);
WRITE_FUNCTION(FS_w_pulldownslewrate);
READ_FUNCTION(FS_r_writeonelowtime);
WRITE_FUNCTION(FS_w_writeonelowtime);
READ_FUNCTION(FS_r_datasampleoffset);
WRITE_FUNCTION(FS_w_datasampleoffset);
//#define DEBUG_DS2490
#ifdef DEBUG_DS2490
READ_FUNCTION(FS_r_ds2490status);
#endif
READ_FUNCTION(FS_define);
#if OW_USB
int DS9490_getstatus(BYTE * buffer, int readlen, const struct parsedname *pn);
#endif

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
struct aggregate Asystem = { 1, ag_numbers, ag_separate, };
struct filetype sys_adapter[] = {
  {"name", 128, &Asystem, ft_vascii, fc_static,   FS_name, NO_WRITE_FUNCTION, {v:NULL},} ,
	// variable length
  {"address", 512, &Asystem, ft_vascii, fc_static,   FS_port, NO_WRITE_FUNCTION, {v:NULL},} ,
	// variable length
  {"datasampleoffset",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static,   FS_r_datasampleoffset, FS_w_datasampleoffset, {v:NULL},} ,
  {"ds2404_compliance",PROPERTY_LENGTH_YESNO, &Asystem, ft_yesno, fc_static,   FS_r_ds2404_compliance, FS_w_ds2404_compliance, {v:NULL},} ,
  {"overdrive",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static,   FS_r_overdrive, FS_w_overdrive, {v:NULL},} ,
  {"pulldownslewrate",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static,   FS_r_pulldownslewrate, FS_w_pulldownslewrate, {v:NULL},} ,
#ifdef DEBUG_DS2490
  {"ds2490status", 128, &Asystem, ft_vascii, fc_static,   FS_r_ds2490status, NO_WRITE_FUNCTION, {v:NULL},} ,
#endif
  {"version",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static,   FS_version, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"writeonelowtime",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static,   FS_r_writeonelowtime, FS_w_writeonelowtime, {v:NULL},} ,
};
struct device d_sys_adapter =
	{ "adapter", "adapter", pn_system, COUNT_OF_FILETYPES(sys_adapter), sys_adapter };

/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_process[] = {
  {"pidfile", 128, NULL, ft_vascii, fc_static,   FS_pidfile, NO_WRITE_FUNCTION, {v:NULL},} ,
	// variable length
  {"pid",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_pid, NO_WRITE_FUNCTION, {v:NULL},} ,
};
struct device d_sys_process =
	{ "process", "process", pn_system, COUNT_OF_FILETYPES(sys_process), sys_process };

struct filetype sys_connections[] = {
  {"count_inbound_connections",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_in, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"count_outbound_connections",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_out, NO_WRITE_FUNCTION, {v:NULL},} ,
};
struct device d_sys_connections =
	{ "connections", "connections", pn_system, COUNT_OF_FILETYPES(sys_connections),
	sys_connections
};

struct filetype sys_configure[] = {
  {"threaded",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_MT},} ,
  {"tai8570",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_TAI8570},} ,
  {"thermocouples",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_THERMOCOUPLE},} ,
  {"parport",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_PARPORT},} ,
  {"USB",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_USB},} ,
  {"i2c",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_I2C},} ,
  {"cache",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_CACHE},} ,
  {"HA7Net",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_HA7},} ,
  {"DebugInfo",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:OW_DEBUG},} ,
  {"zeroconf",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static,   FS_define, NO_WRITE_FUNCTION, {i:1},} ,
};
struct device d_sys_configure =
	{ "configuration", "configuration", pn_system, COUNT_OF_FILETYPES(sys_configure),
	sys_configure
};

/* ------- Functions ------------ */


/* Just some tests to support change of extra delay */
static int FS_r_ds2404_compliance(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	OWQ_Y(owq) = in->ds2404_compliance;
	return 0;
}

static int FS_w_ds2404_compliance(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	in->ds2404_compliance = OWQ_Y(owq);
	return 0;
}

/* Just some tests to support overdrive */
static int FS_r_overdrive(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	OWQ_U(owq) = in->use_overdrive_speed;
	return 0;
}

static int FS_w_overdrive(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	switch (OWQ_U(owq)) {
	case 0:
		in->use_overdrive_speed = ONEWIREBUSSPEED_REGULAR;
		break;
	case 1:
		if (pn->in->Adapter != adapter_DS9490)
			return -ENOTSUP;
		in->use_overdrive_speed = ONEWIREBUSSPEED_FLEXIBLE;
		break;
	case 2:
		in->use_overdrive_speed = ONEWIREBUSSPEED_OVERDRIVE;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

#ifdef DEBUG_DS2490
static int FS_r_ds2490status(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	char res[256];
	char buffer[32+1];
	int ret;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	res[0] = '\0';
	if (in->busmode == bus_usb) {
#if OW_USB
		ret = DS9490_getstatus(buffer, 0, PN(owq));
		if(ret < 0) {
			sprintf(res, "DS9490_getstatus failed: %d\n", ret);
		} else {
			sprintf(res,
				"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				buffer[0], buffer[1], buffer[2], buffer[3],
				buffer[4], buffer[5], buffer[6], buffer[7],
				buffer[8], buffer[9], buffer[10], buffer[11],
				buffer[12], buffer[13], buffer[14], buffer[15]);
		}
		/*
		  uchar	EnableFlags;
		  uchar	OneWireSpeed;
		  uchar	StrongPullUpDuration;
		  uchar	ProgPulseDuration;
		  uchar	PullDownSlewRate;
		  uchar	Write1LowTime;
		  uchar	DSOW0RecoveryTime;
		  uchar	Reserved1;
		  uchar	StatusFlags;
		  uchar	CurrentCommCmd1;
		  uchar	CurrentCommCmd2;
		  uchar	CommBufferStatus;  // Buffer for COMM commands
		  uchar	WriteBufferStatus; // Buffer we write to
		  uchar	ReadBufferStatus;  // Buffer we read from
		*/
#endif
	}
	Fowq_output_offset_and_size_z( res, owq ) ;
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
static int FS_r_pulldownslewrate(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		OWQ_U(owq) = 3;
	else
		OWQ_U(owq) = in->connin.usb.pulldownslewrate;

	return 0;
}

static int FS_w_pulldownslewrate(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	LEVEL_DEBUG("FS_w_pulldownslewrate\n");

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		return -ENOTSUP;

	if(OWQ_U(owq) > 7)
		return -ENOTSUP;
	in->connin.usb.pulldownslewrate = OWQ_U(owq);
	LEVEL_DEBUG("Set slewrate to %d\n", in->connin.usb.pulldownslewrate);
#if OW_USB
	DS9490_BusParm(in);
#endif
	return 0;
}

/*
 * Value is between 8 and 15, which represents 8us and 15us.
 * Default value is 10us. (with altUSB)
 * Default value is 12us. (without altUSB)
 */
static int FS_r_writeonelowtime(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;
	
	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		OWQ_U(owq) = 10;
	else
		OWQ_U(owq) = in->connin.usb.writeonelowtime + 8;

	return 0;
}

static int FS_w_writeonelowtime(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		return -ENOTSUP;

	if((OWQ_U(owq) < 8) || (OWQ_U(owq) > 15))
		return -ENOTSUP;
	in->connin.usb.writeonelowtime = OWQ_U(owq) - 8;
#if OW_USB
	DS9490_BusParm(in);
#endif
	return 0;
}

/*
 * Value is between 3 and 10, which represents 3us and 10us.
 * Default value is 8us. (with altUSB)
 * Default value is 7us. (without altUSB)
 */
static int FS_r_datasampleoffset(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		OWQ_U(owq) = 8;
	else
		OWQ_U(owq) = in->connin.usb.datasampleoffset + 3;
	return 0;
}

static int FS_w_datasampleoffset(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;
	if (in->busmode != bus_usb)
		return -ENOTSUP;

	if((OWQ_U(owq) < 3) || (OWQ_U(owq) > 10))
		return -ENOTSUP;
	in->connin.usb.datasampleoffset = OWQ_U(owq) - 3;
#if OW_USB
	DS9490_BusParm(in);
#endif
	return 0;
}

/* special check, -remote file length won't match local sizes */
static int FS_name(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;
	char * name = "" ;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	if (in->adapter_name )
        name = in->adapter_name ;
	Fowq_output_offset_and_size_z( name, owq ) ;
	return 0 ;
}

/* special check, -remote file length won't match local sizes */
static int FS_port(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;
	char * name = "" ;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	if (in->name) name = in->name ;
	Fowq_output_offset_and_size_z( name, owq ) ;
	return 0 ;
}

/* special check, -remote file length won't match local sizes */
static int FS_version(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	OWQ_U(owq) = in->Adapter;
	return 0;
}

static int FS_pidfile(struct one_wire_query * owq)
{
	char * name = "" ;
	if (pid_file) name = pid_file ;
	Fowq_output_offset_and_size_z( name, owq ) ;
	return 0 ;
}

static int FS_pid(struct one_wire_query * owq)
{
	OWQ_U(owq) = getpid();
	return 0;
}

static int FS_in(struct one_wire_query * owq)
{
	CONNINLOCK;
	OWQ_U(owq) = count_inbound_connections;
	CONNINUNLOCK;
	return 0;
}

static int FS_out(struct one_wire_query * owq)
{
	OWQ_U(owq) = count_outbound_connections;
	return 0;
}

static int FS_define(struct one_wire_query * owq)
{
	OWQ_Y(owq) = OWQ_pn(owq).selected_filetype->data.i;
	return 0;
}
