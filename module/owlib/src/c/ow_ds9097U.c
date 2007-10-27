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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

static int DS2480_next_both(struct device_search *ds,
							const struct parsedname *pn);
static int DS2480_databit(int sendbit, int *getbit,
						  const struct parsedname *pn);
static int DS2480_reset(const struct parsedname *pn);
static int DS2480_read(BYTE * buf, const size_t size,
					   const struct parsedname *pn);
static int DS2480_write(const BYTE * buf, const size_t size,
						const struct parsedname *pn);
static int DS2480_sendout_data(const BYTE * data, const size_t len,
							   const struct parsedname *pn);
static int DS2480_level(int new_level, const struct parsedname *pn);
static int DS2480_level_low(int new_level, const struct parsedname *pn);
static int DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay,
							const struct parsedname *pn);
static int DS2480_ProgramPulse(const struct parsedname *pn);
static int DS2480_sendout_cmd(const BYTE * cmd, const size_t len,
							  const struct parsedname *pn);
static int DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp,
							   const size_t len,
							   const struct parsedname *pn);
static int DS2480_sendback_data(const BYTE * data, BYTE * resp,
								const size_t len,
								const struct parsedname *pn);
static void DS2480_setroutines(struct interface_routines *f);
static int DS2480_configuration_code( BYTE parameter_code, BYTE value_code, const struct parsedname * pn ) ;
static int DS2480_stop_pulse( BYTE * response, const struct parsedname * pn ) ;

static void DS2480_setroutines(struct interface_routines *f)
{
	f->detect = DS2480_detect;
	f->reset = DS2480_reset;
	f->next_both = DS2480_next_both;
	f->PowerByte = DS2480_PowerByte;
	f->ProgramPulse = DS2480_ProgramPulse;
	f->sendback_data = DS2480_sendback_data;
//    f->sendback_bits = ;
	f->select = NULL;
	f->reconnect = NULL;		// use "detect"
	f->close = COM_close;
	f->transaction = NULL;
	f->flags = 0;
}

/* --------------------------- */
/* DS2480 defines from PDkit   */
/* --------------------------- */

// Mode Commands
#define MODE_DATA                      0xE1
#define MODE_COMMAND                   0xE3
#define MODE_STOP_PULSE                0xF1

// Return byte value
#define RB_CHIPID_MASK                 0x1C
#define RB_RESET_MASK                  0x03
#define RB_1WIRESHORT                  0x00
#define RB_PRESENCE                    0x01
#define RB_ALARMPRESENCE               0x02
#define RB_NOPRESENCE                  0x03

#define RB_BIT_MASK                    0x03
#define RB_BIT_ONE                     0x03
#define RB_BIT_ZERO                    0x00

// Masks for all bit ranges
#define CMD_MASK                       0x80
#define FUNCTSEL_MASK                  0x60
#define BITPOL_MASK                    0x10
#define SPEEDSEL_MASK                  0x0C
#define MODSEL_MASK                    0x02
#define PARMSEL_MASK                   0x70
#define PARMSET_MASK                   0x0E

// Command or config bit
#define CMD_COMM                       0x81
#define CMD_CONFIG                     0x01
#define CMD_RESPONSE                   0x00

// Function select bits
#define FUNCTSEL_BIT                   0x00
#define FUNCTSEL_SEARCHON              0x30
#define FUNCTSEL_SEARCHOFF             0x20
#define FUNCTSEL_RESET                 0x40
#define FUNCTSEL_CHMOD                 0x60

// Bit polarity/Pulse voltage bits
#define BITPOL_ONE                     0x10
#define BITPOL_ZERO                    0x00
#define BITPOL_5V                      0x00
#define BITPOL_12V                     0x10

// One Wire speed bits
#define SPEEDSEL_STD                   0x00
#define SPEEDSEL_FLEX                  0x04
#define SPEEDSEL_OD                    0x08
#define SPEEDSEL_PULSE                 0x0C

// Data/Command mode select bits
#define MODSEL_DATA                    0x00
#define MODSEL_COMMAND                 0x02

