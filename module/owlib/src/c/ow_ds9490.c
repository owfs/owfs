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

/* DS9490R-W USB 1-Wire master

   USB parameters:
       Vendor ID: 04FA
       ProductID: 2490

   Dallas controller DS2490

*/

#ifdef OW_USB /* conditional inclusion of USB */

#include "owfs_config.h"
#include "ow.h"
#include <usb.h>
#include <sys/time.h>

/* All the rest of the code sees is the DS9490_detect routine and the iroutine structure */

static int DS9490_reset( const struct parsedname * const pn ) ;
static int DS9490_open( const struct parsedname * const pn, char *name ) ;
static int DS9490_reconnect( const struct parsedname * const pn ) ;
static int DS9490_getstatus(unsigned char * const buffer,const struct parsedname * const pn, int readlen, int wait_for_idle ) ;
static int DS9490_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len,const struct parsedname * const pn ) ;
static int DS9490_level(int new_level,const struct parsedname * const pn) ;
static void DS9490_setroutines( struct interface_routines * const f ) ;
static int DS9490_detect_low( const struct parsedname * const pn ) ;
static int DS9490_PowerByte(const unsigned char byte, const unsigned int delay,const struct parsedname * const pn) ;
static int DS9490_ProgramPulse( const struct parsedname * const pn) ;
static int DS9490_read( unsigned char * const buf, const size_t size, const struct parsedname * const pn) ;
static int DS9490_write( const unsigned char * const buf, const size_t size, const struct parsedname * const pn) ;
static int DS9490_overdrive( const unsigned int overdrive, const struct parsedname * const pn ) ;
static int DS9490_testoverdrive(const struct parsedname * const pn) ;
static int DS9490_redetect_low( const struct parsedname * const pn ) ;

/* Device-specific routines */
static void DS9490_setroutines( struct interface_routines * const f ) {
    f->write = DS9490_write ;
    f->read = DS9490_read ;
    f->reset = DS9490_reset ;
    f->next_both = DS9490_next_both ;
    f->level = DS9490_level ;
    f->PowerByte = DS9490_PowerByte ;
    f->ProgramPulse = DS9490_ProgramPulse ;
    f->sendback_data = DS9490_sendback_data ;
    f->select        = BUS_select_low ;
    f->overdrive = DS9490_overdrive ;
    f->testoverdrive = DS9490_testoverdrive ;
    f->reconnect  =  DS9490_reconnect ;
}

#define TIMEOUT_USB    5000 /* 5 seconds */

#define CONTROL_CMD     0x00
#define COMM_CMD        0x01
#define MODE_CMD        0x02
#define TEST_CMD        0x03

#define CTL_RESET_DEVICE        0x0000
#define CTL_START_EXE           0x0001
#define CTL_RESUME_EXE          0x0002
#define CTL_HALT_EXE_IDLE       0x0003
#define CTL_HALT_EXE_DONE       0x0004
#define CTL_FLUSH_COMM_CMDS     0x0007
#define CTL_FLUSH_CV_BUFFER     0x0008
#define CTL_FLUSH_CMT_BUFFER    0x0009
#define CTL_GET_COMM_CMDS       0x000A

#define MOD_PULSE_EN            0x0000
#define MOD_SPEED_CHANGE_EN     0x0001
#define MOD_1WIRE_SPEED         0x0002
#define MOD_STRONG_PU_DURATION  0x0003
#define MOD_PULLDOWN_SLEWRATE   0x0004
#define MOD_PROG_PULSE_DURATION 0x0005
#define MOD_WRITE1_LOWTIME      0x0006
#define MOD_DSOW0_TREC          0x0007

//
// Value field COMM Command options
//
// COMM Bits (bitwise or into COMM commands to build full value byte pairs)
// Byte 1
#define COMM_TYPE                   0x0008
#define COMM_SE                     0x0008
#define COMM_D                      0x0008
#define COMM_Z                      0x0008
#define COMM_CH                     0x0008
#define COMM_SM                     0x0008
#define COMM_R                      0x0008
#define COMM_IM                     0x0001

// Byte 2
#define COMM_PS                     0x4000
#define COMM_PST                    0x4000
#define COMM_CIB                    0x4000
#define COMM_RTS                    0x4000
#define COMM_DT                     0x2000
#define COMM_SPU                    0x1000
#define COMM_F                      0x0800
#define COMM_ICP                    0x0200
#define COMM_RST                    0x0100

// Read Straight command, special bits 
#define COMM_READ_STRAIGHT_NTF          0x0008
#define COMM_READ_STRAIGHT_ICP          0x0004
#define COMM_READ_STRAIGHT_RST          0x0002
#define COMM_READ_STRAIGHT_IM           0x0001

//
// Value field COMM Command options (0-F plus assorted bits)
//
#define COMM_ERROR_ESCAPE               0x0601
#define COMM_SET_DURATION               0x0012
#define COMM_BIT_IO                     0x0020
#define COMM_PULSE                      0x0030
#define COMM_1_WIRE_RESET               0x0042
#define COMM_BYTE_IO                    0x0052
#define COMM_MATCH_ACCESS               0x0064
#define COMM_BLOCK_IO                   0x0074
#define COMM_READ_STRAIGHT              0x0080
#define COMM_DO_RELEASE                 0x6092
#define COMM_SET_PATH                   0x00A2
#define COMM_WRITE_SRAM_PAGE            0x00B2
#define COMM_WRITE_EPROM                0x00C4
#define COMM_READ_CRC_PROT_PAGE         0x00D4
#define COMM_READ_REDIRECT_PAGE_CRC     0x21E4
#define COMM_SEARCH_ACCESS              0x00F4

// Mode Command Code Constants 
// Enable Pulse Constants
#define ENABLEPULSE_PRGE         0x01  // programming pulse
#define ENABLEPULSE_SPUE         0x02  // strong pull-up

// 1Wire Bus Speed Setting Constants
#define ONEWIREBUSSPEED_REGULAR        0x00
#define ONEWIREBUSSPEED_FLEXIBLE       0x01
#define ONEWIREBUSSPEED_OVERDRIVE      0x02

