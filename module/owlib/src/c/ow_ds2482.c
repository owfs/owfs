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

#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

// #include <linux/i2c.h>
#include <sys/ioctl.h>
#include "i2c-dev.h"

static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) ;


/* i2c support for the DS2482-100 and DS2482-800 1-wire host adapters */
/* Stolen shamelessly from Ben Gardners kernel module */

#if 0
/**
 * ds2482.c - provides i2c to w1-master bridge(s)
 * Copyright (C) 2005  Ben Gardner <bgardner@wabtec.com>
 *
 * The DS2482 is a sensor chip made by Dallas Semiconductor (Maxim).
 * It is a I2C to 1-wire bridge.
 * There are two variations: -100 and -800, which have 1 or 8 1-wire ports.
 * The complete datasheet can be obtained from MAXIM's website at:
 *   http://www.maxim-ic.com/quick_view2.cfm/qv_pk/4382
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * MODULE_AUTHOR("Ben Gardner <bgardner@wabtec.com>");
 * MODULE_DESCRIPTION("DS2482 driver");
 * MODULE_LICENSE("GPL");
 */

#include <linux/i2c.h>

/**
 * Address is selected using 2 pins, resulting in 4 possible addresses.
 *  0x18, 0x19, 0x1a, 0x1b
 * However, the chip cannot be detected without doing an i2c write,
 * so use the force module parameter.
 */
static unsigned short normal_i2c[] = {I2C_CLIENT_END};

/**
 * Insmod parameters
 */
I2C_CLIENT_INSMOD_1(ds2482);

/**
 * The DS2482 registers - there are 3 registers that are addressed by a read
 * pointer. The read pointer is set by the last command executed.
 *
 * To read the data, issue a register read for any address
 */
#define DS2482_CMD_RESET       0xF0    /* No param */
#define DS2482_CMD_SET_READ_PTR        0xE1    /* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_CHANNEL_SELECT  0xC3    /* Param: Channel byte - DS2482-800 only */
#define DS2482_CMD_WRITE_CONFIG        0xD2    /* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET     0xB4    /* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT    0x87    /* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE    0xA5    /* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE 0x96    /* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET   0x78    /* Param: Dir byte (bit7) */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_PTR_CODE_STATUS     0xF0
#define DS2482_PTR_CODE_DATA       0xE1
#define DS2482_PTR_CODE_CHANNEL        0xD2    /* DS2482-800 only */
#define DS2482_PTR_CODE_CONFIG     0xC3

/**
 * Configure Register bit definitions
 * The top 4 bits always read 0.
 * To write, the top nibble must be the 1's compl. of the low nibble.
 */
#define DS2482_REG_CFG_1WS     0x08
#define DS2482_REG_CFG_SPU     0x04
#define DS2482_REG_CFG_PPM     0x02
#define DS2482_REG_CFG_APU     0x01


/**
 * Write and verify codes for the CHANNEL_SELECT command (DS2482-800 only).
 * To set the channel, write the value at the index of the channel.
 * Read and compare against the corresponding value to verify the change.
 */
static const u8 ds2482_chan_wr[8] =
   { 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 };
static const u8 ds2482_chan_rd[8] =
   { 0xB8, 0xB1, 0xAA, 0xA3, 0x9C, 0x95, 0x8E, 0x87 };


/**
 * Status Register bit definitions (read only)
 */
#define DS2482_REG_STS_DIR     0x80
#define DS2482_REG_STS_TSB     0x40
#define DS2482_REG_STS_SBR     0x20
#define DS2482_REG_STS_RST     0x10
#define DS2482_REG_STS_LL      0x08
#define DS2482_REG_STS_SD      0x04
#define DS2482_REG_STS_PPD     0x02
#define DS2482_REG_STS_1WB     0x01


static int ds2482_attach_adapter(struct i2c_adapter *adapter);
static int ds2482_detect(struct i2c_adapter *adapter, int address, int kind);
static int ds2482_detach_client(struct i2c_client *client);


/**
 * Driver data (common to all clients)
 */
static struct i2c_driver ds2482_driver = {
   .driver = {
       .owner  = THIS_MODULE,
       .name   = "ds2482",
   },
   .attach_adapter = ds2482_attach_adapter,
   .detach_client  = ds2482_detach_client,
};