// 5V Follow Pulse select bits (If 5V pulse
// will be following the next byte or bit.)
#define PRIME5V_TRUE                   0x02
#define PRIME5V_FALSE                  0x00

// Parameter select bits
#define PARMSEL_PARMREAD               0x00
#define PARMSEL_SLEW                   0x10
#define PARMSEL_12VPULSE               0x20
#define PARMSEL_5VPULSE                0x30
#define PARMSEL_WRITE1LOW              0x40
#define PARMSEL_SAMPLEOFFSET           0x50
#define PARMSEL_ACTIVEPULLUPTIME       0x60
#define PARMSEL_BAUDRATE               0x70

// Pull down slew rate.
#define PARMSET_Slew15Vus              0x00
#define PARMSET_Slew2p2Vus             0x02
#define PARMSET_Slew1p65Vus            0x04
#define PARMSET_Slew1p37Vus            0x06
#define PARMSET_Slew1p1Vus             0x08
#define PARMSET_Slew0p83Vus            0x0A
#define PARMSET_Slew0p7Vus             0x0C
#define PARMSET_Slew0p55Vus            0x0E

// 12V programming pulse time table
#define PARMSET_32us                   0x00
#define PARMSET_64us                   0x02
#define PARMSET_128us                  0x04
#define PARMSET_256us                  0x06
#define PARMSET_512us                  0x08
#define PARMSET_1024us                 0x0A
#define PARMSET_2048us                 0x0C
#define PARMSET_infinite               0x0E

// 5V strong pull up pulse time table
#define PARMSET_16p4ms                 0x00
#define PARMSET_65p5ms                 0x02
#define PARMSET_131ms                  0x04
#define PARMSET_262ms                  0x06
#define PARMSET_524ms                  0x08
#define PARMSET_1p05s                  0x0A
#define PARMSET_2p10s                  0x0C
#define PARMSET_infinite               0x0E

// Write 1 low time
#define PARMSET_Write8us               0x00
#define PARMSET_Write9us               0x02
#define PARMSET_Write10us              0x04
#define PARMSET_Write11us              0x06
#define PARMSET_Write12us              0x08
#define PARMSET_Write13us              0x0A
#define PARMSET_Write14us              0x0C
#define PARMSET_Write15us              0x0E

// Data sample offset and Write 0 recovery time
#define PARMSET_SampOff3us             0x00
#define PARMSET_SampOff4us             0x02
#define PARMSET_SampOff5us             0x04
#define PARMSET_SampOff6us             0x06
#define PARMSET_SampOff7us             0x08
#define PARMSET_SampOff8us             0x0A
#define PARMSET_SampOff9us             0x0C
#define PARMSET_SampOff10us            0x0E

// Active pull up on time
#define PARMSET_PullUp0p0us            0x00
#define PARMSET_PullUp0p5us            0x02
#define PARMSET_PullUp1p0us            0x04
#define PARMSET_PullUp1p5us            0x06
#define PARMSET_PullUp2p0us            0x08
#define PARMSET_PullUp2p5us            0x0A
#define PARMSET_PullUp3p0us            0x0C
#define PARMSET_PullUp3p5us            0x0E

// Baud rate bits
#define PARMSET_9600                   0x00
#define PARMSET_19200                  0x02
#define PARMSET_57600                  0x04
#define PARMSET_115200                 0x06

// DS2480B program voltage available
#define DS2480PROG_MASK                0x20

/* Reset and detect a DS2480B */
/* returns 0=good
   DS2480 sendback error
   COM_write error
   -EINVAL baudrate error
   If no detection, try a DS9097 passive port */