#define ONEWIREDEVICEDETECT               0xA5
#define COMMCMDERRORRESULT_NRS            0x01
#define COMMCMDERRORRESULT_SH             0x02 

// Device Status Flags
#define STATUSFLAGS_SPUA                       0x01  // if set Strong Pull-up is active
#define STATUSFLAGS_PRGA                       0x02  // if set a 12V programming pulse is being generated
#define STATUSFLAGS_12VP                       0x04  // if set the external 12V programming voltage is present
#define STATUSFLAGS_PMOD                       0x08  // if set the DS2490 powered from USB and external sources
#define STATUSFLAGS_HALT                       0x10  // if set the DS2490 is currently halted
#define STATUSFLAGS_IDLE                       0x20  // if set the DS2490 is currently idle



/** EP1 -- control read */
#define DS2490_EP1              0x81
/** EP2 -- bulk write */
#define DS2490_EP2              0x02
/** EP3 -- bulk read */
#define DS2490_EP3              0x83

int DS9490_detect( struct connection_in * in ) {
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;

    pn.si = &si ;
    FS_ParsedName(NULL,&pn) ;
    pn.in = in ;

    /* No locking needed for this since it's not multithreaded yet */
    in->reconnect_in_progress = 1 ;

    ret = DS9490_detect_low(&pn) ;
    if (ret==0) in->busmode = bus_usb ;

    in->reconnect_in_progress = 0 ;
    return ret ;
}



static int DS9490_setup_adapter(const struct parsedname * const pn) {
  unsigned char buffer[32];
  int ret ;
  usb_dev_handle * usb = pn->in->connin.usb.usb ;

  if((ret = usb_control_msg(usb,0x40,CONTROL_CMD,CTL_RESET_DEVICE, 0x0000, NULL, 0, TIMEOUT_USB )) < 0) {
    LEVEL_DATA("DS9490_setup_adapter: error1 ret=%d\n", ret);
    return -EIO ;
  }

  // set the strong pullup duration to infinite
  if((ret = usb_control_msg(usb,0x40,COMM_CMD,COMM_SET_DURATION | COMM_IM, 0x0000, NULL, 0, TIMEOUT_USB )) < 0) {
    LEVEL_DATA("DS9490_setup_adapter: error2 ret=%d\n", ret);
    return -EIO ;
  }
            
  // set the 12V pullup duration to 512us
  if((ret = usb_control_msg(usb,0x40,COMM_CMD,COMM_SET_DURATION | COMM_IM | COMM_TYPE, 0x0040, NULL, 0, TIMEOUT_USB )) < 0) {
    LEVEL_DATA("DS9490_setup_adapter: error3 ret=%d\n", ret);
    return -EIO ;
  }

#if 0
  if((ret = usb_control_msg(usb,0x40,MODE_CMD,MOD_PULSE_EN, ENABLEPULSE_PRGE, NULL, 0, TIMEOUT_USB )) < 0) {
    LEVEL_DATA("DS9490_setup_adapter: error4 ret=%d\n", ret);
    return -EIO ;
  }
#endif

  pn->in->ULevel = MODE_NORMAL ;

  if((ret=DS9490_getstatus(buffer,pn,0,0)) < 0) {
    LEVEL_DATA("DS9490_setup_adapter: getstatus failed ret=%d\n", ret);
    return ret ;
  }

  LEVEL_DATA("DS9490_setup_adapter: done\n");
  return ret ;
}

static int DS9490_open( const struct parsedname * const pn, char *name ) {
    int ret ;
    usb_dev_handle * usb ;

    if(pn->in->connin.usb.usb) {
      LEVEL_DEFAULT("DS9490_open: usb.usb is NOT closed before DS9490_open() ?");
      return -ENODEV;
    }

    if ( pn->in->connin.usb.dev && (usb=usb_open(pn->in->connin.usb.dev)) ) {
      pn->in->connin.usb.usb = usb ;
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
      usb_detach_kernel_driver_np(usb,0);
#endif /* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */
      if ( (ret=usb_set_configuration(usb, 1))==0 ) {
        if( (ret=usb_claim_interface(usb, 0))==0 ) {
            if ( (ret=usb_set_altinterface(usb, 3))==0 ) {
            LEVEL_CONNECT("Opened USB DS9490 adapter at %s.\n",name);
            DS9490_setroutines( & pn->in->iroutines ) ;
            pn->in->Adapter = adapter_DS9490 ; /* OWFS assigned value */
            pn->in->adapter_name = "DS9490" ;

            ret = 0;
#if 1
	    // clear endpoints
	    ret = usb_clear_halt(usb, DS2490_EP3) ||
	      usb_clear_halt(usb, DS2490_EP2) ||
	      usb_clear_halt(usb, DS2490_EP1) ;
	    if(ret) {
	      LEVEL_DEFAULT("DS9490_open: usb_clear_halt failed ret=%d\n", ret);
	    }
#endif
	    if(!ret) ret = DS9490_setup_adapter(pn) || DS9490_overdrive(MODE_NORMAL, pn) || DS9490_level(MODE_NORMAL, pn) ;
	    if(!ret) {
	      return 0 ; /* Everything is ok */
	    }
	    LEVEL_DEFAULT("Error setting up USB DS9490 adapter at %s.\n",name);
	  } else {
	    LEVEL_CONNECT("Failed to set alt interface on USB DS9490 adapter at %s.\n",name);
	  }
	  ret = usb_release_interface( usb, 0) ;
	} else {
	  LEVEL_CONNECT("Failed to claim interface on USB DS9490 adapter at %s. ret=%d\n",name, ret);
	}
      } else {
	LEVEL_CONNECT("Failed to set configuration on USB DS9490 adapter at %s.\n",name);
      }
      usb_close( usb ) ;
      pn->in->connin.usb.usb = NULL ;
    } else {
    }
    pn->in->connin.usb.dev = NULL ; // this will force a re-scan next time
    //LEVEL_CONNECT("Failed to open USB DS9490 adapter %s\n", name);
    return -ENODEV;
}

