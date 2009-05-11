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

static int DS2480_next_both(struct device_search *ds, const struct parsedname *pn);
//static int DS2480_databit(int sendbit, int *getbit, const struct parsedname *pn);
static int DS2480_reset(const struct parsedname *pn);
static int DS2480_big_reset(const struct parsedname *pn) ;
static int DS2480_read(BYTE * buf, const size_t size, const struct parsedname *pn);
static int DS2480_write(const BYTE * buf, const size_t size, const struct parsedname *pn);
static int DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static int DS2480_ProgramPulse(const struct parsedname *pn);
static int DS2480_sendout_cmd(const BYTE * cmd, const size_t len, const struct parsedname *pn);
static int DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, const struct parsedname *pn);
static int DS2480_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void DS2480_setroutines(struct connection_in *in);
static int DS2480_configuration_write(BYTE parameter_code, BYTE value_code, const struct parsedname *pn);
static int DS2480_configuration_read(BYTE parameter_code, BYTE value_code, const struct parsedname *pn);
static int DS2480_stop_pulse(BYTE * response, const struct parsedname *pn);
static int DS2480_reset_once(const struct parsedname *pn) ;
static int DS2480_set_baud(const struct parsedname *pn) ;
static void DS2480_set_baud_control(const struct parsedname *pn) ;
static BYTE DS2480b_speed_byte( const struct parsedname * pn ) ;

static void DS2480_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS2480_detect;
	in->iroutines.reset = DS2480_reset;
	in->iroutines.next_both = DS2480_next_both;
	in->iroutines.PowerByte = DS2480_PowerByte;
	in->iroutines.ProgramPulse = DS2480_ProgramPulse;
	in->iroutines.sendback_data = DS2480_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = DS2480_big_reset ;
	in->iroutines.close = COM_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = 0;
	in->bundling_length = UART_FIFO_SIZE;
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
#define CMD_COMM_RESPONSE              0x80
#define CMD_CONFIG                     0x01
#define CMD_CONFIG_RESPONSE            0x00

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
#define PARMSET_REVERSE_POLARITY       0x08

// Search defines
#define SEARCH_BIT_ON		0x01
#define	DS2404_family	0x04

// Could probably be UART_FIFO_SIZE (160) but that will take some testing
#define MAX_SEND_SIZE			64

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
	int ret;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	DS2480_setroutines(in);

	in->speed = bus_speed_slow ;
	in->flex = Globals.serial_flextime ? bus_yes_flex : bus_no_flex ;

	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	in->connin.serial.reverse_polarity = Globals.serial_reverse ;
	in->baud = Globals.baud ;

	ret = DS2480_big_reset(&pn) ;
	if ( ret ) {
		return ret ;
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
	return 0;
}