/*
 * Client data (each client gets its own)
 */

struct ds2482_data;

struct ds2482_w1_chan {
   struct ds2482_data  *pdev;
   u8          channel;
   struct w1_bus_master    w1_bm;
};

struct ds2482_data {
   struct i2c_client   client;
   struct semaphore    access_lock;

   /* 1-wire interface(s) */
   int         w1_count;   /* 1 or 8 */
   struct ds2482_w1_chan   w1_ch[8];

   /* per-device values */
   u8          channel;
   u8          read_prt;   /* see DS2482_PTR_CODE_xxx */
   u8          reg_config;
};


/**
 * Sets the read pointer.
 * @param pdev     The ds2482 client pointer
 * @param read_ptr see DS2482_PTR_CODE_xxx above
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_select_register(struct ds2482_data *pdev, u8 read_ptr)
{
   if (pdev->read_prt != read_ptr) {
       if (i2c_smbus_write_byte_data(&pdev->client,
                         DS2482_CMD_SET_READ_PTR,
                         read_ptr) < 0)
           return -1;

       pdev->read_prt = read_ptr;
   }
   return 0;
}

/**
 * Sends a command without a parameter
 * @param pdev The ds2482 client pointer
 * @param cmd  DS2482_CMD_RESET,
 *     DS2482_CMD_1WIRE_RESET,
 *     DS2482_CMD_1WIRE_READ_BYTE
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_send_cmd(struct ds2482_data *pdev, u8 cmd)
{
   if (i2c_smbus_write_byte(&pdev->client, cmd) < 0)
       return -1;

   pdev->read_prt = DS2482_PTR_CODE_STATUS;
   return 0;
}

/**
 * Sends a command with a parameter
 * @param pdev The ds2482 client pointer
 * @param cmd  DS2482_CMD_WRITE_CONFIG,
 *     DS2482_CMD_1WIRE_SINGLE_BIT,
 *     DS2482_CMD_1WIRE_WRITE_BYTE,
 *     DS2482_CMD_1WIRE_TRIPLET
 * @param byte The data to send
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_send_cmd_data(struct ds2482_data *pdev,
                      u8 cmd, u8 byte)
{
   if (i2c_smbus_write_byte_data(&pdev->client, cmd, byte) < 0)
       return -1;

   /* all cmds leave in STATUS, except CONFIG */
   pdev->read_prt = (cmd != DS2482_CMD_WRITE_CONFIG) ?
            DS2482_PTR_CODE_STATUS : DS2482_PTR_CODE_CONFIG;
   return 0;
}


/*
 * 1-Wire interface code
 */

#define DS2482_WAIT_IDLE_TIMEOUT   100

/**
 * Waits until the 1-wire interface is idle (not busy)
 *
 * @param pdev Pointer to the device structure
 * @return the last value read from status or -1 (failure)
 */
static int ds2482_wait_1wire_idle(struct ds2482_data *pdev)
{
   int temp = -1;
   int retries = 0;

   if (!ds2482_select_register(pdev, DS2482_PTR_CODE_STATUS)) {
       do {
           temp = i2c_smbus_read_byte(&pdev->client);
       } while ((temp >= 0) && (temp & DS2482_REG_STS_1WB) &&
            (++retries > DS2482_WAIT_IDLE_TIMEOUT));
   }

   if (retries > DS2482_WAIT_IDLE_TIMEOUT)
       printk(KERN_ERR "%s: timeout on channel %d\n",
              __func__, pdev->channel);

   return temp;
}

/**
 * Selects a w1 channel.
 * The 1-wire interface must be idle before calling this function.
 *
 * @param pdev     The ds2482 client pointer
 * @param channel  0-7
 * @return     -1 (failure) or 0 (success)
 */
static int ds2482_set_channel(struct ds2482_data *pdev, u8 channel)
{
   if (i2c_smbus_write_byte_data(&pdev->client, DS2482_CMD_CHANNEL_SELECT,
                     ds2482_chan_wr[channel]) < 0)
       return -1;

   pdev->read_prt = DS2482_PTR_CODE_CHANNEL;
   pdev->channel = -1;
   if (i2c_smbus_read_byte(&pdev->client) == ds2482_chan_rd[channel]) {
       pdev->channel = channel;
       return 0;
   }
   return -1;
}