/* When the errors stop the USB device from functioning -- close and reopen.
 * If it fails, re-scan the USB bus and search for the old adapter */
static int DS9490_reconnect( const struct parsedname * const pn ) {
    int ret ;
    STATLOCK;
    BUS_reconnect++;
    if ( !pn || !pn->in ) {
      STATUNLOCK;
      return -EIO;
    }
    pn->in->bus_reconnect++;

    /* Have to protect this function and since it's called in a very low-level
     * function. When reconnect is in progress, a new reconnect shouldn't
     * be initiated by the same thread. */
    if(pn->in->reconnect_in_progress) {
      STATUNLOCK;
      return -EIO;
    }
    pn->in->reconnect_in_progress = 1 ;
    STATUNLOCK;

    /* Have to protect usb_find_busses() and usb_find_devices() with
     * a lock since libusb could crash if 2 threads call it at the same time.
     * It's not called until DS9490_redetect_low(), but I lock here just
     * to be sure DS9490_close() and DS9490_open() get one try first. */
    RECONNECTLOCK;

    /* close and open if there were any existing USB adapter */
    DS9490_close(pn->in);
    if(!DS9490_open( pn, (pn->in->name ? pn->in->name : "?/?") )) {
      RECONNECTUNLOCK;
      STATLOCK;
      pn->in->reconnect_in_progress = 0 ;
      STATUNLOCK;
      LEVEL_DEFAULT("USB DS9490 adapter at %s reconnected\n", pn->in->name);
      return 0 ;
    }
    //LEVEL_CONNECT("Failed to reopen USB DS9490 adapter!\n");
    STATLOCK;
    BUS_reconnect_errors++;
    pn->in->bus_reconnect_errors++;
    STATUNLOCK;

    if(!DS9490_redetect_low(pn)) {
      LEVEL_DEFAULT("Found USB DS9490 adapter after USB rescan as [%s]!\n", pn->in->name);
      ret = 0;
    } else {
      ret = -EIO;
    }
    RECONNECTUNLOCK;
    STATLOCK;
    pn->in->reconnect_in_progress = 0 ;
    STATUNLOCK;
    return ret ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_detect_low( const struct parsedname * const pn ) {
    struct usb_bus *bus ;
    struct usb_device *dev ;
    int useusb = pn->in->fd ; /* fd holds the number of the adapter */
    int usbnum = 0 ;
    char name[16] ;
    unsigned char sn[8] ;
    char id[17] ;
    int ret ;
    struct parsedname pncopy;

    usb_init() ;
    usb_find_busses() ;
    usb_find_devices() ;
    for ( bus = usb_busses ; bus ; bus=bus->next ) {
        for ( dev=bus->devices ; dev ; dev=dev->next ) {
//printf("USB %s:%s is %.4X:%.4X\n",bus->dirname,dev->filename,dev->descriptor.idVendor,dev->descriptor.idProduct);
            if ( dev->descriptor.idVendor==0x04FA && dev->descriptor.idProduct==0x2490 ) {
                if ( ++usbnum < useusb ) {
                    LEVEL_CONNECT("USB DS9490 adapter at %s/%s passed over.\n",bus->dirname,dev->filename)
                } else {
                    strcpy(name, bus->dirname) ;
                    strcat(name, "/") ;
                    strcat(name, dev->filename) ;
                    pn->in->connin.usb.dev = dev ;
                    if ( DS9490_open( pn, name ) ) {
                        if(pn->in->name) free(pn->in->name) ;
                        pn->in->name = strdup("-1/-1") ;
                        pn->in->connin.usb.dev = NULL ;
                        return -EIO ;
                    }

                    // clear it just in case nothing is found
                    memset(pn->in->connin.usb.ds1420_address, 0, 8);
                    memset(sn, 0, 8);

                    /* We are looking for devices in the root (not the branch
                        * pn eventually points to */
                    memcpy(&pncopy, pn, sizeof(struct parsedname));
                    pncopy.dev = NULL;
                    pncopy.pathlength = 0;
                    
                    /* Do a quick directory listing and find the DS1420 id */
#if 0
                    (ret=BUS_select(&pncopy)) || (ret=BUS_first(sn,&pncopy)) ;
#else
                    if(!(ret = BUS_select(&pncopy))) {
                      if((ret = BUS_first(sn,&pncopy))) {
                        LEVEL_DATA("BUS_first failed during connect [%s]\n", name);
                      }
                    } else {
                      LEVEL_DATA("BUS_select failed during connect [%s]\n", name);
                    }
#endif
                    while (ret==0) {
                      if(error_level > 3) {
                        bytes2string(id, sn, 8) ;
                        id[16] = 0;
                        LEVEL_DATA("Found device [%s] on adapter [%s]\n", id, name);
                      }
#if 0
                        /* This test is actually made in DS9490_next_both() */
                        if(!pn->in->connin.usb.ds1420_address[0] &&
                        ((sn[0] == 0x81) || (sn[0] == 0x01))) {
                        /* accept the first ds1420 here and set as unique id */
                        memcpy(pn->in->connin.usb.ds1420_address, sn, 8);
                        break;
                      }
#endif

                    /* Unique id is set... no use to loop any more */
                    if(pn->in->connin.usb.ds1420_address[0]) break;

                      (ret=BUS_select(&pncopy)) || (ret=BUS_next(sn,&pncopy)) ;
                    }
                    if(!pn->in->connin.usb.ds1420_address[0]) {
                      /* There are still no unique id, set it to the last device if the
                       * search above ended normally.  Eg. with -ENODEV */
                      if((ret == -ENODEV) && sn[0]) {
                        bytes2string(id, sn, 8);
                        id[16] = '\000';
                        LEVEL_DATA("No DS1420 found, so use device [%s] on [%s] instead\n", id, name);
                        memcpy(pn->in->connin.usb.ds1420_address, sn, 8);
                        ret = 0;
                      }
                    }
                    if(ret == -ENODEV) ret = 0 ;

                    if(ret == 0) {
                        bytes2string(id, pn->in->connin.usb.ds1420_address, 8);
                        id[16] = '\000';
                        LEVEL_DEFAULT("Set DS9490 [%s] unique id to adapter [%s]", id, name);
                    } else {
                        LEVEL_CONNECT("Couldn't find ds1420 chip on this adapter [%s]\n", name);
                    }
                    if(pn->in->name) free(pn->in->name);
                    pn->in->name = strdup(name);
                    return 0;
                }
            }
        }
    }
    //LEVEL_DEFAULT("No available USB DS9490 adapter found\n")
    return -ENODEV ;
}

static int usbdevice_in_use(char *name) {
    struct connection_in *in = indevice;
    while(in) {
        if ( (in->busmode == bus_usb) && (in->name) && (!strcmp(in->name, name)) ) return 1; // It seems to be in use already
        in = in->next;
    }
    return 0; // not found in the current indevice-list
}


/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_redetect_low( const struct parsedname * const pn ) {
    struct usb_bus *bus ;
    struct usb_device *dev ;
    unsigned char sn[8] ;
    int ret;
    char name[16];
    char id2[17];
    struct parsedname pncopy;

    //LEVEL_CONNECT("DS9490_redetect_low: name=%s", pn->in->name);

    /*
     * I don't think we need to call usb_init() or usb_find_busses() here.
     * They are already called once in DS9490_detect_low().
     * usb_init() is more like a os_init() function and there are probably
     * no new USB-busses added after rebooting kernel... or?
     * It doesn't seem to leak any memory when calling them, so they are
     * still called.
     */
    usb_init() ;
    usb_find_busses() ;
    usb_find_devices() ;
    for ( bus = usb_busses ; bus ; bus=bus->next ) {
        for ( dev=bus->devices ; dev ; dev=dev->next ) {
            if ( !(dev->descriptor.idVendor==0x04FA && dev->descriptor.idProduct==0x2490) ) continue;

            strcpy(name,bus->dirname) ;
            strcat(name,"/") ;
            strcat(name,dev->filename) ;
            //LEVEL_DEFAULT("Found %s, searching for %s\n",name,pn->in->name);

            if(usbdevice_in_use(name)) continue;
            pn->in->connin.usb.dev = dev ;

            if ( DS9490_open( pn, name ) ) {
                LEVEL_DEFAULT("Cant open USB adapter [%s], Find next...\n", name);
                pn->in->connin.usb.dev = NULL ;
                continue;
            }
            // pn->in->connin.usb.usb is set in DS9490_open().


            bytes2string(id2, pn->in->connin.usb.ds1420_address, 8) ;
            id2[16] = 0;

            memset(sn, 0, 8); // clear it just in case nothing is found
            /* Do a quick directory listing and find the DS1420 id */

            /* We are looking for devices in the root (not the branch
                * pn eventually points to */
            memcpy(&pncopy, pn, sizeof(struct parsedname));
            pncopy.dev = NULL;
            pncopy.pathlength = 0;

#if 0
            (ret=BUS_select(&pncopy)) || (ret=BUS_first(sn,&pncopy)) ;
#else
            if( (ret = BUS_select(&pncopy)) ) {
                LEVEL_DATA("BUS_select failed during reconnect [%s]\n", name);
            } else if ( (ret = BUS_first(sn,&pncopy)) ) {
                LEVEL_DATA("BUS_first failed during reconnect [%s]\n", name);
            }
#endif
            while (ret==0) {
                if(error_level > 3) {
                    char id[17];
                    bytes2string(id, sn, 8) ;
                    id[16] = 0;
                    LEVEL_DATA("Found device [%s] on adapter [%s] (want: %s)\n", id, name, id2);
                }
#if 0
                /* This test is actually made in DS9490_next_both() */
                if(!pn->in->connin.usb.ds1420_address[0] &&
                ((sn[0] == 0x81) || (sn[0] == 0x01))) {
                    memcpy(pn->in->connin.usb.ds1420_address, sn, 8);
                    break;
                }
#endif
                /* Unique id is set and match... no use to loop any more */
                if(!memcmp(sn, pn->in->connin.usb.ds1420_address, 8)) break;

                (ret=BUS_select(&pncopy)) || (ret=BUS_next(sn,&pncopy)) ;
            }

            if(!pn->in->connin.usb.ds1420_address[0]) {
                /* There are still no unique id, set it to the last device if the
                * search above ended normally.  Eg. with -ENODEV */
                if((ret == -ENODEV) && sn[0]) {
                    memcpy(pn->in->connin.usb.ds1420_address, sn, 8);
                    ret = 0;
                }
            }
            if(ret == 0) {
                // Yeah... We found the adapter again...
                if(pn->in->name) free(pn->in->name);
                pn->in->name = strdup(name);
                return 0;
            }
            // Couldn't find correct ds1420 chip on this adapter
            LEVEL_CONNECT("Couldn't find correct ds1420 chip on this adapter [%s] (want: %s)\n", name, id2);
            DS9490_close(pn->in);
            pn->in->connin.usb.dev = NULL;
        }
    }
    //LEVEL_CONNECT("No available USB DS9490 adapter found\n");
    return -ENODEV ;
}

void DS9490_close(struct connection_in * in) {
    int ret;
    usb_dev_handle * usb = in->connin.usb.usb ;

    if ( usb ) {
        ret = usb_release_interface( usb, 0) ;
        if(ret) in->connin.usb.dev = NULL ; // force a re-scan
        if(ret) LEVEL_CONNECT("Release interface failed ret=%d\n", ret);
        
        /* It might already be closed? (returning -ENODEV)
            * I have seen problem with calling usb_close() twice, so we
            * might perhaps skip it if usb_release_interface() fails */
        ret = usb_close( usb ) ;
        if(ret) in->connin.usb.dev = NULL ; // force a re-scan
        if(ret) LEVEL_CONNECT("usb_close() failed ret=%d\n", ret);
            in->connin.usb.usb = NULL ;
        LEVEL_CONNECT("Closed USB DS9490 adapter at %s. ret=%d\n",in->name, ret);
    }
}

/* DS9490_getstatus()
   return -1          if Short detected on 1-wire bus
   return -ETIMEDOUT  on timeout (will require DS9490_close())
   return -EIO        on read errors (will require DS9490_close())
   otherwise return number of status bytes in buffer
*/

static int DS9490_getstatus(unsigned char * const buffer, const struct parsedname * const pn, int readlen, int wait_for_idle) {
    int ret , loops = 0 ;
    int i ;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;

    //printf("DS9490_getstatus: readlen=%d wfi=%d\n", readlen, wait_for_idle);
    memset(buffer, 0, 32) ; // should not be needed
    do {
#ifdef HAVE_USB_INTERRUPT_READ
        // Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
        if ( (ret=usb_interrupt_read(usb,DS2490_EP1,buffer,(size_t)32,TIMEOUT_USB)) < 0 ) {
#else
        if ( (ret=usb_bulk_read(usb,DS2490_EP1,buffer,(size_t)32,TIMEOUT_USB)) < 0 ) { 
#endif
            STAT_ADD1(DS9490_wait_errors);
            LEVEL_DATA("DS9490_getstatus: error reading ret=%d\n", ret);
            return -EIO ;
        }
        if(!wait_for_idle) break ;

        if ( !(buffer[8]&STATUSFLAGS_IDLE) ) {  // wait until not idle
            if (!readlen) break ; // ignore if bytes are available
        }
        if (readlen) {
            if (buffer[13] >= readlen) {   // (ReadBufferStatus)
                // we have enough bytes to read now!
                break ;
            }
        }

        if(ret > 16) break ; // we have some status byte to examine
        if(++loops > 100) {
            LEVEL_DATA("DS9490_getstatus(): never got idle\n");
            return -ETIMEDOUT ;  // adapter never got idle
        }
        /* Since result seem to be on the usb bus very quick, I sleep
            * sleep 0.1ms or something like that instead... It seems like
            * result is there after 0.2-0.3ms
            */
        UT_delay_us(100);
    } while(1);

#if 0
    printf("DS9490_getstatus: read %d bytes\n", ret);
    printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
            buffer[0], buffer[1], buffer[2], buffer[3],
            buffer[4], buffer[5], buffer[6], buffer[7],
            buffer[8], buffer[9], buffer[10], buffer[11],
            buffer[12], buffer[13], buffer[14], buffer[15]
            );
#endif

    if(ret < 16) return -EIO ;   // incomplete packet??

    for(i=16; i<ret; i++) {
      //printf("status=%X\n", buffer[i]);
      if(buffer[i] & COMMCMDERRORRESULT_SH) { // short detected
        //LEVEL_DEFAULT("Short detected on USB DS9490 adapter at %s.\n",pn->in->name)
        LEVEL_DATA("DS9490_getstatus(): short detected\n");
        return -1;
      }
    }
    return (ret - 16) ;  // return number of status bytes in buffer
}

static int DS9490_testoverdrive(const struct parsedname * const pn) {
    unsigned char buffer[32] ;
    unsigned char r[8] ;
    unsigned char p = 0x69 ;
    int i, ret ;
    
    //printf("DS9490_testoverdrive:\n");
    
    // make sure normal level
    if((ret = DS9490_level(MODE_NORMAL, pn)) < 0) return ret ;
    
    // force to normal communication speed
    if((ret = DS9490_overdrive(MODE_NORMAL, pn)) < 0) return ret ;
    
    if((ret = DS9490_reset(pn)) < 0) return ret ;
    
    if (!DS9490_sendback_data(&p,buffer,1, pn) && (p==buffer[0])) {  // match command 0x69
        if(!DS9490_overdrive(MODE_OVERDRIVE, pn)) {
            for (i=0; i<8; i++) buffer[i] = pn->sn[i] ;
#if 0
            for (i=0; i<8; i++) {
                printf(" %02X", pn->sn[i]);
            }
            printf("\n");
#endif
            if (!DS9490_sendback_data(buffer, r, 8, pn)) {
                for(i=0; i<8; i++) {
                    if(r[i] != pn->sn[i]) break ;
                }
                if(i==8) {
                    if((i = DS9490_getstatus(buffer,pn,0,0)) < 0) {
                        //printf("DS9490_testoverdrive: failed to get status\n");
                    } else {
                        //printf("DS9490_testoverdrive: i=%d status=%X\n", i, (i>0 ? buffer[16] : 0));
                        return 0; // ok
                    }
                }
            }
        }
    }
    //printf("DS9490_testoverdrive failed\n");
    DS9490_overdrive(MODE_NORMAL, pn);
    return -EINVAL ;
}

static int DS9490_overdrive( const unsigned int overdrive, const struct parsedname * const pn ) {
    int ret ;
    int speed;
    unsigned char sp = 0x3C;
    unsigned char resp ;
    int i ;
    
    // set speed
    if(overdrive & MODE_OVERDRIVE) {
        speed = ONEWIREBUSSPEED_OVERDRIVE ;
        //printf("set overdrive speed\n");
        
        if(!pn->in->use_overdrive_speed) return 0 ; // adapter doesn't use overdrive
        if(!(pn->in->USpeed & MODE_OVERDRIVE)) {
            // we need to change speed to overdrive
            for(i=0; i<3; i++) {
                if ((ret=DS9490_reset(pn)) < 0) continue ;
                if ((DS9490_sendback_data(&sp,&resp,1, pn)<0) || (sp!=resp)) {
                    //printf("error sending 0x3C\n");
                    continue ;
                }
                if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_1WIRE_SPEED, speed, NULL, 0, TIMEOUT_USB )) == 0) break ;
            }
            if(i==3) {
                //printf("error after 3 tries\n");
                return -EINVAL ;
            }
        }
        pn->in->USpeed = MODE_OVERDRIVE ;
    } else {
        speed = ONEWIREBUSSPEED_FLEXIBLE ;
        if(pn->in->connin.usb.usb) {
            /* Have to make sure usb isn't closed after last reconnect */
            if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_1WIRE_SPEED, speed, NULL, 0, TIMEOUT_USB )) < 0) return ret ;
        }
        pn->in->USpeed = MODE_NORMAL ;
    }
    return 0 ;
}