// do the com port and configuration stuff
static int DS2480_big_reset(const struct parsedname *pn)
{
	int ret ;
	BYTE single_bit = CMD_COMM | BITPOL_ONE |  DS2480b_speed_byte(pn) ;
	BYTE single_bit_response ;
	BYTE reset_byte = (BYTE) ( CMD_COMM | FUNCTSEL_RESET | SPEEDSEL_STD );

	// Open the com port in 9600 Baud.
	if (COM_open(pn->selected_connection)) {
		return -ENODEV;
	}

	// send a break to reset the DS2480
	COM_break(pn->selected_connection);

	// It's in command mode now
	pn->selected_connection->connin.serial.mode = ds2480b_command_mode ;

	// send the timing byte (A reset command at 9600 baud)
	DS2480_write( &reset_byte, 1, pn ) ;

	// delay to let line settle
	UT_delay(4);
	// flush the buffers
	COM_flush(pn->selected_connection);
	// ignore response
	COM_slurp( pn->selected_connection->file_descriptor ) ;
	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	pn->selected_connection->changed_bus_settings = 1 ; // Force a mode change
	// Send a reset again
	BUS_reset(pn) ;

	// delay to let line settle
	UT_delay(4);
	// flush the buffers
	COM_flush(pn->selected_connection);
	// ignore response
	COM_slurp( pn->selected_connection->file_descriptor ) ;
	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	pn->selected_connection->changed_bus_settings = 1 ; // Force a mode change
	// Send a reset again
	BUS_reset(pn) ;

	// delay to let line settle
	UT_delay(4);

	// default W1LT = 10us (write-1 low time)
	if (DS2480_configuration_write(PARMSEL_WRITE1LOW, PARMSET_Write10us, pn)) {
		return -EINVAL;
	}
	// default DSO/WORT = 8us (data sample offset / write 0 recovery time )
	if (DS2480_configuration_write(PARMSEL_SAMPLEOFFSET, PARMSET_SampOff8us, pn)) {
		return -EINVAL;
	}
	// Strong pullup duration = infinite
	if (DS2480_configuration_write(PARMSEL_5VPULSE, PARMSET_infinite, pn)) {
		return -EINVAL;
	}
	// Program pulse duration = 512usec
	if (DS2480_configuration_write(PARMSEL_12VPULSE, PARMSET_512us, pn)) {
		return -EINVAL;
	}

	// Send a single bit
	// The datasheet wants this
	if (DS2480_sendback_cmd(&single_bit, &single_bit_response, 1, pn)) {
		return -EIO;
	}

	/* Apparently need to reset again to get the version number properly */
	if ((ret = DS2480_reset(pn))<0) {
		return ret;
	}

	return 0 ;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
static int DS2480_configuration_write(BYTE parameter_code, BYTE value_code, const struct parsedname *pn)
{
	BYTE send_code = CMD_CONFIG | parameter_code | value_code;
	BYTE expected_response = CMD_CONFIG_RESPONSE | parameter_code | value_code;
	BYTE actual_response;

	if (DS2480_sendback_cmd(&send_code, &actual_response, 1, pn)) {
		return -EIO;
	}
	if (expected_response == actual_response) {
		return 0;
	}
    LEVEL_DEBUG("wrong response (%.2X not %.2X)\n",actual_response,expected_response) ;
    return -EINVAL;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
static int DS2480_configuration_read(BYTE parameter_code, BYTE value_code, const struct parsedname *pn)
{
	BYTE send_code = CMD_CONFIG | PARMSEL_PARMREAD | (parameter_code>>3);
	BYTE expected_response = CMD_CONFIG_RESPONSE | PARMSEL_PARMREAD | value_code;
	BYTE actual_response;

	if (DS2480_sendback_cmd(&send_code, &actual_response, 1, pn)) {
		return -EIO;
	}
	if (expected_response == actual_response) {
		return 0;
	}
    LEVEL_DEBUG("wrong response (%.2X not %.2X)\n",actual_response,expected_response) ;
    return -EINVAL;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
// Set Baud rate -- can't use DS2480_conficuration_code because return byte is in a different speed
static void DS2480_set_baud_control(const struct parsedname *pn)
{
	// restrict allowable baud rates based on device capabilities
	OW_BaudRestrict( &(pn->selected_connection->baud), B9600, B19200, B57600, B115200, 0 ) ;

	if ( DS2480_set_baud(pn) == 0 ) {
		return ;
	}
	LEVEL_DEBUG("Failed first attempt at resetting baud rate of bus master %s\n",SAFESTRING(pn->selected_connection->name)) ;

	if ( DS2480_set_baud(pn) == 0 ) {
		return ;
	}
	LEVEL_DEBUG("Failed second attempt at resetting baud rate of bus master %s\n",SAFESTRING(pn->selected_connection->name)) ;

	// uh oh -- undefined state -- not sure what the bus speed is.
	pn->selected_connection->reconnect_state = reconnect_error ;
	pn->selected_connection->baud = B9600 ;
	++pn->selected_connection->changed_bus_settings ;
	return;
}

static int DS2480_set_baud(const struct parsedname *pn)
{
	BYTE value_code ;
	BYTE send_code ;

	// Find rate parameter
	switch ( pn->selected_connection->baud ) {
		case B9600:
			value_code = PARMSET_9600 ;
			break ;
		case B19200:
			value_code = PARMSET_19200 ;
			break ;
#ifdef B57600
		/* MacOSX support max 38400 in termios.h ? */
		case B57600:
			value_code = PARMSET_57600 ;
			break ;
#endif
#ifdef B115200
		/* MacOSX support max 38400 in termios.h ? */
		case B115200:
			value_code = PARMSET_115200 ;
			break ;
#endif
	}

	// Add polarity
	if ( pn->selected_connection->connin.serial.reverse_polarity ) {
		value_code |= PARMSET_REVERSE_POLARITY ;
	}
	send_code = CMD_CONFIG | PARMSEL_BAUDRATE | value_code;

	// Send configuration change
	COM_flush(pn->selected_connection);
	UT_delay(5);
	if (DS2480_sendout_cmd(&send_code, 1, pn)) {
		// uh oh -- undefined state -- not sure what the bus speed is.
		return 1;
	}

	// Change OS view of rate
	UT_delay(5);
	COM_speed(pn->selected_connection->baud,pn->selected_connection) ;
	UT_delay(5);
	COM_slurp(pn->selected_connection->file_descriptor);

	// Check rate
	if ( DS2480_configuration_read( PARMSEL_BAUDRATE, value_code, pn ) ) {
		// Can't read new setting
		return 1 ;
	}

	return 0 ;
}

static BYTE DS2480b_speed_byte( const struct parsedname * pn )
{
	if ( pn->selected_connection->speed == bus_speed_overdrive ) {
		return SPEEDSEL_OD ;
	} else if ( pn->selected_connection->flex == bus_yes_flex ) {
		return SPEEDSEL_FLEX ;
	} else {
		return SPEEDSEL_STD ;
	}
}

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
//          This routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B.
/* return 0=good
          1=short
          <0 error
 */
static int DS2480_reset(const struct parsedname *pn)
{
	int ret ;

	if (pn->selected_connection->changed_bus_settings > 0) {
		--pn->selected_connection->changed_bus_settings ;
		DS2480_set_baud_control(pn);	// reset paramters
	}

	ret = DS2480_reset_once(pn) ;

	// Some kind of problem (not including bus short)
	// Try simple fixes
	if ( ret < 0 ) {
		BYTE dummy[1] ; //we don't check the pulse stop, since it's only a whim.
		// perhaps the DS9097U wasn't in command mode, so missed the reset
		// force a mode switch
		pn->selected_connection->connin.serial.mode = ds2480b_data_mode ;
		// Also make sure no power or programming pulse active
		DS2480_stop_pulse(dummy,pn) ;
		// And now reset again
		ret = DS2480_reset_once(pn) ;
	}
	return ret ;
}

static int DS2480_reset_once(const struct parsedname *pn)
{
	int ret = 0;
	BYTE reset_byte = (BYTE) ( CMD_COMM | FUNCTSEL_RESET | DS2480b_speed_byte(pn) );
	BYTE reset_response ;

	//printf("DS2480_reset\n");
	// flush the buffers
	COM_flush(pn->selected_connection);

	// send the packet
	// read back the 1 byte response
	if (DS2480_sendback_cmd(&reset_byte, &reset_response, 1, pn)) {
		return -EIO;
	}
	/* The adapter type is encode in this response byte */
	/* The known values coorespond to the types in enum adapter_type */
	/* Other values are assigned for adapters that don't have this hardcoded value */
	pn->selected_connection->Adapter = (reset_response & RB_CHIPID_MASK) >> 2;

	switch (reset_response & RB_RESET_MASK) {
	case RB_1WIRESHORT:
		ret = BUS_RESET_SHORT;
		break;
	case RB_NOPRESENCE:
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 0;
		break;
	case RB_PRESENCE:
	case RB_ALARMPRESENCE:
		ret = BUS_RESET_OK;
		pn->selected_connection->AnyDevices = 1;
		// check if programming voltage available
		pn->selected_connection->ProgramAvailable = ((reset_response & PARMSEL_12VPULSE) == PARMSEL_12VPULSE);
		if (pn->selected_connection->ds2404_compliance) {
			// extra delay for alarming DS1994/DS2404 complience
			UT_delay(5);
		}
		COM_flush(pn->selected_connection);
		break;

	}
	return ret;
}

/* search = normal and alarm */
static int DS2480_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret;
	int mismatched;
	BYTE sn[8];
	BYTE bitpairs[16];
	BYTE searchon = (BYTE) ( CMD_COMM | FUNCTSEL_SEARCHON | DS2480b_speed_byte(pn) );
	BYTE searchoff = (BYTE) ( CMD_COMM | FUNCTSEL_SEARCHOFF | DS2480b_speed_byte(pn) );
	int i;

	if (!(pn->selected_connection->AnyDevices)) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

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
	if (ds->LastDiscrepancy > -1) {
		UT_set2bit(bitpairs, ds->LastDiscrepancy, 1 << 1);
	}
	// after last discrepancy so leave zeros

	// flush the buffers
	COM_flush(pn->selected_connection);

	// search ON
	// change back to command mode
	// send the packet
	// search OFF
	if ((ret = BUS_send_data(&(ds->search), 1, pn))
		|| (ret = DS2480_sendout_cmd(&searchon, 1, pn))
		|| (ret = BUS_sendback_data(bitpairs, bitpairs, 16, pn))
		|| (ret = DS2480_sendout_cmd(&searchoff, 1, pn))
		) {
		return ret;
	}

	// interpret the bit stream
	for (i = 0; i < 64; i++) {
		// get the SerialNum bit
		UT_setbit(sn, i, UT_get2bit(bitpairs, i) >> 1);
		// check LastDiscrepancy
		if (UT_get2bit(bitpairs, i) == SEARCH_BIT_ON ) {
			mismatched = i;
		}
	}

	// CRC check
	if (CRC8(sn, 8) || (ds->LastDiscrepancy == 63) || (sn[0] == 0)) {
		if ( sn[0]==0xFF && sn[1]==0xFF && sn[2]==0xFF && sn[3]==0xFF && sn[4]==0xFF && sn[5]==0xFF && sn[6]==0xFF && sn[7]==0xFF ) {
			// special case for no alarm present
			return -ENODEV ;
		}
		return -EIO;
	}

	// successful search
	// check for last one
	if ((mismatched == ds->LastDiscrepancy) || (mismatched == -1)) {
		ds->LastDevice = 1;
	}

	// copy the SerialNum to the buffer
	memcpy(ds->sn, sn, 8);

	if ((sn[0] & 0x7F) == DS2404_family) {
		/* We found a DS1994/DS2404 which require longer delays */
		pn->selected_connection->ds2404_compliance = 1;
	}
	// set the count
	ds->LastDiscrepancy = mismatched;

	LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(ds->sn));
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
static int DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	int ret;
	BYTE bits = CMD_COMM | FUNCTSEL_BIT | DS2480b_speed_byte(pn) ;
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
	BYTE response[1];

	// flush the buffers
	COM_flush(pn->selected_connection);

	// send the packet
	// read back the 8 byte response from sending the 5V pulse
	ret = DS2480_sendback_cmd(cmd, respbits, 8, pn);

	UT_delay(delay);

	// return to normal level
	DS2480_stop_pulse(response, pn);

	resp[0] = ((respbits[7] & 1) << 7)
		| ((respbits[6] & 1) << 6)
		| ((respbits[5] & 1) << 5)
		| ((respbits[4] & 1) << 4)
		| ((respbits[3] & 1) << 3)
		| ((respbits[2] & 1) << 2)
		| ((respbits[1] & 1) << 1)
		| ((respbits[0] & 1));

	return ret;
}

static int DS2480_stop_pulse(BYTE * response, const struct parsedname *pn)
{
	BYTE cmd[1] = { MODE_STOP_PULSE, };
	// read back the 8 byte response from setting time limit
	return DS2480_sendback_cmd(cmd, response, 1, pn);
}

/* Send a 12v 480usec pulse on the 1wire bus to program the EPROM */
// returns 0 if good
static int DS2480_ProgramPulse(const struct parsedname *pn)
{
	int ret;
	BYTE cmd[] = { CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE, };
	BYTE response_mask = 0xFC;
	BYTE command_resp[1];
	BYTE stop_pulse[1] ;

	COM_flush(pn->selected_connection);

	// send the packet
	// read back the 8 byte response from sending the 12V pilse
	ret = DS2480_sendback_cmd(cmd, command_resp, 1, pn);

	if (ret == 0) {
		UT_delay_us(520);
	}
	// return to normal level
	DS2480_stop_pulse(stop_pulse, pn);

	if (ret) {
		return ret;
	}

	if ((command_resp[0] & response_mask) != (cmd[0] & response_mask)) {
		ret = -EIO;
	}

	return ret;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
 */
static int DS2480_write(const BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_write( buf, size, pn->selected_connection );
}

/* Assymetric */
/* Read from DS2480 with timeout on each character */
// NOTE: from PDkit, read 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK (emulation mode)
// returns 0=good 1=bad
static int DS2480_read(BYTE * buf, const size_t size, const struct parsedname *pn)
{
	return COM_read( buf, size, pn->selected_connection ) ;
}

//
// DS2480_sendout_cmd
//  Send a command but expect no response
//  puts into command mode if needed.
// Note most use is through DS2480_sendback_command
// The only uses for this alone are the single byte search accelerators
// so size checking will be done in sendback
// return 0=good
static int DS2480_sendout_cmd(const BYTE * cmd, const size_t len, const struct parsedname *pn)
{
	if (pn->selected_connection->connin.serial.mode == ds2480b_command_mode ) {
		// already in command mode -- very easy
		return DS2480_write(cmd, (unsigned) len, pn);
	} else {
		// need to switch to command mode
		// add to current string for efficiency
		BYTE cmd_plus[len+1] ;
		cmd_plus[0] = MODE_COMMAND ;
		memcpy( &cmd_plus[1], cmd, len ) ;
		// MODE_COMMAND is one of the reservred commands that does not generate a response byte
		// page 6 of datasheet: http://datasheets.maxim-ic.com/en/ds/DS2480B.pdf
		pn->selected_connection->connin.serial.mode = ds2480b_command_mode ;
		return DS2480_write(cmd_plus, (unsigned) len+1, pn);
	}
}

//
// DS2480_sendback_cmd
//  Send a command and return response block
//  puts into command mode if needed.
// return 0=good
static int DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	size_t bytes_so_far = 0 ;
	size_t extra_byte_for_modeshift  = (pn->selected_connection->connin.serial.mode != ds2480b_command_mode) ? 1 : 0 ;

	while ( bytes_so_far < len ) {
		int ret;
		size_t bytes_this_segment = len - bytes_so_far ;
		if ( bytes_this_segment > MAX_SEND_SIZE - extra_byte_for_modeshift ) {
			bytes_this_segment = MAX_SEND_SIZE - extra_byte_for_modeshift ;
			extra_byte_for_modeshift = 0 ;
		}
		ret = DS2480_sendout_cmd( &cmd[bytes_so_far], bytes_this_segment, pn);
		if ( ret ) {
            LEVEL_DEBUG("write error\n") ;
			return ret ;
		}
		ret = DS2480_read(       &resp[bytes_so_far], bytes_this_segment, pn);
		if ( ret ) {
            LEVEL_DEBUG("read error\n") ;
            return ret ;
		}
		bytes_so_far += bytes_this_segment ;
	}
	return 0 ;
}

//
// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
// Repeat magic MODE_COMMAND byte to show true data
// return 0=good
static int DS2480_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	BYTE sendout[MAX_SEND_SIZE] ;
	size_t bytes_this_segment = 0 ;
	size_t bytes_at_segment_start = 0 ;
	size_t bytes_from_list = 0 ;

	// skip if nothing
	// don't even switch to data mode
	if ( len == 0 ) {
		return 0 ;
	}

	// switch mode if needed
	if (pn->selected_connection->connin.serial.mode != ds2480b_data_mode) {
		// this is one of the "reserved commands" that does not generate a resonse byte
		// page 6 of the datasheet http://datasheets.maxim-ic.com/en/ds/DS2480B.pdf
		sendout[bytes_this_segment++] = MODE_DATA;
		// change flag to data mode
		pn->selected_connection->connin.serial.mode = ds2480b_data_mode;
	}

	while ( bytes_from_list < len )
	{
		BYTE current_char = data[bytes_from_list++] ;
		// transfer this byte
		sendout[ bytes_this_segment++ ] = current_char ;
		// see about "doubling"
		if ( current_char == MODE_COMMAND ) {
			sendout[ bytes_this_segment++ ] = current_char ;
		}

		// Should we send off this data so far and queue up more?
		// need room for potentially 2 characters
		if ( bytes_this_segment > MAX_SEND_SIZE-2 || bytes_from_list == len ) {
			// write out
			int ret = DS2480_write(sendout, bytes_this_segment, pn ) ;
			if ( ret ) {
				return ret ;
			}

			// read in
			// note that read and write sizes won't match because of doubled bytes and mode shift
			ret = DS2480_read(&resp[bytes_at_segment_start], bytes_from_list - bytes_at_segment_start, pn);
			if ( ret ) {
				return ret ;
			}

			// move indexes for next segment
			bytes_at_segment_start = bytes_from_list ;
			bytes_this_segment = 0 ;
		}
	}
	return 0 ;
}
