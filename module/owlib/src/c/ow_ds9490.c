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

/* All the rest of the code sees is the DS9490_detect routine and the iroutine structure */

static int DS9490_reset( const struct parsedname * const pn ) ;
static int DS9490wait(unsigned char * const buffer,const struct parsedname * const pn) ;
static int DS9490_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len,const struct parsedname * const pn ) ;
static int DS9490_level(int new_level,const struct parsedname * const pn) ;
static void DS9490_setroutines( struct interface_routines * const f ) ;
static int DS9490_select_low(const struct parsedname * const pn) ;
static int DS9490_detect_low( const struct parsedname * const pn ) ;
static int DS9490_PowerByte(const unsigned char byte, const unsigned int delay,const struct parsedname * const pn) ;
static int DS9490_ProgramPulse( const struct parsedname * const pn) ;

/* Device-specific routines */
static void DS9490_setroutines( struct interface_routines * const f ) {
    // stay with BadAdapter default
    //    f->write = NULL ;
    // stay with BadAdapter default
    //    f->read  = NULL ;
    f->reset = DS9490_reset ;
    f->next_both = DS9490_next_both ;
    f->level = DS9490_level ;
    f->PowerByte = DS9490_PowerByte ;
    f->ProgramPulse = DS9490_ProgramPulse ;
    f->sendback_data = DS9490_sendback_data ;
    f->select        = BUS_select_low ;
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

    ret = DS9490_detect_low(&pn) ;
    if (ret==0) in->busmode = bus_usb ;
    return ret ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_detect_low( const struct parsedname * const pn ) {
    struct usb_bus *bus ;
    struct usb_device *dev ;
    int useusb = pn->in->fd ; /* fd holds the number of the adapter */
    int usbnum = 0 ;
    int ret ;

    usb_init() ;
    usb_find_busses() ;
    usb_find_devices() ;
//    for ( bus = usb_get_busses() ; bus ; bus=bus->next ) {
    for ( bus = usb_busses ; bus ; bus=bus->next ) {
        for ( dev=bus->devices ; dev ; dev=dev->next ) {
//printf("USB %s:%s is %.4X:%.4X\n",bus->dirname,dev->filename,dev->descriptor.idVendor,dev->descriptor.idProduct);
            if ( dev->descriptor.idVendor==0x04FA && dev->descriptor.idProduct==0x2490 ) {
                if ( ++usbnum < useusb ) {
                    syslog(LOG_INFO,"USB DS9490 adapter at %s/%s passed over.\n",bus->dirname,dev->filename) ;
                } else if ( (pn->in->name=(char *)malloc(strlen(bus->dirname)+strlen(dev->filename)+2) ) == NULL ) {
                    return -ENOMEM ;
                } else {
                    strcpy(pn->in->name,bus->dirname) ;
                    strcat(pn->in->name,"/") ;
                    strcat(pn->in->name,dev->filename) ;
                    if ( (pn->in->usb=usb_open(dev)) ) {
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
                        usb_detach_kernel_driver_np(pn->in->usb,0);
#endif /* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */
                        if ( usb_set_configuration( pn->in->usb, 1 )==0 && usb_claim_interface( pn->in->usb, 0)==0 ) {
                            if ( usb_set_altinterface( pn->in->usb, 3)==0 ) {
                                syslog(LOG_INFO,"Opened USB DS9490 adapter at %s.\n",pn->in->name) ;
                                DS9490_setroutines( & pn->in->iroutines ) ;
                                pn->in->Adapter = adapter_DS9490 ; /* OWFS assigned value */
                                pn->in->adapter_name = "DS9490" ;
                                ret = DS9490_reset(pn) ;
                                if (ret) {
                                    syslog(LOG_INFO,"Couldn\'t RESET 2490.\n") ;
                                } else {
                                    unsigned char buffer[32] ;
                                    ret = usb_control_msg(pn->in->usb,0x40,CONTROL_CMD,CTL_RESET_DEVICE, 0x0000, NULL, 0, TIMEOUT_USB )<0;
                                    ret = ret || DS9490wait(buffer,pn) ;
                                    syslog(LOG_INFO,"Successful setup (reset=%d) USB DS9490 adapter at %s.\n",ret,pn->in->name) ;
                                }
                                return ret ;
                            } else {
                                syslog(LOG_INFO,"Failed to configure alt interface on USB DS9490 adapter at %s.\n",pn->in->name) ;
                            }
                            usb_release_interface( pn->in->usb, 0) ;
                        } else {
                            syslog(LOG_INFO,"Failed to configure/claim interface on USB DS9490 adapter at %s.\n",pn->in->name) ;
                        }
                        usb_close( pn->in->usb ) ;
                        pn->in->usb = 0 ;
                    } else {
                        syslog(LOG_INFO,"Failed to open USB DS9490 adapter at %s.\n",pn->in->name) ;
                    }
                    if ( pn->in->name ) free(pn->in->name) ;
                    pn->in->name = NULL ;
                    return -EIO ;
                }
            }
        }
    }
    syslog(LOG_INFO,"No available USB DS9490 adapter found\n") ;
    return -ENODEV ;
}

void DS9490_close(struct connection_in * in) {
    if ( in->usb ) {
        usb_release_interface( in->usb, 0) ;
        usb_close( in->usb ) ;
        in->usb = NULL ;
    }
    syslog(LOG_INFO,"Closed USB DS9490 adapter at %s.\n",in->name) ;
}

static int DS9490wait(unsigned char * const buffer,const struct parsedname * const pn) {
    int ret ;
    do {
#ifdef HAVE_USB_INTERRUPT_READ
        // Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
        if ( (ret=usb_interrupt_read(pn->in->usb,DS2490_EP1,buffer,32,TIMEOUT_USB)) < 0 ) {
          STATLOCK
          DS9490_wait_errors++;
          STATUNLOCK
	  return ret ;
	}
#else
        if ( (ret=usb_bulk_read(pn->in->usb,DS2490_EP1,buffer,32,TIMEOUT_USB)) < 0 ) {
          STATLOCK
          DS9490_wait_errors++;
          STATUNLOCK
	  return ret ;
	}
#endif
    } while ( !(buffer[8]&0x20) ) ;
//{
//int i ;
//printf ("USBwait return buffer:\n") ;
//for( i=0;i<8;++i) printf("%.2X: %.2X  |  %.2X: %.2X  ||  %.2X: %.2X  |  %.2X: %.2X\n",i,buffer[i],i+8,buffer[i+8],i+16,buffer[i+16],i+24,buffer[i+24]);
//}
    return 0 ;
}

    /* Reset the bus */
static int DS9490_reset( const struct parsedname * const pn ) {
    int ret ;
    unsigned char buffer[32] ;
//printf("9490RESET\n");
    if ( (ret=usb_control_msg(pn->in->usb,0x40,COMM_CMD,0x0043, 0x0000, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer,pn))
    ) {
        STATLOCK
        DS9490_reset_errors++;
        STATUNLOCK
        return ret ;
    }
//    USBpowered = (buffer[8]&0x08) == 0x08 ;
    if ( pn ) {
      pn->si->AnyDevices = !(buffer[16]&0x01) ;
#if 0
      /* I seem to loose some devices if this isn't set (only when using
       * owserver though) */
      if(!(buffer[16]&0x01) != 1) {
	printf("DS9490_reset: anydevices would be set to 0 !\n");
      }
      pn->si->AnyDevices = 1 ;
#endif
    }
    return 0 ;
}

static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len,const struct parsedname * const pn ) {
    int ret ;
    usb_dev_handle * usb = pn->in->usb ;
    unsigned char buffer[32] ;

    if ( len > USB_FIFO_EACH ) {
//printf("USBsendback splitting\n");
        return DS9490_sendback_data(data,resp,USB_FIFO_EACH,pn)
            || DS9490_sendback_data(&data[USB_FIFO_EACH],&resp[USB_FIFO_EACH],len-USB_FIFO_EACH,pn) ;
    }

    if ( (ret=usb_bulk_write(usb,DS2490_EP2,data,len, TIMEOUT_USB )) < len ) {
//printf("USBsendback bulk write problem = %d\n",ret);
        usb_clear_halt(usb,DS2490_EP2) ;
        STATLOCK
        DS9490_sendback_data_errors++;
        STATUNLOCK
        return -EIO ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0075, len, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer,pn))
          ) {
//printf("USBsendback control problem\n");
        STATLOCK
        DS9490_sendback_data_errors++;
        STATUNLOCK
        return ret ;
    }

    if ( usb_bulk_read(usb,DS2490_EP3,resp,len, TIMEOUT_USB ) > 0 ) return 0 ;