/* Reset adapter and detect devices on the bus */
static int DS9490_reset( const struct parsedname * const pn ) {
    int i, ret ;
    unsigned char val ;
    unsigned char buffer[32] ;
    //printf("9490RESET\n");
    //printf("DS9490_reset() index=%d pn->in->Adapter=%d %s\n", pn->in->index, pn->in->Adapter, pn->in->adapter_name);

    if(!pn->in->connin.usb.usb || !pn->in->connin.usb.dev) {
        /* last reconnect probably failed and usb-handle is closed.
        * Try to reconnect again */
        //if(!pn->in->connin.usb.usb) { LEVEL_DATA("DS9490_reset: usb.usb is null\n"); }
        //if(!pn->in->connin.usb.dev) { LEVEL_DATA("DS9490_reset: usb.dev is null\n"); }
        if((ret = DS9490_reconnect(pn))) {
            //LEVEL_DEFAULT("DS9490_reset: reconnect ret=%d\n", ret);
            return ret;
        }
    }

    memset(buffer, 0, 32); 
 
    if((ret=DS9490_level(MODE_NORMAL, pn)) < 0) {
        LEVEL_DATA("DS9490_reset: level failed ret=%d\n", ret);
        STAT_ADD1(DS9490_reset_errors);
        return ret ;
    }

    // force normal speed if not using overdrive anymore
    if (!pn->in->use_overdrive_speed && (pn->in->USpeed & MODE_OVERDRIVE)) {
        if((ret=DS9490_overdrive(MODE_NORMAL, pn)) < 0) return ret ;
    }

    if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,COMM_CMD,
            COMM_1_WIRE_RESET | COMM_F | COMM_IM | COMM_SE,
            ((pn->in->USpeed & MODE_OVERDRIVE)?ONEWIREBUSSPEED_OVERDRIVE:ONEWIREBUSSPEED_FLEXIBLE),
            NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1(DS9490_reset_errors);
        LEVEL_DATA("DS9490_reset: error sending reset ret=%d\n", ret);
        return -EIO ;  // fatal error... probably closed usb-handle
    }

    if(ds2404_alarm_compliance && !(pn->in->USpeed & MODE_OVERDRIVE)) {
      // extra delay for alarming DS1994/DS2404 complience
      UT_delay(5);
    }

    if((ret=DS9490_getstatus(buffer,pn,0,0)) < 0) {
        STAT_ADD1(DS9490_reset_errors);
        if(ret == -1) {
            /* Short detected, but otherwise no bigger "problem"?
            * Make sure 1-wires won't be scanned */
            pn->si->AnyDevices = 0;
            LEVEL_DATA("DS9490_reset: short detected\n", ret);
            return 0;
        }
        LEVEL_DATA("DS9490_reset: getstatus failed ret=%d\n", ret);
        return ret ;
    }

