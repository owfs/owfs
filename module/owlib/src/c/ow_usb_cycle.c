/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* DS9490R-W USB 1-Wire master

   USB parameters:
       Vendor ID: 04FA
       ProductID: 2490

   Dallas controller DS2490

*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_usb_msg.h"
#include "ow_usb_cycle.h"

#if OW_USB

static void DS9490_dir_callback( void * v, const struct parsedname * pn_entry );
static GOOD_OR_BAD lusbdevice_in_use(int address, int bus_number);

/* ------------------------------------------------------------ */
/* --- USB bus scaning -----------------------------------------*/

GOOD_OR_BAD USB_match(libusb_device * dev)
{
	struct libusb_device_descriptor lusbd ;
	int libusb_err ;
	
	if ( (libusb_err=libusb_get_device_descriptor( dev, &lusbd )) != 0 ) {
		LEVEL_DEBUG("%s",libusb_error_name(libusb_err));
		return gbBAD ;
	}
	
	if ( lusbd.idVendor != DS2490_USB_VENDOR ) {
		return gbBAD ;
	}
	
	if ( lusbd.idProduct != DS2490_USB_PRODUCT ) {
		return gbBAD ;
	}
	
	return lusbdevice_in_use( libusb_get_device_address(dev), libusb_get_bus_number(dev) ) ;
}

/* Used only in root_dir */
static void DS9490_dir_callback( void * v, const struct parsedname * pn_entry )
{
	struct dirblob * db = v ;

	LEVEL_DEBUG("Callback on %s",SAFESTRING(pn_entry->path));
	if ( pn_entry->sn[0] != '\0' ) {
		DirblobAdd( pn_entry->sn, db ) ;
	}
}

/* Get the root directory listing for finding a 1-wire tag for a DS9490 */
/* Only error is if parsename fails */
GOOD_OR_BAD DS9490_root_dir( struct dirblob * db, struct connection_in * in )
{
	ASCII path[PATH_MAX] ;
	struct parsedname pn_root ;

	UCLIBCLOCK;
		/* Force this adapter with bus.n path */
		snprintf(path, PATH_MAX, "/uncached/bus.%d", in->index);
	UCLIBCUNLOCK;

	if ( FS_ParsedName(path, &pn_root) != 0 ) {
		LEVEL_DATA("Cannot get root directory on [%s] Parsing %s error.", SAFESTRING(DEVICENAME(in)), path);
		return gbBAD ;
	}
	DirblobInit( db ) ;
	/* First time pretend there are devices */
	pn_root.selected_connection->changed_bus_settings |= CHANGED_USB_SPEED ;	// Trigger needing new configuration
	pn_root.selected_connection->overdrive = 0 ;	// not overdrive at start
	pn_root.selected_connection->flex = Globals.usb_flextime ;
	
	SetReconnect(&pn_root) ;
	FS_dir( DS9490_dir_callback, db, &pn_root ) ;
	LEVEL_DEBUG("Finished FS_dir");
	FS_ParsedName_destroy(&pn_root) ;

	return gbGOOD ;
	// Dirblob must be cleared by recipient.
}

/* Found a DS9490 that seems good, now check list and find a device to ID for reconnects */
/* Choose in order:
 * (first) 0x81
 * (first) 0x01
 * first other family
 * 0x00
 * */
GOOD_OR_BAD DS9490_ID_this_master(struct connection_in *in)
{
	struct dirblob db ;
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	int device_number ;

	
	RETURN_BAD_IF_BAD( DS9490_root_dir( &db, in ) ) ;

	// Use 0x00 if no devices (homegrown adapters?)
	if ( DirblobElements( &db) == 0 ) {
		DirblobClear( &db ) ;
		memset( in->master.usb.ds1420_address, 0, SERIAL_NUMBER_SIZE ) ;
		LEVEL_DEFAULT("Set DS9490 %s unique id 0x00 (no devices at all)", SAFESTRING(DEVICENAME(in))) ;
		return gbGOOD ;
	}
	
	// look for the special 0x81 device
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (sn[0] == 0x81) {	// 0x81 family code
			memcpy(in->master.usb.ds1420_address, sn, SERIAL_NUMBER_SIZE);
			LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat, SAFESTRING(DEVICENAME(in)), SNvar(in->master.usb.ds1420_address));
			DirblobClear( &db ) ;
			return gbGOOD ;
		}
		++device_number ;
	}

	// look for the (less specific, for older DS9490s) 0x01 device
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (sn[0] == 0x01) {	// 0x01 family code
			memcpy(in->master.usb.ds1420_address, sn, SERIAL_NUMBER_SIZE);
			LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat, SAFESTRING(DEVICENAME(in)), SNvar(in->master.usb.ds1420_address));
			DirblobClear( &db ) ;
			return gbGOOD ;
		}
		++device_number ;
	}

	// Take the first device, whatever it is
	DirblobGet( 0, sn, &db ) ;
	memcpy(in->master.usb.ds1420_address, sn, SERIAL_NUMBER_SIZE);
	LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat, SAFESTRING(DEVICENAME(in)), SNvar(in->master.usb.ds1420_address));
	DirblobClear( &db ) ;
	return gbGOOD;
}

// return bad if already exists and is open
// matches bus and address
static GOOD_OR_BAD lusbdevice_in_use(int address, int bus_number)
{
	struct port_in * pin ; 
	
	for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
		struct connection_in *cin;

		if ( pin->busmode != bus_usb ) {
			continue ;
		}
		for (cin = pin->first; cin != NO_CONNECTION; cin = cin->next) {
			if ( cin->master.usb.usb_bus_number != bus_number ) {
				continue ;
			}
			if ( cin->master.usb.address != address ) {
				continue ;
			}
			if ( cin->master.usb.lusb_handle != NULL ) {
				continue ;
			}
			return gbBAD;			// It seems to be in use already
		}
	}
	return gbGOOD;					// not found in the current inbound list
}

#endif							/* OW_USB */