//printf("USBsendback bulk read problem\n");
    usb_clear_halt(usb,DS2490_EP3) ;
    STATLOCK
    DS9490_sendback_data_errors++;
    STATUNLOCK
    return -EIO ;
}

static int DS9490_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
    unsigned char buffer[32] ;
    unsigned char * cb = pn->in->combuffer ;
    usb_dev_handle * usb = pn->in->usb ;
    struct stateinfo * si = pn->si ;
    int ret ;
    int i ;
    int buflen ;

//printf("DS9490_next_both: Anydevices=%d LastDevice=%d\n",si->AnyDevices, si->LastDevice);
//printf("DS9490_next_both SN in: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
    // if the last call was not the last one
    if ( !si->AnyDevices ) si->LastDevice = 1 ;
    if ( si->LastDevice ) return -ENODEV ;

    /** Play LastDescrepancy games with bitstream */
    memcpy( cb,serialnumber,8) ; /* set bufferto zeros */
    if ( si->LastDiscrepancy > -1 ) UT_setbit(cb,si->LastDiscrepancy,1) ;
    for ( i=si->LastDiscrepancy+1;i<64;i++) {
        UT_setbit(cb,i,0) ;
    }
//printf("DS9490_next_both EP2: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",combuffer[0],combuffer[1],combuffer[2],combuffer[3],combuffer[4],combuffer[5],combuffer[6],combuffer[7]) ;