//    USBpowered = (buffer[8]&STATUSFLAGS_PMOD) == STATUSFLAGS_PMOD ;
    pn->si->AnyDevices = 1;
    for(i=0; i<ret; i++) {
        val = buffer[16+i];
        //printf("Status bytes: %X\n", val);
        if(val != ONEWIREDEVICEDETECT) {
            // check for NRS bit (0x01)
            if(val & COMMCMDERRORRESULT_NRS) {
                // empty bus detected, no presence pulse detected
                pn->si->AnyDevices = 0;
                LEVEL_DATA("DS9490_reset: no presense pulse detected\n");
            }
        }
    }

    LEVEL_DATA("DS9490_reset: ok\n");
    return 0 ;
}

static int DS9490_read( unsigned char * const buf, const size_t size, const struct parsedname * const pn) {
    int ret;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    //printf("DS9490_read\n");
    if ((ret=usb_bulk_read(usb,DS2490_EP3,buf,(int)size,TIMEOUT_USB )) > 0) return ret ;
    LEVEL_DATA("DS9490_read: failed ret=%d\n", ret);
    usb_clear_halt(usb,DS2490_EP3) ;
    return ret ;
}

static int DS9490_write( const unsigned char * const buf, const size_t size, const struct parsedname * const pn) {
    int ret;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    //printf("DS9490_write\n");
    if ((ret=usb_bulk_write(usb,DS2490_EP2,buf,(int)size,TIMEOUT_USB )) > 0) return ret ;
    LEVEL_DATA("DS9490_write: failed ret=%d\n", ret);
    usb_clear_halt(usb,DS2490_EP2) ;
    return ret ;
}

