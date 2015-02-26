/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* This is the busmaster code for the DS1WM
 * The "Synthesizable 1-wire Bus MAster" adapter from Dallas Maxim
 * 
 * Out technique is a little dangerous -- direct memory access to the registers
 * We use /dev/mem although writing a UIO kernel module is also a possibility
 * Obviously we'll nee root access
 * 
 */ 
 
#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include <sys/mman.h>

 
// DS1WM Registers
#define DS1WM_COMMAND_REGISTER 0
#define DS1WM_TXRX_BUFFER 1
#define DS1WM_INTERRUPT_REGISTER 2
#define DS1WM_INTERRUPT_ENABLE_REGISTER 3
#define DS1WM_CLOCK_DEVISOR_REGISTER 4
#define DS1WM_CONTROL_REGISTER 5
 
// Access register via mmap-ed memory
#define DS1WM_register(in, off) (((uint8_t *) (in->master.ds1wm.page_start))[(in->master.ds1wm.page_offset)+off])

// Register access macros
#define DS1WM_command(in)	DS1WM_register(in,DS1WM_COMMAND_REGISTER)
#define DS1WM_txrx(in)		DS1WM_register(in,DS1WM_TXRX_BUFFER)
#define DS1WM_interrupt(in)	DS1WM_register(in,DS1WM_INTERRUPT_REGISTER)
#define DS1WM_enable(in)	DS1WM_register(in,DS1WM_INTERRUPT_ENABLE_REGISTER)
#define DS1WM_clock(in)		DS1WM_register(in,DS1WM_CLOCK_DEVISOR_REGISTER)
#define DS1WM_control(in)	DS1WM_register(in,DS1WM_CONTROL_REGISTER)

enum e_DS1WM_command { e_ds1wm_1wr=0, e_ds1wm_sra, e_ds1wm_fow, e_ds1wm_ow_in, } ;

enum e_DS1WM_int { e_ds1wm_pd=0, e_ds1wm_pdr, e_ds1wm_tbe, e_ds1wm_temt, e_ds1wm_rbf, e_ds1wm_rsrf, e_ds1wm_ow_short, e_ds1wm_ow_low,  } ;

enum e_DS1WM_enable { e_ds1wm_epd=0, e_ds1wm_ias, e_ds1wm_etbe, e_ds1wm_etmt, e_ds1wm_erbf, e_ds1wm_ersf, e_ds1wm_eowsh, e_ds1wm_eowl } ;

enum e_DS1WM_control { e_ds1wm_llm=0, e_ds1wm_ppm, e_ds1wm_en_fow, e_ds1wm_stpen, e_ds1wm_stp_sply, e_ds1wm_bit_ctl, e_ds1wm_od } ;

static RESET_TYPE DS1WM_reset(const struct parsedname *pn);
static enum search_status DS1WM_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD DS1WM_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD DS1WM_PowerBit(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD DS1WM_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD DS1WM_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn);
static GOOD_OR_BAD DS1WM_reconnect(const struct parsedname * pn);
static void DS1WM_close(struct connection_in *in) ;

static void DS1WM_setroutines(struct connection_in *in);

static unsigned char DS1WM_freq( unsigned long f );
static GOOD_OR_BAD DS1WM_setup( struct connection_in * in );
static RESET_TYPE DS1WM_wait_for_reset( struct connection_in * in );
static GOOD_OR_BAD DS1WM_wait_for_read( const struct connection_in * in );
static GOOD_OR_BAD DS1WM_wait_for_write( const struct connection_in * in );
static GOOD_OR_BAD DS1WM_wait_for_byte( const struct connection_in * in );
static GOOD_OR_BAD DS1WM_sendback_byte(const BYTE * data, BYTE * resp, const struct connection_in * in ) ;

static void DS1WM_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS1WM_detect;
	in->iroutines.reset = DS1WM_reset;
	in->iroutines.next_both = DS1WM_next_both;
	in->iroutines.PowerByte = DS1WM_PowerByte;
	in->iroutines.PowerBit = DS1WM_PowerBit;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = DS1WM_sendback_data;
    in->iroutines.sendback_bits = DS1WM_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = DS1WM_reconnect ;
	in->iroutines.close = DS1WM_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_default;
	in->bundling_length = UART_FIFO_SIZE;
}

// Search defines
#define SEARCH_BIT_ON		0x01