// bus locking at a higher level
int DS2480_detect(struct connection_in *in)
{
	struct parsedname pn;
	BYTE timing = 0xC1;
	int ret;
	BYTE setup[5];

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	DS2480_setroutines(&(in->iroutines));

	/* Open the com port in 9600 Baud.
	 * This will set in->connin.serial.speed which is used below.
	 * It might have been a bug before. */
	if (COM_open(in))
		return -ENODEV;

	// Send BREAK to reset device
	tcsendbreak(in->file_descriptor,0) ;

	// set the FLEX configuration parameters
	// copied shamelessly from Brian Lane's Digitemp
	// default PDSRC = 1.37Vus
	//setup[0] = CMD_CONFIG | PARMSEL_SLEW | PARMSET_Slew1p37Vus;
	// default W1LT = 10us
	setup[1] = CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_Write10us;
	// default DSO/WORT = 8us
	setup[2] = CMD_CONFIG | PARMSEL_SAMPLEOFFSET | PARMSET_SampOff8us;
	// construct the command to read the baud rate (to test command block)
	setup[3] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);
	// also do 1 bit operation (to test 1-Wire block)
	setup[4] = CMD_COMM | FUNCTSEL_BIT |
	  DS2480_baud(in->connin.serial.speed, &pn) | BITPOL_ONE;

	/* Reset the bus and adapter */
	DS2480_reset(&pn);

	// reset modes
	in->connin.serial.UMode = MODSEL_COMMAND;
	in->connin.serial.USpeed = SPEEDSEL_FLEX;

	// set the baud rate to 9600. (Already set to 9600 in COM_open())
	COM_speed(B9600, &pn);

	// send a break to reset the DS2480
	COM_break(&pn);

	// delay to let line settle
	UT_delay(2);

	// flush the buffers
	COM_flush(&pn);

	// send the timing byte
	if (DS2480_write(&timing, 1, &pn))
		return -EIO;

	// delay to let line settle
	UT_delay(4);

	// flush the buffers
	COM_flush(&pn);

	// send the packet
	// read back the response
	if (DS2480_sendback_cmd(setup, setup, 5, &pn))
		return -EIO;

	
	// default W1LT = 10us
	if ( DS2480_configuration_code( PARMSEL_WRITE1LOW, PARMSET_Write10us, &pn ) ) return -EINVAL ;
	// default DSO/WORT = 8us
	if ( DS2480_configuration_code( PARMSEL_SAMPLEOFFSET, PARMSET_SampOff8us, &pn ) ) return -EINVAL;
	// Strong pullup duration = infinite
	if ( DS2480_configuration_code( PARMSEL_5VPULSE, PARMSET_infinite, &pn ) ) return -EINVAL;
	// Program pulse duration = 512usec
	if ( DS2480_configuration_code( PARMSEL_12VPULSE, PARMSET_512us, &pn ) ) return -EINVAL;

	//printf("2480Detect response: %2X %2X %2X %2X %2X\n",setup[0],setup[1],setup[2],setup[3],setup[4]);
	/* Apparently need to reset again to get the version number properly */

	if ((ret = DS2480_reset(&pn))) {
		return ret;
	}
	in->busmode = bus_serial;

	// in->Adapter is set in DS2480_reset from some status bits
	switch (in->Adapter) {
	case adapter_DS9097U2:
	case adapter_DS9097U:
		in->adapter_name = "DS9097U";
		break;
	case adapter_LINK:
	case adapter_LINK_10:
	case adapter_LINK_11:
	case adapter_LINK_12:
		in->adapter_name = "LINK(emulate mode)";
		break;
	default:
		return -ENODEV;
	}
	//printf("2480Detect version=%d\n",in->Adapter) ;
	return 0 ;
	//printf("2480Detect response: %2X %2X %2X %2X %2X %2X\n",setup[0],setup[1],setup[2],setup[3],setup[4]);

	return -EINVAL;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
static int DS2480_configuration_code( BYTE parameter_code, BYTE value_code, const struct parsedname * pn )
{
	BYTE send_code = CMD_CONFIG | parameter_code | value_code ;
	BYTE expected_response = CMD_RESPONSE | parameter_code | value_code ;
	BYTE actual_response ;

	if ( DS2480_sendback_cmd( &send_code, &actual_response, 1, pn ) ) return -EIO ;
	if ( expected_response == actual_response ) return 0 ;
	return -EINVAL ;
}