static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len,const struct parsedname * const pn ) {
    int ret = 0 ;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    unsigned char buffer[32] ;

    if ( len > USB_FIFO_EACH ) {
//printf("USBsendback splitting\n");
        return DS9490_sendback_data(data,resp,USB_FIFO_EACH,pn)
            || DS9490_sendback_data(&data[USB_FIFO_EACH],&resp[USB_FIFO_EACH],len-USB_FIFO_EACH,pn) ;
    }

    if ( (ret=DS9490_write(data, (size_t)len, pn)) < len ) {
        //printf("USBsendback bulk write problem ret=%d\n", ret);
        STAT_ADD1(DS9490_sendback_data_errors);
        return ret ;
    }

    if ( ((ret=usb_control_msg(usb,0x40,COMM_CMD,COMM_BLOCK_IO | COMM_IM | COMM_F, len, NULL, 0, TIMEOUT_USB )) < 0)
        ||
        ((ret = DS9490_getstatus(buffer,pn,len,1)) < 0) // wait for len bytes
        ) {
        //printf("USBsendback control problem ret=%d\n", ret);
        STAT_ADD1(DS9490_sendback_data_errors);
        return ret ;
    }

    if ( (ret=DS9490_read(resp, (size_t)len, pn)) < 0 ) {
        //printf("USBsendback bulk read problem ret=%d\n", ret);
        STAT_ADD1(DS9490_sendback_data_errors);
        return ret ;
    }
    return 0 ;
}

