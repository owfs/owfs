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
READ_FUNCTION(FS_pidfile);
READ_FUNCTION(FS_pid);
READ_FUNCTION(FS_in);
READ_FUNCTION(FS_out);
READ_FUNCTION(FS_define);

/* -------- Structures ---------- */
/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_process[] = {
  {"pidfile", 128, NULL, ft_vascii, fc_static,   FS_pidfile, NO_WRITE_FUNCTION, {v:NULL},} ,
	// variable length
  {"pid",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_pid, NO_WRITE_FUNCTION, {v:NULL},} ,
};
struct device d_sys_process =
	{ "process", "process", ePN_system, COUNT_OF_FILETYPES(sys_process), sys_process };

struct filetype sys_connections[] = {
  {"count_inbound_connections",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_in, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"count_outbound_connections",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static,   FS_out, NO_WRITE_FUNCTION, {v:NULL},} ,
};
struct device d_sys_connections =
	{ "connections", "connections", ePN_system, COUNT_OF_FILETYPES(sys_connections),
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
	{ "configuration", "configuration", ePN_system, COUNT_OF_FILETYPES(sys_configure),
	sys_configure
};

/* ------- Functions ------------ */

#ifdef DEBUG_DS2490
static int FS_r_ds2490status(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	char res[256];
	char buffer[32+1];
	int ret;

    SetKnownBus(pn->extension,pn) ;

	res[0] = '\0';
    if (pn->selected_connection->busmode == bus_usb) {
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
