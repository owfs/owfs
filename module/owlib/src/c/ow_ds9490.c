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

    ret = DS9490_detect_low(&pn) ;
    if (ret==0) in->busmode = bus_usb ;
    return ret ;
}

static int DS9490_setup_adapter(const struct parsedname * const pn) {
  unsigned char buffer[32];
  int ret ;

  //printf("DS9490_setup_adapter\n");

  if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,CONTROL_CMD,CTL_RESET_DEVICE, 0x0000, NULL, 0, TIMEOUT_USB )) < 0)
    return ret ;

  // set the strong pullup duration to infinite
  if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,COMM_CMD,COMM_SET_DURATION | COMM_IM, 0x0000, NULL, 0, TIMEOUT_USB )) < 0)
    return ret ;
            
  // set the 12V pullup duration to 512us
  if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,COMM_CMD,COMM_SET_DURATION | COMM_IM | COMM_TYPE, 0x0040, NULL, 0, TIMEOUT_USB )) < 0)
    return ret ;
            
#if 0
  if((ret = usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_PULSE_EN, ENABLEPULSE_PRGE, NULL, 0, TIMEOUT_USB )) < 0)
    return ret ;
#endif

  pn->in->ULevel = MODE_NORMAL ;

  if((ret=DS9490_getstatus(buffer,pn,0,0)) < 0) {
    return ret ;
  }

  //printf("DS9490_setup done\n");
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
                    LEVEL_CONNECT("USB DS9490 adapter at %s/%s passed over.\n",bus->dirname,dev->filename)
                } else if ( (pn->in->name=(char *)malloc(strlen(bus->dirname)+strlen(dev->filename)+2) ) == NULL ) {
                    return -ENOMEM ;
                } else {
                    strcpy(pn->in->name,bus->dirname) ;
                    strcat(pn->in->name,"/") ;
                    strcat(pn->in->name,dev->filename) ;
                    if ( (pn->in->connin.usb.usb=usb_open(dev)) ) {
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
                        usb_detach_kernel_driver_np(pn->in->connin.usb.usb,0);
#endif /* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */
                        if ( usb_set_configuration( pn->in->connin.usb.usb, 1 )==0 && usb_claim_interface( pn->in->connin.usb.usb, 0)==0 ) {
    if ( usb_set_altinterface( pn->in->connin.usb.usb, 3)==0 ) {
                                LEVEL_CONNECT("Opened USB DS9490 adapter at %s.\n",pn->in->name)
                                DS9490_setroutines( & pn->in->iroutines ) ;
                                pn->in->Adapter = adapter_DS9490 ; /* OWFS assigned value */
                                pn->in->adapter_name = "DS9490" ;

                                ret = DS9490_setup_adapter(pn) || DS9490_overdrive(MODE_NORMAL, pn) || DS9490_level(MODE_NORMAL, pn) ;
                                if(!ret) return 0 ;
                                LEVEL_DEFAULT("Error setting up USB DS9490 adapter at %s.\n",pn->in->name)
                            } else {
                                LEVEL_CONNECT("Failed to configure alt interface on USB DS9490 adapter at %s.\n",pn->in->name)
                            }
                            usb_release_interface( pn->in->connin.usb.usb, 0) ;
                        } else {
                            LEVEL_CONNECT("Failed to configure/claim interface on USB DS9490 adapter at %s.\n",pn->in->name)
                        }
                        usb_close( pn->in->connin.usb.usb ) ;
                        pn->in->connin.usb.usb = 0 ;
                    } else {
                        LEVEL_CONNECT("Failed to open USB DS9490 adapter at %s.\n",pn->in->name)
                    }
                    if ( pn->in->name ) free(pn->in->name) ;
                    pn->in->name = NULL ;
                    return -EIO ;
                }
            }
        }
    }
    LEVEL_DEFAULT("No available USB DS9490 adapter found\n")
    return -ENODEV ;
}

void DS9490_close(struct connection_in * in) {
    if ( in->connin.usb.usb ) {
        usb_release_interface( in->connin.usb.usb, 0) ;
        usb_close( in->connin.usb.usb ) ;
        in->connin.usb.usb = NULL ;
    }
    LEVEL_CONNECT("Closed USB DS9490 adapter at %s.\n",in->name)
}