static int next_both_errors( const struct parsedname * const pn, int ret ) {
    STAT_ADD1(DS9490_next_both_errors);

    /* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
    if(ret >= -1) {
      LEVEL_DATA("next_both_erorrs: return %d\n", ret) ;
      return ret;
    }


    ret = BUS_reconnect( pn ) ;
    LEVEL_DATA("next_both_erorrs: reconnect = %d\n", ret) ;
    return ret ;
}   


/*
 * return 0       if success
 * return -ENODEV if no more devices
 * return -ENOENT if no devices at all
 * return -EIO    on errors
 */

static int DS9490_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
    unsigned char buffer[32] ;
    unsigned char * cb = pn->in->combuffer ;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    struct stateinfo * si = pn->si ;
    struct timeval tv ;
    struct timezone tz;
    time_t endtime, now ;
    int ret ;
    int i ;
    size_t buflen ;

    //printf("DS9490_next_both: Anydevices=%d LastDevice=%d\n",si->AnyDevices, si->LastDevice);
    printf("DS9490_next_both SN in: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
    // if the last call was not the last one
    if ( !si->AnyDevices ) si->LastDevice = 1 ;

    /* DS1994/DS2404 might need an extra reset */
    if (si->ExtraReset) {
        if(DS9490_reset(pn) < 0) si->LastDevice = 1 ;
        si->ExtraReset = 0;
    }

    if ( si->LastDevice ) return -ENODEV ;

    /** Play LastDescrepancy games with bitstream */
    memcpy( cb,serialnumber,8) ; /* set bufferto zeros */
    if ( si->LastDiscrepancy > -1 ) UT_setbit(cb,si->LastDiscrepancy,1) ;
    /* This could be more efficiently done than bit-setting, but probably wouldnt make a difference */
    for ( i=si->LastDiscrepancy+1;i<64;i++) UT_setbit(cb,i,0) ;
    printf("DS9490_next_both EP2: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",cb[0],cb[1],cb[2],cb[3],cb[4],cb[5],cb[6],cb[7]) ;

    //printf("USBnextboth\n");
    buflen = 8;
    if ( (ret=DS9490_write(cb, buflen, pn)) < 8 ) {
        LEVEL_DATA("USBnextboth bulk write problem = %d\n",ret);
        next_both_errors(pn, -EIO);
        return -EIO;
    }

    //printf("USBnextboth\n");

    // COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS
    // 0xF4 + +0x1 + 0x8 + 0x800 + 0x4000 = 0x48FD
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x48FD, 0x0100|search, NULL, 0, TIMEOUT_USB ))<0 ) {
        LEVEL_DATA("USBnextboth control problem ret=%d\n", ret);
        next_both_errors(pn, -EIO);
        return -EIO;
    }

    /* Wait for status max 200ms */
    if(gettimeofday(&tv, &tz)<0) return -1;
    endtime = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 + 500;
    //now = 0 ;
    do {
        // just get first status packet without waiting for not idle
        if((ret = DS9490_getstatus(buffer,pn,0,0)) < 0) {
            LEVEL_DATA("USBnextboth getstatus returned ret=%d\n", ret);
            break ;
        }
        for(i=0; i<ret; i++) {
            unsigned char val = buffer[16+i];
            //printf("Status bytes: %X\n", val);
            if(val != ONEWIREDEVICEDETECT) {
                // If other status then ONEWIREDEVICEDETECT we have failed
                LEVEL_DATA("USBnextboth status[%d]=0x%02X (!=%d) ret=%d\n", i, val, ONEWIREDEVICEDETECT, ret);
                // check for NRS bit (0x01)
                if(val & COMMCMDERRORRESULT_NRS) {
                    // empty bus detected, no presence pulse detected
                    LEVEL_DATA("USBnextboth: no presense pulse detected ??\n");
                }
                break;
            }
        }
      
        if(gettimeofday(&tv, &tz)<0) return -1;
        now = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 ;
    } while(((buffer[8]&STATUSFLAGS_IDLE) == 0) && (endtime > now));

    if((buffer[8]&STATUSFLAGS_IDLE) == 0) {
        LEVEL_DATA("USBnextboth: still not idle\n") ;
        next_both_errors(pn, -EIO) ;
        return -EIO ;
    }
    printf("NEXT: buffer[13]=%d\n",(int)buffer[13]) ;
    if(buffer[13] == 0) {  // (ReadBufferStatus)
        /* Nothing found on the bus. Have to return something != 0 to avoid
        * getting stuck in loop in FS_realdir() and FS_alarmdir()
        * which ends when ret!=0 */
        LEVEL_DATA("USBnextboth: ReadBufferstatus == 0\n");
        return -ENOENT;
    }

    //buflen = 16 ;  // try read 16 bytes
    buflen=buffer[13] ;
    printf("USBnextboth len=%d (available=%d)\n", buflen, buffer[13]);
    if ( (ret=DS9490_read(cb,buflen,pn)) <= 0 ) {
        LEVEL_DATA("USBnextboth: bulk read problem ret=%d\n", ret);
        next_both_errors(pn, -EIO);
        return -EIO ;
    }
    
    memcpy(serialnumber,cb,8) ;
    printf("DS9490_next_both SN out: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
    si->LastDevice = (ret==8) ;
    printf("DS9490_next_both lastdevice=%d bytes=%d\n",si->LastDevice,ret) ;
    
    for ( i=63 ; i>=0 ; i-- ) {
        if ( UT_getbit(cb,i+64) && (UT_getbit(cb,i)==0) ) {
            si->LastDiscrepancy = i ;
            //printf("DS9490_next_both lastdiscrepancy=%d\n",si->LastDiscrepancy) ;
            break ;
        }
    }

    /* test for CRC error */
    if( CRC8(serialnumber,8) ) {
        /* this should not cause a reconnect, just some "minor" error */
        LEVEL_DATA("USBnextboth: CRC error\n");
        next_both_errors(pn, 0);
        return -EIO;
    }

    /* test for special device families */
    switch ( serialnumber[0] ) {
        case 0x00:
            LEVEL_DATA("USBnextboth: NULL family found\n");
            next_both_errors(pn, 0);
            return -EIO;
        case 0x01:
        case 0x81:
            if ( pn->in->connin.usb.ds1420_address[0] == 0 ) {
                char tmp[17];
                bytes2string(tmp, serialnumber, 8);
                tmp[16] = '\000';
                LEVEL_DEFAULT("Found a DS1420 device [%s]\n", tmp);
                memcpy(pn->in->connin.usb.ds1420_address, serialnumber, 8);
            }
            break ;
        case 0x40:
            /* We found a DS1994/DS2404 which require longer delays */
            ds2404_alarm_compliance = 1 ;
            break ;
    }
    
    return 0 ;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'byte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
// Delay delay msec and return to normal
//
/* Returns 0=good
   bad = -EIO
 */
static int DS9490_PowerByte(const unsigned char byte, const unsigned int delay,const struct parsedname * const pn) {
    unsigned char buffer[32] ;
    unsigned char resp ;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    unsigned char dly = 1 + ( (unsigned char) (delay>>4) ) ;
    int ret ;

    //printf("9490 Powerbyte\n") ;

#if 1
    /* Send the byte */
    if ( (ret=DS9490_sendback_data(&byte, &resp , 1,pn)) ) {
        STAT_ADD1( DS9490_PowerByte_errors ) ;
        return ret ;
    }
    //printf("9490 Powerbyte in=%.2X out %.2X\n",byte,resp) ;
    if ( byte != resp ) {
        STAT_ADD1( DS9490_PowerByte_errors );
        return -EIO ;
    }
    /** Pulse duration */
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0213, 0x0000|dly, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1( DS9490_PowerByte_errors );
        return ret ;
    }
    /** Pulse start */
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0431, 0x0000, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1( DS9490_PowerByte_errors );
        return ret ;
    }
    /** Delay */
    UT_delay( delay ) ;
    /** wait */
    if ( ((ret=DS9490_getstatus(buffer,pn,0,1)) < 0) ) {
        STAT_ADD1(DS9490_PowerByte_errors);
        return ret ;
    }