/**
 * Performs the touch-bit function, which writes a 0 or 1 and reads the level.
 *
 * @param data The ds2482 channel pointer
 * @param bit  The level to write: 0 or non-zero
 * @return The level read: 0 or 1
 */
static u8 ds2482_w1_touch_bit(void *data, u8 bit)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int status = -1;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the touch command, wait until 1WB == 0, return the status */
   if (!ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_SINGLE_BIT,
                 bit ? 0xFF : 0))
       status = ds2482_wait_1wire_idle(pdev);

   up(&pdev->access_lock);

   return (status & DS2482_REG_STS_SBR) ? 1 : 0;
}

/**
 * Performs the triplet function, which reads two bits and writes a bit.
 * The bit written is determined by the two reads:
 *   00 => dbit, 01 => 0, 10 => 1
 *
 * @param data The ds2482 channel pointer
 * @param dbit The direction to choose if both branches are valid
 * @return b0=read1 b1=read2 b3=bit written
 */
static u8 ds2482_w1_triplet(void *data, u8 dbit)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int status = (3 << 5);

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the triplet command, wait until 1WB == 0, return the status */
   if (!ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_TRIPLET,
                 dbit ? 0xFF : 0))
       status = ds2482_wait_1wire_idle(pdev);

   up(&pdev->access_lock);

   /* Decode the status */
   return (status >> 5);
}

/**
 * Performs the write byte function.
 *
 * @param data The ds2482 channel pointer
 * @param byte The value to write
 */
static void ds2482_w1_write_byte(void *data, u8 byte)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the write byte command */
   ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_WRITE_BYTE, byte);

   up(&pdev->access_lock);
}

/**
 * Performs the read byte function.
 *
 * @param data The ds2482 channel pointer
 * @return The value read
 */
static u8 ds2482_w1_read_byte(void *data)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int result;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the read byte command */
   ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_READ_BYTE);

   /* Wait until 1WB == 0 */
   ds2482_wait_1wire_idle(pdev);

   /* Select the data register */
   ds2482_select_register(pdev, DS2482_PTR_CODE_DATA);

   /* Read the data byte */
   result = i2c_smbus_read_byte(&pdev->client);

   up(&pdev->access_lock);

   return result;
}


/**
 * Sends a reset on the 1-wire interface
 *
 * @param data The ds2482 channel pointer
 * @return 0=Device present, 1=No device present or error
 */
static u8 ds2482_w1_reset_bus(void *data)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int err;
   u8 retval = 1;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the reset command */
   err = ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_RESET);
   if (err >= 0) {
       /* Wait until the reset is complete */
       err = ds2482_wait_1wire_idle(pdev);
       retval = !(err & DS2482_REG_STS_PPD);

       /* If the chip did reset since detect, re-config it */
       if (err & DS2482_REG_STS_RST)
           ds2482_send_cmd_data(pdev, DS2482_CMD_WRITE_CONFIG,
                        0xF0);
   }

   up(&pdev->access_lock);

   return retval;
}


/**
 * Called to see if the device exists on an i2c bus.
 */
static int ds2482_attach_adapter(struct i2c_adapter *adapter)
{
   return i2c_probe(adapter, &addr_data, ds2482_detect);
}


/*
 * The following function does more than just detection. If detection
 * succeeds, it also registers the new chip.
 */