/* DS9490_getstatus()
   return -1 on error
   otherwise return number of status bytes in buffer
*/
static int DS9490_getstatus(unsigned char * const buffer, const struct parsedname * const pn, int readlen, int wait_for_idle) {
    int ret , loops = 0 ;
    int i ;
    //printf("DS9490_getstatus: readlen=%d wfi=%d\n", readlen, wait_for_idle);
    do {
        memset(buffer, 0, 32) ; // should not be needed
#ifdef HAVE_USB_INTERRUPT_READ
        // Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
        if ( (ret=usb_interrupt_read(pn->in->connin.usb.usb,DS2490_EP1,buffer,32,TIMEOUT_USB)) < 0 )
#else
        if ( (ret=usb_bulk_read(pn->in->connin.usb.usb,DS2490_EP1,buffer,32,TIMEOUT_USB)) < 0 )
#endif
	{
          STATLOCK
          DS9490_wait_errors++;
          STATUNLOCK
	  return ret ;
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
	if(++loops > 100) return 0 ;  // adapter never got idle
	UT_delay(1);
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

//{
//int i ;
//printf ("USBwait return buffer:\n") ;
//for( i=0;i<8;++i) printf("%.2X: %.2X  |  %.2X: %.2X  ||  %.2X: %.2X  |  %.2X: %.2X\n",i,buffer[i],i+8,buffer[i+8],i+16,buffer[i+16],i+24,buffer[i+24]);
//}
    if(ret < 16) return 0 ;   // incomplete packet??

    for(i=16; i<ret; i++) {
      //printf("status=%X\n", buffer[i]);
      if(buffer[i] & COMMCMDERRORRESULT_SH) { // short detected
        // printf("Short detected on USB DS9490 adapter at %s.\n",pn->in->name) ;
        LEVEL_CONNECT("Short detected on USB DS9490 adapter at %s.\n",pn->in->name)
        // we should perhaps call DS9490_setup_adapter() here.
        return -1;
      }
    }
    //printf("DS9490_getstatus: %d status bytes\n", (ret-16));
    return (ret - 16) ;  // return number of status bytes in buffer
}

static int DS9490_testoverdrive(const struct parsedname * const pn) {
  unsigned char buffer[32] ;
  unsigned char r[8] ;
  unsigned char p = 0x69 ;
  int i ;

  //printf("DS9490_testoverdrive:\n");

  // make sure normal level
  DS9490_level(MODE_NORMAL, pn) ;

  // force to normal communication speed
  DS9490_overdrive(MODE_NORMAL, pn) ;

  if(DS9490_reset(pn) < 0) {
    return -1 ;
  }

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
	  if(r[i] != pn->sn[i]) {
	    break ;
	  }
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
  unsigned int speed;
  unsigned char sp = 0x3C;
  unsigned char resp ;
  int i ;

  // set speed
  if(overdrive & MODE_OVERDRIVE) {
    speed = ONEWIREBUSSPEED_OVERDRIVE ;
  } else {
    speed = ONEWIREBUSSPEED_FLEXIBLE ;
  }

  if (speed == ONEWIREBUSSPEED_OVERDRIVE) {
    //printf("set overdrive speed\n");
	  
    if(!pn->in->use_overdrive_speed) return 0 ; // adapter doesn't use overdrive
    if(!(pn->in->USpeed & MODE_OVERDRIVE)) {
      // we need to change speed to overdrive
      for(i=0; i<3; i++) {
	if ((ret=DS9490_reset(pn)) < 0) {
	  continue ;
	}
	if ((DS9490_sendback_data(&sp,&resp,1, pn)<0) || (sp!=resp)) {
	  //printf("error sending 0x3C\n");
	  continue ;
	}
	ret = usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_1WIRE_SPEED, speed, NULL, 0, TIMEOUT_USB ) ;
	if(ret == 0) {
	  break ;
	}
      }
      if(i==3) {
	//printf("error after 3 tries\n");
	return -EINVAL ;
      }
    }
    pn->in->USpeed = MODE_OVERDRIVE ;
  } else {
    ret = usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_1WIRE_SPEED, speed, NULL, 0, TIMEOUT_USB ) ;
    if(ret < 0) {
      return -EINVAL ;
    }
    pn->in->USpeed = MODE_NORMAL ;
  }
  return 0 ;
}