/* returns baud rate variable, no errors */
int DS2480_baud(speed_t baud, const struct parsedname *pn)
{
	(void) pn;
	switch (baud) {
	case B9600:
		return PARMSET_9600;
	case B19200:
		return PARMSET_19200;
#ifdef B57600
		/* MacOSX support max 38400 in termios.h ? */
	case B57600:
		return PARMSET_57600;
#endif
#ifdef B115200
		/* MacOSX support max 38400 in termios.h ? */
	case B115200:
		return PARMSET_115200;
#endif
	}
	return PARMSET_9600;
}

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// WARNING: Without setting the above global (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
//          to TRUE, this routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B.
/* return 0=good
          1=short
          <0 error
 */
static int DS2480_reset(const struct parsedname *pn)
{
	int ret = 0;
	BYTE buf = (BYTE)(CMD_COMM |
			  FUNCTSEL_RESET |
			  pn->selected_connection->connin.serial.USpeed);

	//printf("DS2480_reset\n");
	// make sure normal level
	if (DS2480_level(MODE_NORMAL, pn))
		return -EIO;
	// flush the buffers
	COM_flush(pn);

	// send the packet
	// read back the 1 byte response
	if (DS2480_sendback_cmd(&buf, &buf, 1, pn))
		return -EIO;
	/* The adapter type is encode in this response byte */
	/* The known values coorespond to the types in enum adapter_type */
	/* Other values are assigned for adapters that don't have this hardcoded value */
	pn->selected_connection->Adapter = (buf & RB_CHIPID_MASK) >> 2;

	switch (buf & RB_RESET_MASK) {
	case RB_1WIRESHORT:
		ret = BUS_RESET_SHORT ;
		break ;
	case RB_NOPRESENCE:
		ret = BUS_RESET_OK ;
		pn->selected_connection->AnyDevices = 0;
		break;
	case RB_PRESENCE:
	case RB_ALARMPRESENCE:
		ret = BUS_RESET_OK ;
		pn->selected_connection->AnyDevices = 1;
		// check if programming voltage available
		pn->selected_connection->ProgramAvailable = ((buf & 0x20) == 0x20);
		if (pn->selected_connection->ds2404_compliance) {
			// extra delay for alarming DS1994/DS2404 complience
			UT_delay(5);
		}
		COM_flush(pn);
		break;
	}
	return ret;
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for new_level are
// as follows:
//
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
//  ULEVEL     - global var - level set

// Returns:    0 GOOD, !0 Error
/* return 0=good
  sendout_cmd,readin
  -EIO response byte doesn't match
 */
static int DS2480_level(int new_level, const struct parsedname *pn)
{
	int ret = DS2480_level_low(new_level, pn);
	if (ret) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
	}
	return ret;
}

static int DS2480_stop_pulse( BYTE * response, const struct parsedname * pn )
{
	BYTE cmd[1] = { MODE_STOP_PULSE, } ;
	// read back the 8 byte response from setting time limit
	return DS2480_sendback_cmd(cmd, response, 1, pn) ;
}