/* Setup DS1WM bus master structure */
// bus locking at a higher level
GOOD_OR_BAD DS1WM_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	long long int prebase ;
	off_t base ;
	void * mm ;
	FILE_DESCRIPTOR_OR_ERROR mem_fd ;
	int pagesize = getpagesize() ;

	
	in->Adapter = adapter_ds1wm ;
	in->master.ds1wm.longline = 0 ; // longline timing
	in->master.ds1wm.frequency = 10000000 ; // 10MHz
	in->master.ds1wm.presence_mask = 1 ; // pulse presence mask

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("DS1WM needs a memory location");
		return gbBAD;
	}

	if ( sscanf( pin->init_data, "%lli", &prebase ) != 1 ) {
		LEVEL_DEFAULT("DS1WM: Could not interpret <%s> as a memory address", pin->init_data ) ;
		return gbBAD ;
	}
	base = prebase ; // convert types long long int -> off_t
	if ( base == 0 ) {
		LEVEL_DEFAULT("DS1WM: Illegal address 0x0000 from <%s>", pin->init_data ) ;
		return gbBAD ;
	}
	LEVEL_DEBUG("DS1WM at address %p",(void *)base);
	
	in->master.ds1wm.base = base ;
	in->master.ds1wm.page_offset = base % pagesize ;
	in->master.ds1wm.page_start = base - in->master.ds1wm.page_offset ;
	
	// open /dev/mem
	mem_fd = open( "/dev/mem", O_RDWR | O_SYNC ) ;
	
	if ( FILE_DESCRIPTOR_NOT_VALID(mem_fd) ) {
		LEVEL_DEFAULT("DS1WM: Cannot open memmory directly -- permissions problem?");
		return gbBAD ;
	}
	
	mm = mmap( NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, in->master.ds1wm.page_start );
	
	close(mem_fd) ; // no longer needed
	
	if ( mm == MAP_FAILED ) {
		LEVEL_DEFAULT("DS1WM: Cannot map memmory") ;
		return gbBAD ;
	}
	
	in->master.ds1wm.mm = mm ;

	/* Set up low-level routines */
	DS1WM_setroutines(in);
	
	return DS1WM_setup(in) ;
}

static unsigned char DS1WM_freq( unsigned long f )
{
	static struct {
			unsigned long freq;
			unsigned char divisor;
	} freq[] = {
			{   1000000, 0x80 },
			{   2000000, 0x84 },
			{   3000000, 0x81 },
			{   4000000, 0x88 },
			{   5000000, 0x82 },
			{   6000000, 0x85 },
			{   7000000, 0x83 },
			{   8000000, 0x8c },
			{  10000000, 0x86 },
			{  12000000, 0x89 },
			{  14000000, 0x87 },
			{  16000000, 0x90 },
			{  20000000, 0x8a },
			{  24000000, 0x8d },
			{  28000000, 0x8b },
			{  32000000, 0x94 },
			{  40000000, 0x8e },
			{  48000000, 0x91 },
			{  56000000, 0x8f },
			{  64000000, 0x98 },
			{  80000000, 0x92 },
			{  96000000, 0x95 },
			{ 112000000, 0x93 },
			{ 128000000, 0x9c },
	/* you can continue this table, consult the OPERATION - CLOCK DIVISOR
	   section of the ds1wm spec sheet. */
	};
	int n = sizeof(freq) / sizeof(freq[0]) ;
	int i ;
	int last = 0 ;

	for ( i=1 ; i<n ; ++i ) {
		if ( freq[i].freq > f ) {
			break ;
		}
		last = i ;
	}
	LEVEL_DEBUG( "Frequency %ld matches %ld",f,freq[last],freq ) ;
	return freq[last].divisor ;
}

// set control pins and frequency for defauts and global settings 
static GOOD_OR_BAD DS1WM_setup( struct connection_in * in )
{
	uint8_t control_register = DS1WM_control(in) ;

	DS1WM_clock(in) = 0x00 ; // off (causes reset?)
	
	// set some defaults:
	UT_setbit( &control_register, e_ds1wm_ppm, in->master.ds1wm.presence_mask ) ; // pulse presence masked
	UT_setbit( &control_register, e_ds1wm_en_fow, 0 ) ; // no bit banging
	
	UT_setbit( &control_register, e_ds1wm_stpen, 1 ) ; // allow strong pullup
	UT_setbit( &control_register, e_ds1wm_stp_sply, 0 ) ; // not in strong pullup state, however
	
	in->master.ds1wm.byte_mode = 1 ; // default
	UT_setbit( &control_register, e_ds1wm_bit_ctl, 0 ) ; // byte mode
	
	UT_setbit( &control_register, e_ds1wm_od, in->overdrive ) ; // not overdrive
	UT_setbit( &control_register, e_ds1wm_llm, in->master.ds1wm.longline ) ; // set long line flag
	DS1WM_control(in) = control_register ;
	
	if ( DS1WM_control(in) != control_register ) {
		return gbBAD ;
	}
	
	DS1WM_clock(in) = DS1WM_freq( in->master.ds1wm.frequency ) ;
	
	return gbGOOD ;
}	