/* Reset adapter and detect devices on the bus */
static int DS9490_reset( const struct parsedname * const pn ) {
    int ret ;
    unsigned char buffer[32] ;
//printf("9490RESET\n");
//printf("DS9490_reset() index=%d pn->in->Adapter=%d %s\n", pn->in->index, pn->in->Adapter, pn->in->adapter_name);

    memset(buffer, 0, 32); 
 
    if((ret=DS9490_level(MODE_NORMAL, pn)) < 0) {
        return ret ;
    }

    // force normal speed if not using overdrive anymore
    if (!pn->in->use_overdrive_speed || (pn->in->USpeed & MODE_OVERDRIVE)) {
        if((ret=DS9490_overdrive(MODE_NORMAL, pn)) < 0) {
            return ret ;
        }
    }

    if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,COMM_CMD,
            COMM_1_WIRE_RESET | COMM_F | COMM_IM | COMM_SE,
            ((pn->in->USpeed & MODE_OVERDRIVE)?ONEWIREBUSSPEED_OVERDRIVE:ONEWIREBUSSPEED_FLEXIBLE),
            NULL, 0, TIMEOUT_USB ))<0 ) {
        STATLOCK
        DS9490_reset_errors++;
        STATUNLOCK
        return ret ;
    }

    // extra delay for alarming DS1994/DS2404 complience
    //if(!(pn->in->USpeed & MODE_OVERDRIVE)) UT_delay(5);

    if((ret=DS9490_getstatus(buffer,pn,0,0)) < 0) {
        STATLOCK
        DS9490_reset_errors++;
        STATUNLOCK
        return ret ;
    }

//    USBpowered = (buffer[8]&STATUSFLAGS_PMOD) == STATUSFLAGS_PMOD ;
    if ( pn ) {
        int i ;
        unsigned char val ;
        pn->si->AnyDevices = 1;
        for(i=0; i<ret; i++) {
            val = buffer[16+i];
            //printf("Status bytes: %X\n", val);
            if(val != ONEWIREDEVICEDETECT) {
                // check for NRS bit (0x01)
                if(val & COMMCMDERRORRESULT_NRS) {
                // empty bus detected, no presence pulse detected
                pn->si->AnyDevices = 0;
                }
            }
        }
    }
    //printf("DS9490_reset: ok\n");
    return 0 ;
}

static int DS9490_read( unsigned char * const buf, const size_t size, const struct parsedname * const pn) {
    int ret;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    //printf("DS9490_read\n");
    if ( (ret=usb_bulk_read(usb,DS2490_EP3,buf,size, TIMEOUT_USB )) > 0 ) {
      return ret ;
    }
    //printf("DS9490_read problem\n");
    usb_clear_halt(usb,DS2490_EP3) ;
    return -EIO ;
}

static int DS9490_write( const unsigned char * const buf, const size_t size, const struct parsedname * const pn) {
    int ret;
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    //printf("DS9490_write\n");
    if ( (ret=usb_bulk_write(usb,DS2490_EP2,buf,size, TIMEOUT_USB )) > 0 ) {
      return ret ;
    }
    //printf("DS9490_write problem\n");
    usb_clear_halt(usb,DS2490_EP2) ;
    return -EIO ;
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

    if ( (ret=DS9490_write(data, len, pn)) < len ) {
//printf("USBsendback bulk write problem = %d\n",ret);
        STATLOCK
        DS9490_sendback_data_errors++;
        STATUNLOCK
        return -EIO ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,COMM_BLOCK_IO | COMM_IM | COMM_F, len, NULL, 0, TIMEOUT_USB ))<0
        ||
        ((ret = DS9490_getstatus(buffer,pn, len,1)) < 0) // wait for len bytes
        ) {
//printf("USBsendback control problem\n");
        STATLOCK
        DS9490_sendback_data_errors++;
        STATUNLOCK
        return ret ;
    }

    if ( DS9490_read(resp, len, pn) > 0 ) return 0 ;
