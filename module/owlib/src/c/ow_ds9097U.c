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

/* This is the busmaster code for the DS2480B-based DS9097U
 * The serial adapter from Dallas Maxim
 */ 
 
/* Multimaster:
 * In general the link is DS9097U a multimaster design (only one bus master per serial or telnet port)
 * None the less, all settings are assigned to the head line
 */
 
 


#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

static RESET_TYPE DS2480_reset(const struct parsedname *pn);
static enum search_status DS2480_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD DS2480_PowerBit(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD DS2480_ProgramPulse(const struct parsedname *pn);
static GOOD_OR_BAD DS2480_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD DS2480_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn);
static GOOD_OR_BAD DS2480_reconnect(const struct parsedname * pn);
static void DS2480_close(struct connection_in *in) ;

static void DS2480_setroutines(struct connection_in *in);

static GOOD_OR_BAD DS2480_detect_serial(struct connection_in *in) ;
static RESET_TYPE DS2480_reset_in(struct connection_in * in);
static GOOD_OR_BAD DS2480_initialize_repeatedly(struct connection_in * in);
static GOOD_OR_BAD DS2480_big_reset(struct connection_in * in) ;
static GOOD_OR_BAD DS2480_big_reset_serial(struct connection_in * in) ;
static void DS2480_adapter(struct connection_in *in) ;
static GOOD_OR_BAD DS2480_big_configuration(struct connection_in * in) ;
static GOOD_OR_BAD DS2480_read(BYTE * buf, const size_t size, struct connection_in * in);
static GOOD_OR_BAD DS2480_write(const BYTE * buf, const size_t size, struct connection_in * in);
static GOOD_OR_BAD DS2480_sendout_cmd(const BYTE * cmd, const size_t len, struct connection_in * in);
static GOOD_OR_BAD DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, struct connection_in * in);
static GOOD_OR_BAD DS2480_configuration_write(BYTE parameter_code, BYTE value_code, struct connection_in * in);
static GOOD_OR_BAD DS2480_configuration_read(BYTE parameter_code, BYTE value_code, struct connection_in * in);
static GOOD_OR_BAD DS2480_stop_pulse(BYTE * response, struct connection_in * in);
static RESET_TYPE DS2480_reset_once(struct connection_in * in) ;
static GOOD_OR_BAD DS2480_set_baud(struct connection_in * in) ;
static void DS2480_set_baud_control(struct connection_in * in) ;
static BYTE DS2480b_speed_byte( struct connection_in * in ) ;
static void DS2480_flush( const struct connection_in * in ) ;
static void DS2480_slurp( struct connection_in * in ) ;

static void DS2480_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS2480_detect;
	in->iroutines.reset = DS2480_reset;
	in->iroutines.next_both = DS2480_next_both;
	in->iroutines.PowerByte = DS2480_PowerByte;
	in->iroutines.PowerBit = DS2480_PowerBit;
	in->iroutines.ProgramPulse = DS2480_ProgramPulse;
//	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_data = DS2480_sendback_data;
    in->iroutines.sendback_bits = DS2480_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = DS2480_reconnect ;
	in->iroutines.close = DS2480_close;
	in->iroutines.flags = ADAP_FLAG_default;
	in->bundling_length = UART_FIFO_SIZE;
}

// Number of times to try init (from digitemp code))
#define DS9097U_INIT_CYCLES   3

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
/* returns 0=good */
// bus locking at a higher level
GOOD_OR_BAD DS2480_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	if (pin->init_data == NULL) {
		return gbBAD;
	} else {
		SOC(in)->devicename = owstrdup(pin->init_data) ;
	}

	/* Set up low-level routines */
	DS2480_setroutines(in);

	in->overdrive = 0 ;
	in->flex = Globals.serial_flextime ;
	pin->busmode = bus_serial;

	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	in->master.serial.reverse_polarity = Globals.serial_reverse ;
	COM_set_standard( in ) ; // standard COM port settings
	
	// first pass with hardware flow control
	RETURN_GOOD_IF_GOOD( DS2480_detect_serial(in) ) ;

	SOC(in)->flow = flow_second; // flow control
	RETURN_BAD_IF_BAD(COM_change(in)) ;
	return DS2480_detect_serial(in) ;
}

// setting for serial port already made
static GOOD_OR_BAD DS2480_detect_serial(struct connection_in *in)
{
	SOC(in)->state = cs_virgin ;
	if ( BAD(DS2480_initialize_repeatedly(in)) ) {
		LEVEL_DEBUG("Could not initilize the DS9097U even after several tries") ;
		COM_close(in) ;
		return gbBAD ;
	}

	DS2480_adapter(in) ;
	return gbGOOD ;
}