static GOOD_OR_BAD DS1WM_reconnect(const struct parsedname * pn)
{
	LEVEL_DEBUG("Attempting reconnect on %s",SAFESTRING(DEVICENAME(pn->selected_connection)));
	return DS1WM_setup(pn->selected_connection) ;
}

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
//          This routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B.
static RESET_TYPE DS1WM_reset(const struct parsedname * pn)
{
	struct connection_in * in = pn->selected_connection ; 
	if ( in->changed_bus_settings != 0) {
		in->changed_bus_settings = 0 ;
		DS1WM_setup(in);	// reset paramters
	}

	UT_setbit( &DS1WM_command(in), e_ds1wm_1wr, 1 ) ;
	
	switch( DS1WM_wait_for_reset(in) ) {
		case BUS_RESET_SHORT:
			return BUS_RESET_SHORT ;
		case BUS_RESET_OK:
			return BUS_RESET_OK ;
		default:
			return DS1WM_wait_for_reset(in) ;
	}
}

#define SERIAL_NUMBER_BITS (8*SERIAL_NUMBER_SIZE)
/* search = normal and alarm */
static enum search_status DS1WM_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int mismatched;
	BYTE sn[SERIAL_NUMBER_SIZE];
	BYTE bitpairs[SERIAL_NUMBER_SIZE*2];
	BYTE dummy ;
	struct connection_in * in = pn->selected_connection ;
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

	// clear sn to satisfy static error checking (Coverity)
	memset( sn, 0, SERIAL_NUMBER_SIZE ) ;
	
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
	memset(bitpairs, 0, SERIAL_NUMBER_SIZE*2);

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

	// search ON
	// Send search rom or conditional search byte
	if ( BAD( DS1WM_sendback_byte(&(ds->search), &dummy, in) ) ) {
		return search_error;
	}

	// Set search accelerator
	UT_setbit( &DS1WM_command(in), e_ds1wm_sra, 1 ) ;

	// send the packet
	// cannot use single-bit mode with search accerator
	// search OFF
	if ( BAD( DS1WM_sendback_data(bitpairs, bitpairs, SERIAL_NUMBER_SIZE*2, pn) ) ) {
		return search_error;
	}

	// interpret the bit stream
	for (i = 0; i < SERIAL_NUMBER_BITS; i++) {
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
	if (CRC8(sn, SERIAL_NUMBER_SIZE) || (ds->LastDiscrepancy == SERIAL_NUMBER_BITS-1) || (sn[0] == 0)) {
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
static GOOD_OR_BAD DS1WM_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret = gbBAD ; // default
	struct connection_in * in = pn->selected_connection ;
	uint8_t control_register ;
	
	// Set power on
	control_register = DS1WM_control(in) ;
	UT_setbit( &control_register,e_ds1wm_stp_sply, 1 ) ;
	DS1WM_control(in) = control_register ;

	if ( GOOD( DS1WM_sendback_byte( &byte, resp, in ) ) && GOOD( DS1WM_wait_for_write(in) ) ) {
		UT_delay(delay);
		ret = gbGOOD ;
	}

	// Set power off
	control_register = DS1WM_control(in) ;
	UT_setbit( &control_register,e_ds1wm_stp_sply, 0 ) ;
	DS1WM_control(in) = control_register ;

	return ret ;
}


//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// bit read from the 1-Wire Net is the same (write operation).
// Delay delay msec and return to normal
//
static GOOD_OR_BAD DS1WM_PowerBit(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret = gbBAD ; // default
	struct connection_in * in = pn->selected_connection ;
	uint8_t control_register ;
	
	// Set power, bitmode on
	control_register = DS1WM_control(in) ;
	UT_setbit( &control_register,e_ds1wm_stp_sply, 1 ) ;
	UT_setbit( &control_register,e_ds1wm_bit_ctl, 1 ) ;
	in->master.ds1wm.byte_mode = 0 ;
	DS1WM_control(in) = control_register ;

	if ( GOOD( DS1WM_sendback_byte( &byte, resp, in ) ) && GOOD( DS1WM_wait_for_write(in) ) ) {
		UT_delay(delay);
		ret = gbGOOD ;
	}

	// Set power, bitmode off
	control_register = DS1WM_control(in) ;
	UT_setbit( &control_register,e_ds1wm_stp_sply, 0 ) ;
	UT_setbit( &control_register,e_ds1wm_bit_ctl, 0 ) ;
	in->master.ds1wm.byte_mode = 1 ;
	DS1WM_control(in) = control_register ;

	return ret ;
}

//--------------------------------------------------------------------------
// Send  and receive 1 bit of communication to the 1-Wire
//
static GOOD_OR_BAD DS1WM_sendback_bits(const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn)
{
	struct connection_in * in = pn->selected_connection ;
	uint8_t control_register ;
	GOOD_OR_BAD ret ;

	// Set bitmode on
	control_register = DS1WM_control(in) ;
	UT_setbit( &control_register,e_ds1wm_bit_ctl, 1 ) ;
	in->master.ds1wm.byte_mode = 0 ;
	DS1WM_control(in) = control_register ;

	ret = DS1WM_sendback_data( databits, respbits, len, pn ) ;

	// Set bitmode off
	control_register = DS1WM_control(in) ;
	in->master.ds1wm.byte_mode = 1 ;
	UT_setbit( &control_register,e_ds1wm_bit_ctl, 0 ) ;
	DS1WM_control(in) = control_register ;

	return ret ;
}

static GOOD_OR_BAD DS1WM_sendback_byte(const BYTE * data, BYTE * resp, const struct connection_in * in )
{
	RETURN_BAD_IF_BAD( DS1WM_wait_for_write(in) ) ;
	DS1WM_txrx(in) = data[0] ;
	RETURN_BAD_IF_BAD( DS1WM_wait_for_read(in) ) ;
	resp[0] = DS1WM_txrx(in) ;
	return gbGOOD ;
}

//
// sendback_data
//  Send data and return response block
static GOOD_OR_BAD DS1WM_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	size_t i ;

	for (i=0 ; i<len ; ++i ) {
		RETURN_BAD_IF_BAD( DS1WM_sendback_byte( &data[i], &resp[i], in ) ) ;
	}
	return gbGOOD ;
}