static int DS2480_level_low(int new_level, const struct parsedname *pn)
{
	int ret;
	if (new_level == pn->selected_connection->connin.serial.ULevel) {	// check if need to change level
		return 0;
	} else if (new_level == MODE_NORMAL) {	// check if just putting back to normal
		int docheck = 0;
		BYTE c;
		// check for disable strong pullup step
		if (pn->selected_connection->connin.serial.ULevel == MODE_STRONG5)
			docheck = 1;

		// check if correct mode
		// stop pulse command
		c = MODE_STOP_PULSE;

		// flush the buffers
		COM_flush(pn);

		// send the packet
		if ((ret = DS2480_sendout_cmd(&c, 1, pn)))
			return ret;
		UT_delay(4);

		// read back the 1 byte response
		// check response byte
		if ((ret = DS2480_read(&c, 1, pn))
			|| (ret = ((c & 0xE0) == 0xE0) ? 0 : -EIO))
			return ret;

		// we don't want DS2480_databit to change level, so set ULevel here.
		pn->selected_connection->connin.serial.ULevel = MODE_NORMAL;

		// do extra bit for DS2480 disable strong pullup
		if (!docheck || DS2480_databit(1, &docheck, pn)) {
			STAT_ADD1(DS2480_level_docheck_errors);
			pn->selected_connection->connin.serial.ULevel = MODE_STRONG5;	// it failed! restore ULevel
			return -EIO;
		}
	} else if (new_level == MODE_STRONG5) {	// strong 5 volts
		BYTE b[] = {
			CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_5V,
		};
		// flush the buffers
		COM_flush(pn);
		// send the packet
		if ((ret = DS2480_sendout_cmd(b, 1, pn)))
			return ret;
	} else if (new_level == MODE_PROGRAM) {	// 12 volts
		BYTE b[] = {
			// add the command to begin the pulse
			CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_12V,
		};
		// check if programming voltage available
		if (!pn->selected_connection->ProgramAvailable)
			return 0;
		// flush the buffers
		COM_flush(pn);
		// send the packet
		if ((ret = DS2480_sendout_cmd(b, 1, pn)))
			return ret;
	}
	pn->selected_connection->connin.serial.ULevel = new_level;
	return 0;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and get the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the response is the return bit.
//
// 'sendbit' - the least significant bit is the bit to send
//
// 'getbit' - the least significant bit is the bit received
/* return 0=good
   -EIO bad
 */
static int DS2480_databit(int sendbit, int *getbit,
						  const struct parsedname *pn)
{
	BYTE readbuffer[10], sendpacket[10];
	int ret;
	UINT sendlen = 0;

	// make sure normal level
	if ((ret = DS2480_level(MODE_NORMAL, pn))) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return ret;
	}
	// check if correct mode
	if (pn->selected_connection->connin.serial.UMode != MODSEL_COMMAND) {
		pn->selected_connection->connin.serial.UMode = MODSEL_COMMAND;
		sendpacket[sendlen++] = MODE_COMMAND;
	}
	// construct the command
	sendpacket[sendlen] = (sendbit != 0) ? BITPOL_ONE : BITPOL_ZERO;
	sendpacket[sendlen++] |=
		CMD_COMM | FUNCTSEL_BIT | pn->selected_connection->connin.serial.USpeed;

	// flush the buffers
	COM_flush(pn);

	// send the packet
	if ((ret = DS2480_write(sendpacket, sendlen, pn))
		|| (ret = DS2480_read(readbuffer, 1, pn))) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return ret;
	}
	// interpret the response
	*getbit = ((readbuffer[0] & 0xE0) == 0x80)
		&& ((readbuffer[0] & RB_BIT_MASK) == RB_BIT_ONE);

	return 0;
}