// Make several attempts to initialize -- based on Digitemp example
static GOOD_OR_BAD DS2480_initialize_repeatedly(struct connection_in * in)
{
	int init_cycles ;

	for ( init_cycles = 0 ; init_cycles < DS9097U_INIT_CYCLES ; ++init_cycles ) {
		LEVEL_DEBUG("Attempt %d of %d to initialize the DS9097U",init_cycles,DS9097U_INIT_CYCLES) ;
		RETURN_GOOD_IF_GOOD( DS2480_big_reset(in) ) ;
	}

	return gbBAD ;
}

static void DS2480_adapter(struct connection_in *in)
{
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
	case adapter_LINK_13:
	case adapter_LINK_14:
	case adapter_LINK_other:
		in->adapter_name = "LINK(emulate mode)";
		break;
	default:
		in->adapter_name = "DS2480B based";
		break;
	}
}

static GOOD_OR_BAD DS2480_reconnect(const struct parsedname * pn)
{
	LEVEL_DEBUG("Attempting reconnect on %s",SAFESTRING(SOC(pn->selected_connection)->devicename));
	return DS2480_big_reset(pn->selected_connection) ;
}

// do the com port and configuration stuff
static GOOD_OR_BAD DS2480_big_reset(struct connection_in * in)
{
	switch (SOC(in)->type) {
		case ct_telnet:
			SOC(in)->timeout.tv_sec = Globals.timeout_network ;
			SOC(in)->timeout.tv_usec = 0 ;
			return DS2480_big_reset_serial(in) ;

		case ct_serial:
		default:
			SOC(in)->timeout.tv_sec = Globals.timeout_serial ;
			SOC(in)->timeout.tv_usec = 0 ;

			SOC(in)->flow = flow_none ;
			RETURN_GOOD_IF_GOOD( DS2480_big_reset_serial(in)) ;

			SOC(in)->flow = flow_none ;
			RETURN_GOOD_IF_GOOD( DS2480_big_reset_serial(in)) ;

			SOC(in)->flow = flow_hard ;
			RETURN_GOOD_IF_GOOD( DS2480_big_reset_serial(in)) ;

			return gbBAD ;
	}
}
// do the com port and configuration stuff
static GOOD_OR_BAD DS2480_big_reset_serial(struct connection_in * in)
{
	BYTE reset_byte = (BYTE) ( CMD_COMM | FUNCTSEL_RESET | SPEEDSEL_STD );

	// Open the com port in 9600 Baud.
	RETURN_BAD_IF_BAD(COM_open(in)) ;

	// send a break to reset the DS2480
	COM_break(in);

	// It's in command mode now
	in->master.serial.mode = ds2480b_command_mode ;

	// send the timing byte (A reset command at 9600 baud)
	DS2480_write( &reset_byte, 1, in ) ;

	// delay to let line settle
	UT_delay(4);
	// flush the buffers
	DS2480_flush(in);
	// ignore response
	DS2480_slurp( in ) ;
	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	in->changed_bus_settings = 1 ; // Force a mode change
	// Send a reset again
	LEVEL_DEBUG("Send the initial reset to the bus master.");
	DS2480_reset_in(in) ;

	// delay to let line settle
	UT_delay(400);
	// flush the buffers
	DS2480_flush(in);
	// ignore response
	DS2480_slurp( in ) ;

	// Now set desired baud and polarity
	return DS2480_big_configuration(in) ;
}