//printf("USBsendback bulk read problem\n");
    STATLOCK
    DS9490_sendback_data_errors++;
    STATUNLOCK
    return -EIO ;
}

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
    if ( (ret=DS9490_write(cb,8,pn)) < 8 ) {
//printf("USBnextboth bulk write problem = %d\n",ret);
	STATLOCK
	DS9490_next_both_errors++;
	STATUNLOCK
        return -EIO ;
    }

//printf("USBnextboth\n");

    // COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS
    // 0xF4 + +0x1 + 0x8 + 0x800 + 0x4000 = 0x48FD
    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,0x48FD, 0x0100|search, NULL, 0, TIMEOUT_USB ))<0 ) {
//printf("USBnextboth control problem\n");
	STATLOCK
	DS9490_next_both_errors++;
	STATUNLOCK
        return ret ;
    }

    if(gettimeofday(&tv, &tz)<0) return -1;
    endtime = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 + 200;
    do {
      // just get first status packet without waiting for not idle
      if((ret = DS9490_getstatus(buffer,pn,0,0)) < 0) {
	break ;
      }
      for(i=0; i<ret; i++) {
	if(buffer[16+i] != ONEWIREDEVICEDETECT) {
	  // If other status then ONEWIREDEVICEDETECT we have failed
	  break;
	}
      }
      
      if(gettimeofday(&tv, &tz)<0) return -1;
      now = (tv.tv_sec&0xFFFF)*1000 + tv.tv_usec/1000 ;
    } while(((buffer[8]&STATUSFLAGS_IDLE) == 0) && (endtime > now));

    if(buffer[13] == 0) {  // (ReadBufferStatus)
      return 0;
    }
    buflen = 16 ;  // try read 16 bytes
    //printf("USBnextboth len=%d (available=%d)\n", buflen, buffer[13]);
    if ( (ret=DS9490_read(cb,buflen,pn)) > 0 ) {
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
        //printf("USBnextboth done\n");
        return 0;
    }
//printf("USBnextboth bulk read problem error=%d\n",ret);
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
    usb_dev_handle * usb = pn->in->connin.usb.usb ;
    unsigned char dly = 1 + ( (unsigned char) (delay>>4) ) ;
    int ret ;

    //printf("9490 Powerbyte\n") ;

#if 1
    /* Send the byte */
    if ( (ret=DS9490_sendback_data(&byte, &resp , 1,pn)) ) {
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
    if ( ((ret=DS9490_getstatus(buffer,pn,0,1))<0) ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      return ret ;
    }
#else
    // set the strong pullup
    if ( (ret=usb_control_msg(usb,0x40,MODE_CMD,MOD_PULSE_EN, ENABLEPULSE_SPUE, NULL, 0, TIMEOUT_USB ))<0 ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
	printf("Powerbyte err1\n");
      return ret ;
    }

    if ( (ret=usb_control_msg(usb,0x40,COMM_CMD,COMM_BYTE_IO | COMM_IM | COMM_SPU, byte & 0xFF, NULL, 0, TIMEOUT_USB ))<0 ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
      //printf("Powerbyte err2\n");
      return ret ;
    }

    pn->in->ULevel = MODE_STRONG5;    

    /** Delay */
    UT_delay( delay ) ;

    /** wait */
    if ( ((ret=DS9490_getstatus(buffer,pn,0,1))<0) ) {
      STATLOCK
      DS9490_PowerByte_errors++;
      STATUNLOCK
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
    if (new_level == pn->in->ULevel) {     // check if need to change level
        return 0 ;
    }
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
    if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,MODE_CMD,MOD_PULSE_EN, lev, NULL, 0, TIMEOUT_USB ))<0
	 ) {
      STATLOCK
      DS9490_level_errors++;
      STATUNLOCK
      return ret ;
    }

    // set the strong pullup duration to infinite
    if ( (ret=usb_control_msg(pn->in->connin.usb.usb,0x40,COMM_CMD,COMM_PULSE | COMM_IM, 0, NULL, 0, TIMEOUT_USB ))<0
	 ) {
      STATLOCK
      DS9490_level_errors++;
      STATUNLOCK
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