static void DS1WM_close(struct connection_in *in)
{
	// the standard COM_free cleans up the connection
	munmap( in->master.ds1wm.mm, getpagesize() );
}

// wait for reset
static GOOD_OR_BAD DS1WM_wait_for_byte( const struct connection_in * in )
{
	int bits = in->master.ds1wm.byte_mode ? 8 : 1 ;
	long int t_slot = in->overdrive ? 15000 : 86000 ; // nsec
	struct timespec t = {
		0, 
		t_slot*bits,
	};
	
	if ( nanosleep( & t, NULL ) != 0 ) {
		return gbBAD ;
	}
	return gbGOOD ;
}

// Wait max time needed for reset
static RESET_TYPE DS1WM_wait_for_reset( struct connection_in * in )
{
	long int t_reset = in->overdrive ? (74000+63000) : (636000+626000) ; // nsec
	uint8_t interrupt ;
	struct timespec t = {
		0, 
		t_reset,
	} ;
	
	if ( nanosleep( & t, NULL ) != 0 ) {
		return gbBAD ;
	}
	interrupt = DS1WM_interrupt(in) ;
	if ( UT_getbit( &interrupt, e_ds1wm_pd ) == 0 ) {
		return BUS_RESET_ERROR ;
	}
	if ( UT_getbit( &interrupt, e_ds1wm_ow_short ) == 1 ) {
		return BUS_RESET_SHORT ;
	}
	in->AnyDevices = ( UT_getbit( &interrupt, e_ds1wm_pdr ) == 1 ) ? anydevices_yes : anydevices_no ;
	return BUS_RESET_OK ;
}

static GOOD_OR_BAD DS1WM_wait_for_read( const struct connection_in * in )
{
	int i ;
	
	if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_rbf ) == 1 ) {
		return gbGOOD ;
	}

	for ( i=0 ; i < 5 ; ++i ) {
		RETURN_BAD_IF_BAD( DS1WM_wait_for_byte(in) ) ;
		if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_rbf ) == 1 ) {
			return gbGOOD ;
		}
	}
	return gbBAD ;
}

static GOOD_OR_BAD DS1WM_wait_for_write( const struct connection_in * in )
{
	int i ;
	
	if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_tbe ) == 1 ) {
		return gbGOOD ;
	}

	for ( i=0 ; i < 5 ; ++i ) {
		RETURN_BAD_IF_BAD( DS1WM_wait_for_byte(in) ) ;
		if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_tbe ) == 1 ) {
			return gbGOOD ;
		}
	}
	return gbBAD ;
}