// do the configuration stuff
static GOOD_OR_BAD DS2480_big_configuration(struct connection_in * in)
{
	BYTE single_bit = CMD_COMM | BITPOL_ONE |  DS2480b_speed_byte(in) ;
	BYTE single_bit_response ;

	// Now set desired baud and polarity
	// BUS_reset will do the actual changes
	in->changed_bus_settings = 1 ; // Force a mode change
	// Send a reset again
	DS2480_reset_in(in) ;

	// delay to let line settle
	UT_delay(4);

	// default W1LT = 10us (write-1 low time)
	RETURN_BAD_IF_BAD(DS2480_configuration_write(PARMSEL_WRITE1LOW, PARMSET_Write10us, in)) ;

	// default DSO/WORT = 8us (data sample offset / write 0 recovery time )
	RETURN_BAD_IF_BAD(DS2480_configuration_write(PARMSEL_SAMPLEOFFSET, PARMSET_SampOff8us, in)) ;

	// Strong pullup duration = infinite
	RETURN_BAD_IF_BAD(DS2480_configuration_write(PARMSEL_5VPULSE, PARMSET_infinite, in)) ;

	// Program pulse duration = 512usec
	RETURN_BAD_IF_BAD(DS2480_configuration_write(PARMSEL_12VPULSE, PARMSET_512us, in)) ;

	// Send a single bit
	// The datasheet wants this
	RETURN_BAD_IF_BAD(DS2480_sendback_cmd(&single_bit, &single_bit_response, 1, in)) ;

	/* Apparently need to reset again to get the version number properly */
	return gbRESET( DS2480_reset_in(in)  ) ;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
static GOOD_OR_BAD DS2480_configuration_write(BYTE parameter_code, BYTE value_code, struct connection_in * in)
{
	BYTE send_code = CMD_CONFIG | parameter_code | value_code;
	BYTE expected_response = CMD_CONFIG_RESPONSE | parameter_code | value_code;
	BYTE actual_response;

	RETURN_BAD_IF_BAD(DS2480_sendback_cmd(&send_code, &actual_response, 1, in)) ;

	if (expected_response == actual_response) {
		return gbGOOD;
	}
	LEVEL_DEBUG("wrong response (%.2X not %.2X)",actual_response,expected_response) ;
	return gbBAD;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
static GOOD_OR_BAD DS2480_configuration_read(BYTE parameter_code, BYTE value_code, struct connection_in * in)
{
	BYTE send_code = CMD_CONFIG | PARMSEL_PARMREAD | (parameter_code>>3);
	BYTE expected_response = CMD_CONFIG_RESPONSE | PARMSEL_PARMREAD | value_code;
	BYTE actual_response;

	RETURN_BAD_IF_BAD( DS2480_sendback_cmd(&send_code, &actual_response, 1, in)) ;
	if (expected_response == actual_response) {
		return gbGOOD;
	}
	LEVEL_DEBUG("wrong response (%.2X not %.2X)",actual_response,expected_response) ;
	return gbBAD;
}

// configuration of DS2480B -- parameter code is already shifted in the defines (by 4 bites)
// configuration of DS2480B -- value code is already shifted in the defines (by 1 bit)
// Set Baud rate -- can't use DS2480_configuration_code because return byte is at a different speed
static void DS2480_set_baud_control(struct connection_in * in)
{
	// restrict allowable baud rates based on device capabilities
	COM_BaudRestrict( &(SOC(in)->baud), B9600, B19200, B57600, B115200, 0 ) ;

	if ( GOOD( DS2480_set_baud(in) ) ) {
		return ;
	}
	LEVEL_DEBUG("Failed first attempt at resetting baud rate of bus master %s",SAFESTRING(SOC(in)->devicename)) ;

	if ( GOOD( DS2480_set_baud(in) ) ) {
		return ;
	}
	LEVEL_DEBUG("Failed second attempt at resetting baud rate of bus master %s",SAFESTRING(SOC(in)->devicename)) ;

	// uh oh -- undefined state -- not sure what the bus speed is.
	in->reconnect_state = reconnect_error ;
	SOC(in)->baud = B9600 ;
	++in->changed_bus_settings ;
	return;
}

static GOOD_OR_BAD DS2480_set_baud(struct connection_in * in)
{
	BYTE value_code = 0 ;
	BYTE send_code ;

	// Find rate parameter
	switch ( SOC(in)->baud ) {
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
		default:
			SOC(in)->baud = B9600 ;
			value_code = PARMSET_9600 ;
			break ;
	}

	// Add polarity
	if ( in->master.serial.reverse_polarity ) {
		value_code |= PARMSET_REVERSE_POLARITY ;
	}
	send_code = CMD_CONFIG | PARMSEL_BAUDRATE | value_code;

	// Send configuration change
	DS2480_flush(in);
	UT_delay(5);

	// uh oh -- undefined state -- not sure what the bus speed is.
	RETURN_BAD_IF_BAD(DS2480_sendout_cmd(&send_code, 1, in)) ;

	// Change OS view of rate
	UT_delay(5);
	COM_change(in) ;
	UT_delay(5);
	DS2480_slurp(in);

	// Check rate
	RETURN_BAD_IF_BAD( DS2480_configuration_read( PARMSEL_BAUDRATE, value_code, in ) ) ;

	return gbGOOD ;
}

static BYTE DS2480b_speed_byte( struct connection_in * in )
{
	if ( in->overdrive ) {
		return SPEEDSEL_OD ;
	} else if ( in->flex ) {
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
static RESET_TYPE DS2480_reset(const struct parsedname *pn) {
	return DS2480_reset_in( pn->selected_connection ) ;
}

static RESET_TYPE DS2480_reset_in(struct connection_in * in)
{
	if ( in->changed_bus_settings != 0) {
		in->changed_bus_settings = 0 ;
		DS2480_set_baud_control(in);	// reset paramters
	}

	switch( DS2480_reset_once(in) ) {
		case BUS_RESET_OK:
			return BUS_RESET_OK ;
		case BUS_RESET_SHORT:
			return BUS_RESET_SHORT;
		case BUS_RESET_ERROR:
		default: {
			// Some kind of reset problem (not including bus short)
			// Try simple fixes
			BYTE dummy[1] ; //we don't check the pulse stop, since it's only a whim.
			// perhaps the DS9097U wasn't in command mode, so missed the reset
			// force a mode switch
			in->master.serial.mode = ds2480b_data_mode ;
			// Also make sure no power or programming pulse active
			DS2480_stop_pulse(dummy,in) ;
			// And now reset again
			return DS2480_reset_once(in) ;
		}
	}
}

static RESET_TYPE DS2480_reset_once(struct connection_in * in)
{
	BYTE reset_byte = (BYTE) ( CMD_COMM | FUNCTSEL_RESET | DS2480b_speed_byte(in) );
	BYTE reset_response ;

	// flush the buffers
	DS2480_flush(in);

	// send the packet
	// read back the 1 byte response
	if ( BAD(DS2480_sendback_cmd(&reset_byte, &reset_response, 1, in)) ) {
		return BUS_RESET_ERROR;
	}

	/* The adapter type is encoded in this response byte */
	/* The known values correspond to the types in enum adapter_type */
	/* Other values are assigned for adapters that don't have this hardcoded value */
	in->Adapter = (reset_response & RB_CHIPID_MASK) >> 2;

	switch (reset_response & RB_RESET_MASK) {
	case RB_1WIRESHORT:
		return BUS_RESET_SHORT;
	case RB_NOPRESENCE:
		in->AnyDevices = anydevices_no ;
		return BUS_RESET_OK;
	case RB_PRESENCE:
	case RB_ALARMPRESENCE:
		in->AnyDevices = anydevices_yes ;
		// check if programming voltage available
		in->ProgramAvailable = ((reset_response & PARMSEL_12VPULSE) == PARMSEL_12VPULSE);
		DS2480_flush(in);
		return BUS_RESET_OK;
	default:
		return BUS_RESET_ERROR; // should never happen
	}
}

/* search = normal and alarm */
static enum search_status DS2480_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int mismatched;
	BYTE sn[8];
	BYTE bitpairs[16];
	struct connection_in * in = pn->selected_connection ;
	BYTE searchon = (BYTE) ( CMD_COMM | FUNCTSEL_SEARCHON | DS2480b_speed_byte(in) );
	BYTE searchoff = (BYTE) ( CMD_COMM | FUNCTSEL_SEARCHOFF | DS2480b_speed_byte(in) );
	int i;

	if (ds->LastDevice) {
		return search_done;
	}

	if ( BAD( BUS_select(pn) ) ) {
		return search_error;
	}

	// need the reset done in BUS-select to set AnyDevices
	if ( in->AnyDevices == anydevices_no ) {
		ds->LastDevice = 1;
		return search_done;
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
	DS2480_flush(in);

	// search ON
	// change back to command mode
	// send the packet
	// cannot use single-bit mode with search accerator
	// search OFF
	if ( BAD(BUS_send_data(&(ds->search), 1, pn))
		|| BAD(DS2480_sendout_cmd(&searchon, 1, in))
		|| BAD( DS2480_sendback_data(bitpairs, bitpairs, 16, pn) )
		|| BAD(DS2480_sendout_cmd(&searchoff, 1, in))
		) {
		return search_error;
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

	if ( sn[0]==0xFF && sn[1]==0xFF && sn[2]==0xFF && sn[3]==0xFF && sn[4]==0xFF && sn[5]==0xFF && sn[6]==0xFF && sn[7]==0xFF ) {
		// special case for no alarm present
		return search_done ;
	}

	// CRC check
	if (CRC8(sn, 8) || (ds->LastDiscrepancy == 63) || (sn[0] == 0)) {
		return search_error;
	}

	// successful search
	// check for last one
	if ((mismatched == ds->LastDiscrepancy) || (mismatched == -1)) {
		ds->LastDevice = 1;
	}

	// copy the SerialNum to the buffer
	memcpy(ds->sn, sn, 8);

	// set the count
	ds->LastDiscrepancy = mismatched;

	LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
	return search_good;
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
static GOOD_OR_BAD DS2480_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret;
	struct connection_in * in = pn->selected_connection ;
	BYTE bits = CMD_COMM | FUNCTSEL_BIT | DS2480b_speed_byte(in) ;
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
	DS2480_flush(in);

	// send the packet
	// read back the 8 byte response from sending the 5V pulse
	ret = DS2480_sendback_cmd(cmd, respbits, 8, in);

	UT_delay(delay);

	// return to normal level
	DS2480_stop_pulse(response, in);

	resp[0] = ((respbits[7] & 1) << 7)
		| ((respbits[6] & 1) << 6)
		| ((respbits[5] & 1) << 5)
		| ((respbits[4] & 1) << 4)
		| ((respbits[3] & 1) << 3)
		| ((respbits[2] & 1) << 2)
		| ((respbits[1] & 1) << 1)
		| ((respbits[0] & 1));

	return ret ;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// bit read from the 1-Wire Net is the same (write operation).
// Delay delay msec and return to normal
//
/* Returns 0=good
   bad = -EIO
 */
static GOOD_OR_BAD DS2480_PowerBit(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret;
	struct connection_in * in = pn->selected_connection ;
	BYTE bits = CMD_COMM | FUNCTSEL_BIT | DS2480b_speed_byte(in) ;
	BYTE cmd[] = {
		((byte & 0x01) ? BITPOL_ONE : BITPOL_ZERO) | bits | PRIME5V_TRUE,
	};
	BYTE respbits[1];
	BYTE response[1];

	// flush the buffers
	DS2480_flush(in);

	// send the packet
	// read back the 1 byte response from sending the 5V pulse
	ret = DS2480_sendback_cmd(cmd, respbits, 1, in);

	UT_delay(delay);

	// return to normal level
	DS2480_stop_pulse(response, in);

	resp[0] = (respbits[0] & 0x01);

	return ret ;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
//
/* Returns 0=good
   bad = -EIO
 */
static GOOD_OR_BAD DS2480_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn)
{
	struct connection_in * in = pn->selected_connection ;
	BYTE bits = CMD_COMM | FUNCTSEL_BIT | DS2480b_speed_byte(in) | PRIME5V_FALSE;
	size_t counter ;

	for ( counter=0 ; counter < len ; ++counter ) {
		BYTE cmd[] = { ((databits[counter] & 0x01) ? BITPOL_ONE : BITPOL_ZERO) | bits, };

		// send the packet
		// read back the 1 byte response from sending the bit
		RETURN_BAD_IF_BAD( DS2480_sendback_cmd(cmd, &respbits[counter], 1, in) ) ;
	}
	return 0 ;
}

static void DS2480_flush( const struct connection_in * in )
{
	COM_flush( in ) ;
}

static void DS2480_slurp( struct connection_in * in )
{
	COM_slurp( in ) ;
}

static GOOD_OR_BAD DS2480_stop_pulse(BYTE * response, struct connection_in * in)
{
	BYTE cmd[1] = { MODE_STOP_PULSE, };
	// read back the 8 byte response from setting time limit
	return DS2480_sendback_cmd(cmd, response, 1, in);
}

/* Send a 12v 480usec pulse on the 1wire bus to program the EPROM */
// returns 0 if good
static GOOD_OR_BAD DS2480_ProgramPulse(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	GOOD_OR_BAD ret;
	BYTE cmd[] = { CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE, };
	BYTE response_mask = 0xFC;
	BYTE command_resp[1];
	BYTE stop_pulse[1] ;

	DS2480_flush(in);

	// send the packet
	// read back the 8 byte response from sending the 12V pilse
	ret = DS2480_sendback_cmd(cmd, command_resp, 1, in);

	if ( GOOD(ret) ) {
		UT_delay_us(520);
	}
	// return to normal level
	DS2480_stop_pulse(stop_pulse, in);

	RETURN_BAD_IF_BAD(ret) ;

	if ((command_resp[0] & response_mask) != (cmd[0] & response_mask)) {
		return gbBAD;
	}

	return gbGOOD;
}

// Write to the output -- works for tcp and COM
static GOOD_OR_BAD DS2480_write(const BYTE * buf, const size_t size, struct connection_in * in)
{
	switch( SOC(in)->type ) {
		case ct_telnet:
			return telnet_write_binary( buf, size, in) ;
		case ct_serial:
		default:
			return COM_write( buf, size, in ) ;
	}
}

/* Assymetric */
/* Read from DS2480 with timeout on each character */
// NOTE: from PDkit, read 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK (emulation mode)
static GOOD_OR_BAD DS2480_read(BYTE * buf, const size_t size, struct connection_in * in)
{
	return COM_read( buf, size, in ) ;
}

//
// DS2480_sendout_cmd
//  Send a command but expect no response
//  puts into command mode if needed.
// Note most use is through DS2480_sendback_command
// The only uses for this alone are the single byte search accelerators
// so size checking will be done in sendback
// return 0=good
static GOOD_OR_BAD DS2480_sendout_cmd(const BYTE * cmd, const size_t len, struct connection_in * in)
{
	if (in->master.serial.mode == ds2480b_command_mode ) {
		// already in command mode -- very easy
		return DS2480_write(cmd, (unsigned) len, in);
	} else {
		// need to switch to command mode
		// add to current string for efficiency
		BYTE cmd_plus[len+1] ;
		cmd_plus[0] = MODE_COMMAND ;
		memcpy( &cmd_plus[1], cmd, len ) ;
		// MODE_COMMAND is one of the reserved commands that does not generate a response byte
		// page 6 of datasheet: http://datasheets.maxim-ic.com/en/ds/DS2480B.pdf
		in->master.serial.mode = ds2480b_command_mode ;
		return DS2480_write(cmd_plus, (unsigned) len+1, in);
	}
}

//
// DS2480_sendback_cmd
//  Send a command and return response block
//  puts into command mode if needed.
// return 0=good
static GOOD_OR_BAD DS2480_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, struct connection_in * in)
{
	size_t bytes_so_far = 0 ;
	size_t extra_byte_for_modeshift  = (in->master.serial.mode != ds2480b_command_mode) ? 1 : 0 ;


	while ( bytes_so_far < len ) {
		size_t bytes_this_segment = len - bytes_so_far ;
		if ( bytes_this_segment > MAX_SEND_SIZE - extra_byte_for_modeshift ) {
			bytes_this_segment = MAX_SEND_SIZE - extra_byte_for_modeshift ;
			extra_byte_for_modeshift = 0 ;
		}
		RETURN_BAD_IF_BAD( DS2480_sendout_cmd( &cmd[bytes_so_far], bytes_this_segment, in) );
		RETURN_BAD_IF_BAD( DS2480_read( &resp[bytes_so_far], bytes_this_segment, in) );
		bytes_so_far += bytes_this_segment ;
	}
	return gbGOOD ;
}

//
// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
// Repeat magic MODE_COMMAND byte to show true data
// return 0=good
static GOOD_OR_BAD DS2480_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	BYTE sendout[MAX_SEND_SIZE] ;
	size_t bytes_this_segment = 0 ;
	size_t bytes_at_segment_start = 0 ;
	size_t bytes_from_list = 0 ;

	// skip if nothing
	// don't even switch to data mode
	if ( len == 0 ) {
		return gbGOOD ;
	}

	// switch mode if needed
	if (in->master.serial.mode != ds2480b_data_mode) {
		// this is one of the "reserved commands" that does not generate a resonse byte
		// page 6 of the datasheet http://datasheets.maxim-ic.com/en/ds/DS2480B.pdf
		sendout[bytes_this_segment++] = MODE_DATA;
		// change flag to data mode
		in->master.serial.mode = ds2480b_data_mode;
	}

	while ( bytes_from_list < len ) {
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
			RETURN_BAD_IF_BAD( DS2480_write(sendout, bytes_this_segment, in ) ) ;

			// read in
			// note that read and write sizes won't match because of doubled bytes and mode shift
			RETURN_BAD_IF_BAD( DS2480_read(&resp[bytes_at_segment_start], bytes_from_list - bytes_at_segment_start, in) ) ;

			// move indexes for next segment
			bytes_at_segment_start = bytes_from_list ;
			bytes_this_segment = 0 ;
		}
	}
	return gbGOOD ;
}

static void DS2480_close(struct connection_in *in)
{
	// the standard COM_free cleans up the connection
	(void) in ;
}