static int ds2482_detect(struct i2c_adapter *adapter, int address, int kind)
{
   struct ds2482_data *data;
   struct i2c_client  *new_client;
   int err = 0;
   int temp1;
   int idx;

   if (!i2c_check_functionality(adapter,
                    I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
                    I2C_FUNC_SMBUS_BYTE))
       goto exit;

   if (!(data = kzalloc(sizeof(struct ds2482_data), GFP_KERNEL))) {
       err = -ENOMEM;
       goto exit;
   }

   new_client = &data->client;
   i2c_set_clientdata(new_client, data);
   new_client->addr = address;
   new_client->driver = &ds2482_driver;
   new_client->adapter = adapter;

   /* Reset the device (sets the read_ptr to status) */
   if (ds2482_send_cmd(data, DS2482_CMD_RESET) < 0) {
       dev_dbg(&adapter->dev, "DS2482 reset failed at 0x%02x.\n",
           address);
       goto exit_free;
   }

   /* Sleep at least 525ns to allow the reset to complete */
   ndelay(525);

   /* Read the status byte - only reset bit and line should be set */
   temp1 = i2c_smbus_read_byte(new_client);
   if (temp1 != (DS2482_REG_STS_LL | DS2482_REG_STS_RST)) {
       dev_dbg(&adapter->dev, "DS2482 (0x%02x) reset status "
           "0x%02X - not a DS2482\n", address, temp1);
       goto exit_free;
   }

   /* Detect the 8-port version */
   data->w1_count = 1;
   if (ds2482_set_channel(data, 7) == 0)
       data->w1_count = 8;

   /* Set all config items to 0 (off) */
   ds2482_send_cmd_data(data, DS2482_CMD_WRITE_CONFIG, 0xF0);

   /* We can fill in the remaining client fields */
   snprintf(new_client->name, sizeof(new_client->name), "ds2482-%d00",
        data->w1_count);

   init_MUTEX(&data->access_lock);

   /* Tell the I2C layer a new client has arrived */
   if ((err = i2c_attach_client(new_client)))
       goto exit_free;

   /* Register 1-wire interface(s) */
   for (idx = 0; idx < data->w1_count; idx++) {
       data->w1_ch[idx].pdev = data;
       data->w1_ch[idx].channel = idx;

       /* Populate all the w1 bus master stuff */
       data->w1_ch[idx].w1_bm.data       = &data->w1_ch[idx];
       data->w1_ch[idx].w1_bm.read_byte  = ds2482_w1_read_byte;
       data->w1_ch[idx].w1_bm.write_byte = ds2482_w1_write_byte;
       data->w1_ch[idx].w1_bm.touch_bit  = ds2482_w1_touch_bit;
       data->w1_ch[idx].w1_bm.triplet    = ds2482_w1_triplet;
       data->w1_ch[idx].w1_bm.reset_bus  = ds2482_w1_reset_bus;

       err = w1_add_master_device(&data->w1_ch[idx].w1_bm);
       if (err) {
           data->w1_ch[idx].pdev = NULL;
           goto exit_w1_remove;
       }
   }

   return 0;

exit_w1_remove:
   i2c_detach_client(new_client);

   for (idx = 0; idx < data->w1_count; idx++) {
       if (data->w1_ch[idx].pdev != NULL)
           w1_remove_master_device(&data->w1_ch[idx].w1_bm);
   }
exit_free:
   kfree(data);
exit:
   return err;
}

static int ds2482_detach_client(struct i2c_client *client)
{
   struct ds2482_data   *data = i2c_get_clientdata(client);
   int err, idx;

   /* Unregister the 1-wire bridge(s) */
   for (idx = 0; idx < data->w1_count; idx++) {
       if (data->w1_ch[idx].pdev != NULL)
           w1_remove_master_device(&data->w1_ch[idx].w1_bm);
   }

   /* Detach the i2c device */
   if ((err = i2c_detach_client(client))) {
       dev_err(&client->dev,
           "Deregistration failed, client not detached.\n");
       return err;
   }

   /* Free the memory */
   kfree(data);
   return 0;
}

static int __init sensors_ds2482_init(void)
{
   return i2c_add_driver(&ds2482_driver);
}

static void __exit sensors_ds2482_exit(void)
{
   i2c_del_driver(&ds2482_driver);
}


MODULE_AUTHOR("Ben Gardner <bgardner@wabtec.com>");
MODULE_DESCRIPTION("DS2482 driver");
MODULE_LICENSE("GPL");

module_init(sensors_ds2482_init);
module_exit(sensors_ds2482_exit);
                    --
                            1
#endif

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */

static int DS2482_reset( const struct parsedname * pn ) ;
static int DS2482_sendback_bits( const unsigned char * outbits , unsigned char * inbits , const size_t length, const struct parsedname * pn ) ;
static void DS2482_setroutines( struct interface_routines * f ) ;
static int DS2482_send_and_get( const unsigned char * bussend, unsigned char * busget, const size_t length, const struct parsedname * pn ) ;
static int CreateChannels( int num, struct connection_in * in ) ;