#else
    // set the strong pullup
    if ( (ret=usb_control_msg(usb,0x40,MODE_CMD,MOD_PULSE_EN, ENABLEPULSE_SPUE, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1(DS9490_PowerByte_errors);
        //printf("Powerbyte err1\n");
        return ret ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,COMM_BYTE_IO | COMM_IM | COMM_SPU, byte & 0xFF, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1(DS9490_PowerByte_errors);
        //printf("Powerbyte err2\n");
        return ret ;
    }

    pn->in->ULevel = MODE_STRONG5;    

    /** Delay */
    UT_delay( delay ) ;

    /** wait */
    if ( ((ret=DS9490_getstatus(buffer,pn,0,1)) < 0) ) {
        STAT_ADD1(DS9490_PowerByte_errors);
        //printf("Powerbyte err3\n");
        return ret ;
    }
#endif
    return 0 ;
}

static int DS9490_HaltPulse(const struct parsedname * const pn) {
    unsigned char buffer[32] ;
    struct timeval tv ;
    struct timezone tz;
    time_t endtime, now ;
    int ret ;
    
    //printf("DS9490_HaltPulse\n");
    
    if(gettimeofday(&tv, &tz)<0) return -1;
    endtime = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 + 300;
    
    do {
        if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,CONTROL_CMD,CTL_HALT_EXE_IDLE, 0, NULL, 0, TIMEOUT_USB ))<0 ) {
        //printf("DS9490_HaltPulse: err1\n");
        break ;
        }
        if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,CONTROL_CMD,CTL_RESUME_EXE, 0, NULL, 0, TIMEOUT_USB ))<0 ) {
        //printf("DS9490_HaltPulse: err2\n");
        break ;
        }
        if((ret=DS9490_getstatus(buffer,pn,0,0)) < 0) {
        //printf("DS9490_HaltPulse: err3\n");
        break;
        }
    
        // check the SPU flag
        if(!(buffer[8] & STATUSFLAGS_SPUA)) {
            //printf("DS9490_HaltPulse: SPU not set\n");
            if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_PULSE_EN, 0, NULL, 0, TIMEOUT_USB ))<0 ) {
                //printf("DS9490_HaltPulse: err4\n");
                break ;
            }
            //printf("DS9490_HaltPulse: ok\n");
            return 0 ;
        }
        if(gettimeofday(&tv, &tz)<0) return -1;
        now = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 ;
    } while(endtime > now);
    
    //printf("DS9490_HaltPulse: timeout\n");
    return -1;
}


/* Set the 1-Wire Net line level.  The values for new_level are
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:    0 GOOD, !0 Error
   Actually very simple for passive adapter
*/
/* return 0=good
  -EIO not supported
 */
static int DS9490_level(int new_level,const struct parsedname * const pn) {
    int ret ;
    int lev ;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;

    if (new_level == pn->in->ULevel) {     // check if need to change level
        return 0 ;
    }

    if(!usb) return -EIO;

    //printf("DS9490_level %d (old = %d)\n", new_level, pn->in->ULevel);
    switch (new_level) {
    case MODE_NORMAL:
        if(pn->in->ULevel==MODE_STRONG5) {
            if(DS9490_HaltPulse(pn)==0) {
                pn->in->ULevel = new_level ;
                return 0 ;
            }
        }
        return 0;
    case MODE_STRONG5:
        lev = ENABLEPULSE_SPUE ;
        break ;
    case MODE_PROGRAM:
        //lev = ENABLEPULSE_PRGE ;
        // Don't support Program pulse right now
        return -EIO ;
    case MODE_BREAK:
    default:
        return 1 ;
    }

    // set pullup to strong5 or program
    if ( (ret=usb_control_msg(usb,0x40,MODE_CMD,MOD_PULSE_EN, lev, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1(DS9490_level_errors);
        return ret ;
    }

    // set the strong pullup duration to infinite
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,COMM_PULSE | COMM_IM, 0, NULL, 0, TIMEOUT_USB ))<0 ) {
        STAT_ADD1(DS9490_level_errors);
        return ret ;
    }
    pn->in->ULevel = new_level ;
    return 0 ;
}

/* The USB device can't actually deliver the EPROM programming voltage */
static int DS9490_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -ENOPROTOOPT ;
}

#endif /* OW_USB */