//printf("USBnextboth\n");
    if ( (ret=usb_bulk_write(usb,DS2490_EP2,cb,8, TIMEOUT_USB )) < 8 ) {
//printf("USBnextboth bulk write problem = %d\n",ret);
        usb_clear_halt(usb,DS2490_EP2) ;
	STATLOCK
	DS9490_next_both_errors++;
	STATUNLOCK
        return -EIO ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x48FD, 0x0100|search, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer,pn))
          ) {
//printf("USBnextboth control problem\n");
	STATLOCK
	DS9490_next_both_errors++;
	STATUNLOCK
        return ret ;
    }
    buflen = buffer[13] ;
    if ( (ret=usb_bulk_read(usb,DS2490_EP3,cb,buflen, TIMEOUT_USB )) > 0 ) {
        memcpy(serialnumber,cb,8) ;
//printf("DS9490_next_both SN out: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
//printf("DS9490_next_both rest: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",combuffer[8],combuffer[9],combuffer[10],combuffer[11],combuffer[12],combuffer[13],combuffer[14],combuffer[15]) ;
//        LastDevice = !(combuffer[8]||combuffer[9]||combuffer[10]||combuffer[11]||combuffer[12]||combuffer[13]||combuffer[14]||combuffer[15]) ;
        si->LastDevice = (ret==8) ;
//printf("DS9490_next_both lastdevice=%d bytes=%d\n",si->LastDevice,ret) ;

        for ( i=63 ; i>=0 ; i-- ) {
            if ( UT_getbit(cb,i+64) && (UT_getbit(cb,i)==0) ) {
                si->LastDiscrepancy = i ;
//printf("DS9490_next_both lastdiscrepancy=%d\n",si->LastDiscrepancy) ;
                break ;
            }
        }
        if( CRC8(serialnumber,8) || (serialnumber[0] == 0) ) {
	  STATLOCK
	  DS9490_next_both_errors++;
	  STATUNLOCK
	  return -EIO;
	}
	return 0;
    }
//printf("USBnextboth bulk read problem error=%d\n",ret);
    usb_clear_halt(usb,DS2490_EP3) ;
    STATLOCK
    DS9490_next_both_errors++;
    STATUNLOCK
    return -EIO ;
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
    usb_dev_handle * usb = pn->in->usb ;
    unsigned char dly = 1 + ( (unsigned char) (delay>>4) ) ;
    int ret ;

    /* Send the byte */
    if ( (ret= DS9490_sendback_data(&byte, &resp , 1,pn)) ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return ret ;
    }
//printf("9490 Powerbyte in=%.2X out %.2X\n",byte,resp) ;
    if ( byte != resp ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return -EIO ;
    }
    /** Pulse duration */
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0213, 0x0000|dly, NULL, 0, TIMEOUT_USB ))<0 ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return ret ;
    }
    /** Pulse start */
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0431, 0x0000, NULL, 0, TIMEOUT_USB ))<0 ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return ret ;
    }
    /** Delay */
    UT_delay( delay ) ;
    /** wait */
    if ( (ret=DS9490wait(buffer,pn)) ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return ret ;
    }
    return 0 ;
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
    unsigned char buffer[32] ;
    int lev ;
    if (new_level == pn->in->ULevel) {     // check if need to change level
        return 0 ;
    }
    switch (new_level) {
    case MODE_NORMAL:
        lev = 0x0000;
        break ;
    case MODE_BREAK:
        return 1 ;
    case MODE_STRONG5:
        lev = 0x0002 ;
        break ; ;
    case MODE_PROGRAM:
        lev = 0x0001 ;
        break ;
    default:
        return 1 ;
    }

    if ( (ret=usb_control_msg(pn->in->usb,0x40,MODE_CMD,MOD_PULSE_EN, lev, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer,pn))
          ) {
      STATLOCK
      DS9490_level_errors++;
      STATUNLOCK
      return ret ;
    }
    pn->in->ULevel = new_level ;
    return 0 ;
}