#define	OneBit	0xFF
#define ZeroBit 0xC0

/* Device-specific functions */
static void DS2482_setroutines( struct interface_routines * const f ) {
    f->detect        = DS2482_detect         ;
    f->reset         = DS2482_reset          ;
    f->next_both     = DS2482_next_both      ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = BUS_PowerByte_low     ;
//    f->ProgramPulse = ;
    f->sendback_data = BUS_sendback_data_low ;
    f->sendback_bits = DS2482_sendback_bits  ;
    f->select        = BUS_select_low        ;
    f->reconnect     = NULL                  ;
    f->close         = COM_close             ;
}

/* Open a DS2482 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
// no bus locking here (higher up)
int DS2482_detect( struct connection_in * in ) {
    struct parsedname pn ;
    int ret = 0 ;
    int test_address[4] = { 0x18, 0x19, 0x1a, 0x1b } ;
    int i ;
    
    /* open the COM port */
    in->connin.i2c.fd = open(in->name,O_RDWR) ;
    if ( in->connin.i2c.fd < 0 ) {
        ERROR_CONNECT("Could not open i2c device %s\n",in->name) ;
        return -ENODEV ;
    }

    /* clycle though the possible addresses */
    for ( i=0 ; i<4 ; ++i ) {
        /* set the candidate address */
        if ( ioctl(in->connin.i2c.fd,I2C_SLAVE,test_address[i]) < 0 ) {
            ERROR_CONNECT("Cound not set trial i2c address to %.2X\n",test_address[i]) ;
            return -ENODEV ;
        }
    }
    /* fellthough, no device found */
    return -ENODEV ;
}
#if 0    
if (!i2c_check_functionality(adapter,
         I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
                 I2C_FUNC_SMBUS_BYTE))
        /* Set up low-level routines */
    DS2482_setroutines( & (in->iroutines) ) ;

    
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    return DS2482_reset(&pn) ;
}
#endif
/* DS2482 Reset -- A little different from DS2480B */
// return 1 shorted, 0 ok, <0 error
static int DS2482_reset( const struct parsedname * const pn ) {
    unsigned char resetbyte = 0xF0 ;
    unsigned char c;
    struct termios term;
    int fd = pn->in->fd ;
    int ret ;

    if(fd < 0) return -1;

    /* 8 data bits */
    tcgetattr(fd, &term);
    term.c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
    cfsetospeed(&term, B9600);
    cfsetispeed(&term, B9600);
    if (tcsetattr(fd, TCSANOW, &term ) < 0 ) {
        STAT_ADD1_BUS(BUS_tcsetattr_errors,pn->in);
        return -EIO ;
    }
    if ( (ret=DS2482_send_and_get(&resetbyte,&c,1,pn)) ) return ret ;

    switch(c) {
    case 0:
        STAT_ADD1_BUS( BUS_short_errors, pn->in ) ;
        LEVEL_CONNECT("1-wire bus short circuit.\n")
        ret = 1 ; // short circuit
        /* fall through */
    case 0xF0:
        pn->in->AnyDevices = 0 ;
        break ;
    default:
        pn->in->AnyDevices = 1 ;
        pn->in->ProgramAvailable = 0 ; /* from digitemp docs */
        if(pn->in->ds2404_compliance) {
            // extra delay for alarming DS1994/DS2404 compliance
            UT_delay(5);
        }
    }

    /* Reset all settings */
    term.c_lflag = 0;
    term.c_iflag = 0;
    term.c_oflag = 0;

    /* 1 byte at a time, no timer */
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    /* 6 data bits, Receiver enabled, Hangup, Dont change "owner" */
    term.c_cflag = CS6 | CREAD | HUPCL | CLOCAL;

#ifndef B115200
    /* MacOSX support max 38400 in termios.h ? */
    cfsetispeed(&term, B38400);       /* Set input speed to 38.4k    */
    cfsetospeed(&term, B38400);       /* Set output speed to 38.4k   */
#else
    cfsetispeed(&term, B115200);       /* Set input speed to 115.2k    */
    cfsetospeed(&term, B115200);       /* Set output speed to 115.2k   */
#endif

    if(tcsetattr(fd, TCSANOW, &term) < 0 ) {
        STAT_ADD1_BUS(BUS_tcsetattr_errors,pn->in);
        return -EFAULT ;
    }
    /* Flush the input and output buffers */
    COM_flush(pn) ;
    return ret ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
/* Dispatches DS2482_MAX_BITS "bits" at a time */
#define DS2482_MAX_BITS 24
int DS2482_sendback_bits( const unsigned char * outbits , unsigned char * inbits , const size_t length, const struct parsedname * pn ) {
    int ret ;
    unsigned char d[DS2482_MAX_BITS] ;
    size_t l=0 ;
    size_t i=0 ;
    size_t start = 0 ;

    if ( length==0 ) return 0 ;

    /* Split into smaller packets? */
    do {
        d[l++] = outbits[i++] ? OneBit : ZeroBit ;
        if ( l==DS2482_MAX_BITS || i==length ) {
            /* Communication with DS2482 routine */
            if ( (ret= DS2482_send_and_get(d,&inbits[start],l,pn)) ) {
                STAT_ADD1_BUS( BUS_bit_errors, pn->in ) ;
                return ret ;
            }
            l = 0 ;
            start = i ;
        }
    } while ( i<length ) ;
            
    /* Decode Bits */
    for ( i=0 ; i<length ; ++i ) inbits[i] &= 0x01 ;

    return 0 ;
}

/* Routine to send a string of bits and get another string back */
/* This seems rather COM-port specific */
/* Indeed, will move to DS2482 */
static int DS2482_send_and_get( const unsigned char * bussend, unsigned char * busget, const size_t length, const struct parsedname * pn ) {
    size_t gl = length ;
    ssize_t r ;
    size_t sl = length ;
    int rc ;

    if ( sl > 0 ) {
        /* first flush... should this really be done? Perhaps it's a good idea
        * so read function below doesn't get any other old result */
        COM_flush(pn);
    
        /* send out string, and handle interrupted system call too */
        while(sl > 0) {
            if(!pn->in) break;
            r = write(pn->in->fd, &bussend[length-sl], sl);
            if(r < 0) {
                if(errno == EINTR) {
                    /* write() was interrupted, try again */
                    STAT_ADD1_BUS(BUS_write_interrupt_errors,pn->in);
                    continue;
                }
                break;
            }
            sl -= r;
        }
        if(pn->in) {
            tcdrain(pn->in->fd) ; /* make sure everthing is sent */
            gettimeofday( &(pn->in->bus_write_time) , NULL );
        }
        if(sl > 0) {
            STAT_ADD1_BUS(BUS_write_errors,pn->in);
            return -EIO;
        }
        //printf("SAG written\n");
    }
    
    /* get back string -- with timeout and partial read loop */
    if ( busget && length ) {
        while ( gl > 0 ) {
            fd_set readset;
            struct timeval tv;
            //printf("SAG readlength=%d\n",gl);
            if(!pn->in) break;
#if 1
                /* I can't imagine that 5 seconds timeout is needed???
                * Any comments Paul ? */
                tv.tv_sec = 5;
                tv.tv_usec = 0;
#else
                tv.tv_sec = 0;
                tv.tv_usec = 500000;  // 500ms
#endif
                /* Initialize readset */
                FD_ZERO(&readset);
                FD_SET(pn->in->fd, &readset);
            
                /* Read if it doesn't timeout first */
                rc = select( pn->in->fd+1, &readset, NULL, NULL, &tv );
                if( rc > 0 ) {
                //printf("SAG selected\n");
                    /* Is there something to read? */
                    if( FD_ISSET( pn->in->fd, &readset )==0 ) {
                        STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                        return -EIO ; /* error */
                    }
                    update_max_delay(pn);
                    r = read( pn->in->fd, &busget[length-gl], gl ) ; /* get available bytes */
                    //printf("SAG postread ret=%d\n",r);
                    if (r < 0) {
                        if(errno == EINTR) {
                            /* read() was interrupted, try again */
                            STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                            continue;
                        }
                        STAT_ADD1_BUS(BUS_read_errors,pn->in);
                        return r ;
                    }
                    gl -= r ;
                } else if(rc < 0) { /* select error */
                    if(errno == EINTR) {
                        /* select() was interrupted, try again */
                        STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                        continue;
                    }
                    STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                    return -EINTR;
                } else { /* timed out */
                    STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
                    return -EAGAIN;
                }
        }
    } else {
        /* I'm not sure about this... Shouldn't flush buffer if
        user decide not to read any bytes. Any comments Paul ? */
        //COM_flush(pn) ;
    }
    return 0 ;
}

/* create more channels,
   inserts in connection_in chain
   "in" points to  first (head) channel
   called even if n==1
 */
static int CreateChannels( int num, struct connection_in * in ) {
    int i ;
    char * name[2] = { "DS2482-100", "DS2482-800", } ;
    in->connin.i2c.head = in ;
    in->connin.i2c.index = 0 ;
    in->connin.i2c.channels = num ;
    in->connin.i2c.current = num ; // out-of-bounds value to force setting */
    in->Adapter = num==1 ? adapter_DS2482_100 : adapter_DS2482_800 ; /* OWFS assigned value */
    in->adapter_name = num==1 ? name[0]:name[1] ;
    in->busmode = bus_i2c ;
#ifdef OW_MT
    pthread_mutex_init(&(in->connin.i2c.i2c_mutex), pmattr);
#endif /* OW_MT */
    for ( i=1 ; i<num ; ++i ) {
        struct connection_in * added = NewIn() ;
        if ( added==NULL ) return -ENOMEM ;
        added->connin.i2c.head = in ;
        added->connin.i2c.index = i ;
        added->connin.i2c.channels = num ;
        added->Adapter = adapter_DS2482_800 ; /* OWFS assigned value */
        added->adapter_name = name[1] ;
        added->busmode = bus_i2c ;
    }
    return 0 ;
}

static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) {
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    unsigned char bits[3] ;
    int ret ;

    // initialize for search
    // if the last call was not the last one
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&(ds->search),1,pn)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) { /* First bit */
            /* get two bits (AND'ed bit and AND'ed complement) */
            if ( (ret=BUS_sendback_bits(&bits[1],&bits[1],2,pn)) ) return ret ;
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( (ret=BUS_sendback_bits(bits,bits,3,pn)) ) return ret ;
            } else { /* last bit */
                if ( (ret=BUS_sendback_bits(bits,bits,1,pn)) ) return ret ;
                break ;
            }
        }
        if ( bits[1] ) {
            if ( bits[2] ) { /* 1,1 */
                break ; /* No devices respond */
            } else { /* 1,0 */
                search_direction = 1;  // bit write value for search
            }
        } else if ( bits[2] ) { /* 0,1 */
            search_direction = 0;  // bit write value for search
        } else if (bit_number > ds->LastDiscrepancy) {  /* 0,0 looking for last discrepancy in this new branch */
            // Past branch, select zeros for now
            search_direction = 0 ;
            last_zero = bit_number;
        } else if (bit_number == ds->LastDiscrepancy) {  /* 0,0 -- new branch */
            // at branch (again), select 1 this time
            search_direction = 1 ; // if equal to last pick 1, if not then pick 0
        } else  if (UT_getbit(ds->sn,bit_number)) { /* 0,0 -- old news, use previous "1" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 1 ;
        } else {  /* 0,0 -- old news, use previous "0" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 0 ;
            last_zero = bit_number;
        }
        // check for Last discrepancy in family
        //if (last_zero < 9) si->LastFamilyDiscrepancy = last_zero;
        UT_setbit(ds->sn,bit_number,search_direction) ;

        // serial number search direction write bit
        //if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(ds->sn,8) || (bit_number<64) || (ds->sn[0] == 0)) {
      /* A minor "error" and should perhaps only return -1 to avoid
      * reconnect */
        return -EIO ;
    }
    if((ds->sn[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }
    // if the search was successful then

    ds->LastDiscrepancy = last_zero;
//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
    ds->LastDevice = (last_zero < 0);
    LEVEL_DEBUG("Generic_next_both SN found: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",ds->sn[0],ds->sn[1],ds->sn[2],ds->sn[3],ds->sn[4],ds->sn[5],ds->sn[6],ds->sn[7]) ;
    return 0 ;
}