/* search = 0xF0 normal 0xEC alarm */
static int DS2480_next_both(struct device_search *ds,
							const struct parsedname *pn)
{
	int ret;
	int mismatched;
	BYTE sn[8];
	BYTE bitpairs[16];
	BYTE searchon =
		(BYTE) (CMD_COMM | FUNCTSEL_SEARCHON | pn->selected_connection->connin.serial.
				USpeed);
	BYTE searchoff =
		(BYTE) (CMD_COMM | FUNCTSEL_SEARCHOFF | pn->selected_connection->connin.serial.
				USpeed);
	int i;

//printf("NEXT\n");
	if (!(pn->selected_connection->AnyDevices))
		ds->LastDevice = 1;
	if (ds->LastDevice)
		return -ENODEV;

	// build the command stream
	// call a function that may add the change mode command to the buff
	// check if correct mode
	// issue the search command
	// change back to command mode
	// search mode on
	// change back to data mode

	// set the temp Last Descrep to none
	mismatched = -1;

	// add the 16 bytes of the search
	memset(bitpairs, 0, 16);

	// set the bits in the added buffer
	for (i = 0; i < ds->LastDiscrepancy; i++) {
		// before last discrepancy
		UT_set2bit(bitpairs, i, UT_getbit(ds->sn, i) << 1);
	}
	// at last discrepancy
	if (ds->LastDiscrepancy > -1)
		UT_set2bit(bitpairs, ds->LastDiscrepancy, 1 << 1);
	// after last discrepancy so leave zeros

	// flush the buffers
	COM_flush(pn);

	// search ON
	// change back to command mode
	// send the packet
	// search OFF
	if ((ret = BUS_send_data(&(ds->search), 1, pn))
		|| (ret = DS2480_sendout_cmd(&searchon, 1, pn))
		|| (ret = BUS_sendback_data(bitpairs, bitpairs, 16, pn))
		|| (ret = DS2480_sendout_cmd(&searchoff, 1, pn))
		)
		return ret;

	// interpret the bit stream
	for (i = 0; i < 64; i++) {
		// get the SerialNum bit
		UT_setbit(sn, i, UT_get2bit(bitpairs, i) >> 1);
		// check LastDiscrepancy
		if (UT_get2bit(bitpairs, i) == 0x1) {
			mismatched = i;
		}
	}

	// CRC check
	if (CRC8(sn, 8) || (ds->LastDiscrepancy == 63) || (sn[0] == 0))
		return -EIO;

	// successful search
	// check for last one
	if ((mismatched == ds->LastDiscrepancy) || (mismatched == -1))
		ds->LastDevice = 1;

	// copy the SerialNum to the buffer
	memcpy(ds->sn, sn, 8);

	if ((sn[0] & 0x7F) == 0x04) {
		/* We found a DS1994/DS2404 which require longer delays */
		pn->selected_connection->ds2404_compliance = 1;
	}
	// set the count
	ds->LastDiscrepancy = mismatched;

	LEVEL_DEBUG("DS9097U_next_both SN found: " SNformat "\n",
				SNvar(ds->sn));
	return 0;
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
static int DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay,
							const struct parsedname *pn)
{
	int ret;
	BYTE bits = CMD_COMM | FUNCTSEL_BIT | pn->selected_connection->connin.serial.USpeed;
	BYTE cmd[] = {
		// bit 1
		((byte & 0x01) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 2
		((byte & 0x02) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 3
		((byte & 0x04) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 4
		((byte & 0x08) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 5
		((byte & 0x10) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 6
		((byte & 0x20) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 7
		((byte & 0x40) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_FALSE,
		// bit 8
		((byte & 0x80) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_TRUE,
	};
	BYTE respbits[8];
	BYTE response[1] ;

	// flush the buffers
	COM_flush(pn);

	// send the packet
	// read back the 8 byte response from setting time limit
	ret = DS2480_sendback_cmd(cmd, respbits, 8, pn) ;

	UT_delay(delay);

	/* Make sure it's set back to normal mode since the command might be sent
	 * correctly, but response is received with errors. Otherwise it will be
	 * stuck in MODE_STRONG5. (but ULevel will be set to MODE_NORMAL)
	 * Just hope the command will return ok and ignore return value.
	 * Other read/write/dir functions will try to set it back to MODE_NORMAL
	 * if it fails the first time here.
	 */

	// indicate the port is now at power delivery
	pn->selected_connection->connin.serial.ULevel = MODE_STRONG5;

	// return to normal level
//	DS2480_level(MODE_NORMAL, pn);
	DS2480_stop_pulse(response,pn) ;

	resp[0] = ((respbits[7] & 1) << 7)
		| ((respbits[6] & 1) << 6)
		| ((respbits[5] & 1) << 5)
		| ((respbits[4] & 1) << 4)
		| ((respbits[3] & 1) << 3)
		| ((respbits[2] & 1) << 2)
		| ((respbits[1] & 1) << 1)
		| ((respbits[0] & 1)     );

	return ret;
}

/* Send a 12v 480usec pulse on the 1wire bus to program the EPROM */
/* Note, DS2480_reset must have been called at once in the past for ProgramAvailable setting */
/* returns 0 if good
   -EINVAL if not program pulse available
   -EIO on error
 */
static int DS2480_ProgramPulse(const struct parsedname *pn)
{
	int ret;
	BYTE cmd[] = { CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE,	};
	BYTE response_mask = 0xFC ;
	BYTE resp[1];

	COM_flush(pn);

	// send the packet
	// read back the 8 byte response from setting time limit
	ret = DS2480_sendback_cmd(cmd, resp, 1, pn) ;

	if (ret==0) {
		UT_delay_us(520);
	}

	// return to normal level
	DS2480_level(MODE_NORMAL, pn);

	if (ret) return ret ;

	if ( (resp[0]&response_mask) != (cmd[0]&response_mask) ) {
		ret = -EIO ;
	}

	return ret;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
 */
static int DS2480_write(const BYTE * buf, const size_t size,
						const struct parsedname *pn)
{
	ssize_t r, sl = size;

	while (sl > 0) {
		if (!pn->selected_connection)
			break;
		r = write(pn->selected_connection->file_descriptor, &buf[size - sl], sl);
		if (r < 0) {
			if (errno == EINTR) {
				STAT_ADD1_BUS(e_bus_write_errors, pn->selected_connection);
				continue;
			}
			break;
		}
		sl -= r;
	}
	if (pn->selected_connection) {
		tcdrain(pn->selected_connection->file_descriptor);
		gettimeofday(&(pn->selected_connection->bus_write_time), NULL);
	}
	if (sl > 0) {
		STAT_ADD1_BUS(e_bus_timeouts, pn->selected_connection);
		return -EIO;
	}
	return 0;
}

/* Assymetric */
/* Read from DS2480 with timeout on each character */
// NOTE: from PDkit, reead 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK
// returns 0=good 1=bad
/* return 0=good,
          -errno = read error
          -EINTR = timeout
 */
static int DS2480_read(BYTE * buf, const size_t size,
					   const struct parsedname *pn)
{
	fd_set fdset;
	size_t rl = size;
	ssize_t r;
	struct timeval tval;
	int rc = 0;

	while (rl > 0) {
		if (!pn->selected_connection) {
			rc = -EIO;
			STAT_ADD1(DS2480_read_null);
			break;
		}
		// set a descriptor to wait for a character available
		FD_ZERO(&fdset);
		FD_SET(pn->selected_connection->file_descriptor, &fdset);
		tval.tv_sec = Global.timeout_serial;
		tval.tv_usec = 0;
		/* This timeout need to be pretty big for some reason.
		 * Even commands like DS2480_reset() fails with too low
		 * timeout. I raise it to 0.5 seconds, since it shouldn't
		 * be any bad experience for any user... Less read and
		 * timeout errors for users with slow machines. I have seen
		 * 276ms delay on my Coldfire board.
		 *
		 * DS2480_reset()
		 *   DS2480_sendback_cmd()
		 *     DS2480_sendout_cmd()
		 *       DS2480_write()
		 *         write()
		 *         tcdrain()   (all data should be written on serial port)
		 *     DS2480_read()
		 *       select()      (waiting 40ms should be enough!)
		 *       read()
		 *
		 */
		/* Hehehe. Even .5 sec wasn't long enough for a MIPS embedded
		 * system with USB-serial converter
		 */

		// if byte available read or return bytes read
		rc = select(pn->selected_connection->file_descriptor + 1, &fdset, NULL, NULL, &tval);
		if (rc > 0) {
			if (FD_ISSET(pn->selected_connection->file_descriptor, &fdset) == 0) {
				rc = -EIO;		/* error */
				STAT_ADD1(DS2480_read_fd_isset);
				break;
			}
			update_max_delay(pn);
			r = read(pn->selected_connection->file_descriptor, &buf[size - rl], rl);
			if (r < 0) {
				if (errno == EINTR) {
					/* read() was interrupted, try again */
					STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
					continue;
				}
				rc = -errno;	/* error */
				STAT_ADD1(DS2480_read_read);
				break;
			}
			rl -= r;
		} else if (rc < 0) {
			if (errno == EINTR) {
				/* select() was interrupted, try again */
				STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
				continue;
			}
			STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
			return -EINTR;
		} else {
			STAT_ADD1_BUS(e_bus_timeouts, pn->selected_connection);
			return -EINTR;
		}
	}
	if (rl > 0) {
		STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
		return rc;				/* error */
	}
	return 0;
}

//
// DS2480_sendout_cmd
//  Send a command but expect no response
//  puts into command mode if needed.
/* return 0=good
   COM_write
 */
static int DS2480_sendout_cmd(const BYTE * cmd, const size_t len,
							  const struct parsedname *pn)
{
	int ret;
	BYTE mc = MODE_COMMAND;
	if (pn->selected_connection->connin.serial.UMode != MODSEL_COMMAND) {
		// change back to command mode
		pn->selected_connection->connin.serial.UMode = MODSEL_COMMAND;
		if ((ret = DS2480_write(&mc, 1, pn)))
			return ret;
		if ((ret = DS2480_write(cmd, (unsigned) len, pn)))
			return ret;
	} else {
		if ((ret = DS2480_write(cmd, (unsigned) len, pn))) {
			STAT_ADD1(DS2480_sendout_cmd_errors);
			return ret;
		}
	}
	return 0;
}

//
// DS2480_sendback_cmd
//  Send a command and return response block
//  puts into command mode if needed.
/* return 0=good
   sendback_cmd,sendout_cmd,readin
 */
static int DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp,
							   const size_t len,
							   const struct parsedname *pn)
{
	int ret;
	if (len > 16) {
		int clen = len - (len >> 1);
		if ((ret = DS2480_sendback_cmd(cmd, resp, clen, pn)))
			return ret;
		if ((ret =
			 DS2480_sendback_cmd(&cmd[clen], &resp[clen], len >> 1, pn)))
			return ret;
	} else {
		ret = DS2480_sendout_cmd(cmd, len, pn);
		if (ret == 0)
			ret = DS2480_read(resp, len, pn);
		if (ret) {
			STAT_ADD1(DS2480_sendback_cmd_errors);
		}
	}
	return ret;
}


// DS2480_sendout_data
//  Send data but expect no response
//  puts into data mode if needed.
// Repeat magic MODE_COMMAND byte to show true data
/* return 0=good
   COM_write, sendout_data
 */
static int DS2480_sendout_data(const BYTE * data, const size_t len,
							   const struct parsedname *pn)
{
	int ret;
	if (pn->selected_connection->connin.serial.UMode != MODSEL_DATA) {
		BYTE md = MODE_DATA;
		// change back to command mode
		pn->selected_connection->connin.serial.UMode = MODSEL_DATA;
		if ((ret = DS2480_write(&md, 1, pn))) {
			STAT_ADD1(DS2480_sendout_data_errors);
			return ret;
		}
	}
	if (len > 16) {
		int dlen = len - (len >> 1);
		if ((ret = DS2480_sendout_data(data, dlen, pn)))
			return ret;
		if ((ret = DS2480_sendout_data(&data[dlen], len >> 1, pn)))
			return ret;
	} else {
		BYTE data2[32];
		size_t i;
		UINT j = 0;
		for (i = 0; i < len; ++i) {
			data2[j++] = data[i];
			if (data[i] == MODE_COMMAND)
				data2[j++] = MODE_COMMAND;
		}
		if ((ret = DS2480_write(data2, j, pn))) {
			STAT_ADD1(DS2480_sendout_data_errors);
			return ret;
		}
	}
	return 0;
}

//
// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int DS2480_sendback_data(const BYTE * data, BYTE * resp,
								const size_t len,
								const struct parsedname *pn)
{
	int ret;
	ret = DS2480_sendout_data(data, len, pn);
	if (ret == 0)
		ret = DS2480_read(resp, len, pn);
	if (ret) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
	}
	return ret;
}