//--------------------------------------------------------------------------
/** Select
   -- selects a 1-wire device to respond to further commands.
   First resets, then climbs down the branching tree,
    finally 'selects' the device.
   If no device is listed in the parsedname structure,
    only the reset and branching is done. This allows selective listing.
   Return 0=good, else
    reset, send_data, sendback_data
 */
static int DS9490_select_low(const struct parsedname * const pn) {
    int ret ;
    usb_dev_handle * usb = pn->in->usb ;
    int ibranch ;
    // match Serial Number command 0x55
    unsigned char sendbytes[9] = { 0x55, } ;
    unsigned char branch[2] = { 0xCC, 0x33, } ; /* Main, Aux */
    unsigned char resp[3] ;
    unsigned char buffer[32] ;
    unsigned int reset = 0x0100 ;
    unsigned char * cb = pn->in->combuffer ;

    if ( reset ) {
      // reset the 1-wire
      if ( (ret=BUS_reset(pn)) ) {
        STATLOCK
        DS9490_select_low_errors++;
        STATUNLOCK
        return ret ;
      }
      reset = 0x0000 ;
    }
//printf("SELECT\n");
    // send/recieve the transfer buffer
    // verify that the echo of the writes was correct
    for ( ibranch=0 ; ibranch < pn->pathlength ; ++ibranch ) {
#if 0
       /* never reached since reset was cleared above */
       if ( reset ) {
           // reset the 1-wire
           if ( (ret=BUS_reset(pn)) ) return ret ;
           reset = 0 ;
       }
#endif
       memcpy( &sendbytes[1], pn->bp[ibranch].sn, 8 ) ;
//printf("select ibranch=%d %.2X %.2X.%.2X%.2X%.2X%.2X%.2X%.2X %.2X\n",ibranch,send[0],send[1],send[2],send[3],send[4],send[5],send[6],send[7],send[8]);
        if ( (ret=BUS_send_data(sendbytes,9,pn)) ) {
          STATLOCK
          DS9490_select_low_errors++;
          STATUNLOCK
          return ret ;
        }
//printf("select2 branch=%d\n",pn->bp[ibranch].branch);
        if ( (ret=BUS_send_data(&branch[pn->bp[ibranch].branch],1,pn)) || (ret=BUS_readin_data(resp,3,pn)) ) return ret ;
        if ( resp[2] != branch[pn->bp[ibranch].branch] ) {
          STATLOCK
          DS9490_select_low_errors++;
          STATUNLOCK
          return -EINVAL ;
        }
//printf("select3=%d resp=%.2X %.2X %.2X\n",ret,resp[0],resp[1],resp[2]);
    }
    if ( pn->dev == NULL) return 0 ;

    cb[0] = pn->sn[7] ;
    cb[1] = pn->sn[6] ;
    cb[2] = pn->sn[5] ;
    cb[3] = pn->sn[4] ;
    cb[4] = pn->sn[3] ;
    cb[5] = pn->sn[2] ;
    cb[6] = pn->sn[1] ;
    cb[7] = pn->sn[0] ;
    if ( (ret=usb_bulk_write(usb,DS2490_EP2,cb,8, TIMEOUT_USB )) < 8 ) {
        usb_clear_halt(usb,DS2490_EP2) ;
//printf("SELECT write error=%d\n",ret) ;
        STATLOCK
        DS9490_select_low_errors++;
        STATUNLOCK
        return -EIO ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x0465|reset, (int) 0x0055, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer,pn))
          ) {
//printf("SELECT control problem = %d\n",ret);
        STATLOCK
        DS9490_select_low_errors++;
        STATUNLOCK
        return ret ;
    }
    return 0 ;
}

/* The USB device can't actually deliver the EPROM programming voltage */
static int DS9490_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -ENOPROTOOPT ;
}

#endif /* OW_USB */
