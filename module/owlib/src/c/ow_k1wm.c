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
 
 /* Based on a device used by Kistler Corporation
  * and tested in-house by Martin Rapavy
  * 
  * That testing also covers the DS1WM
  * */
 
#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_global.h"
#include <sys/mman.h>
 
// DS1WM Registers
#define DS1WM_COMMAND_REGISTER 0
#define DS1WM_TXRX_BUFFER 1
#define DS1WM_INTERRUPT_REGISTER 2
#define DS1WM_INTERRUPT_ENABLE_REGISTER 3
#define DS1WM_CLOCK_DEVISOR_REGISTER 4
#define K1WM_CHANNEL_SELECT_REGISTER DS1WM_CLOCK_DEVISOR_REGISTER
#define DS1WM_CONTROL_REGISTER 5
 
// Access register via mmap-ed memory
#define DS1WM_register(in, off) (((uint8_t *) (in->master.ds1wm.mm))[(in->master.ds1wm.base)+off])

// Register access macros
#define DS1WM_command(in)	DS1WM_register(in,DS1WM_COMMAND_REGISTER)
#define DS1WM_txrx(in)		DS1WM_register(in,DS1WM_TXRX_BUFFER)
#define DS1WM_interrupt(in)	DS1WM_register(in,DS1WM_INTERRUPT_REGISTER)
#define DS1WM_enable(in)	DS1WM_register(in,DS1WM_INTERRUPT_ENABLE_REGISTER)
#define DS1WM_clock(in)		DS1WM_register(in,DS1WM_CLOCK_DEVISOR_REGISTER)
#define K1WM_channel(in)	DS1WM_register(in,K1WM_CHANNEL_SELECT_REGISTER)
#define DS1WM_control(in)	DS1WM_register(in,DS1WM_CONTROL_REGISTER)

enum e_DS1WM_command { e_ds1wm_1wr=0, e_ds1wm_sra, e_ds1wm_fow, e_ds1wm_ow_in, } ;

enum e_DS1WM_int { e_ds1wm_pd=0, e_ds1wm_pdr, e_ds1wm_tbe, e_ds1wm_temt, e_ds1wm_rbf, e_ds1wm_rsrf, e_ds1wm_ow_short, e_ds1wm_ow_low,  } ;

enum e_DS1WM_enable { e_ds1wm_epd=0, e_ds1wm_ias, e_ds1wm_etbe, e_ds1wm_etmt, e_ds1wm_erbf, e_ds1wm_ersf, e_ds1wm_eowsh, e_ds1wm_eowl } ;

enum e_DS1WM_control { e_ds1wm_llm=0, e_ds1wm_ppm, e_ds1wm_en_fow, e_ds1wm_stpen, e_ds1wm_stp_sply, e_ds1wm_bit_ctl, e_ds1wm_od } ;

static RESET_TYPE K1WM_reset(const struct parsedname *pn);
static enum search_status K1WM_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD K1WM_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD K1WM_reconnect(const struct parsedname * pn);
static void K1WM_close(struct connection_in *in) ;

static void K1WM_setroutines(struct connection_in *in);

static GOOD_OR_BAD K1WM_setup( struct connection_in * in );
static RESET_TYPE K1WM_wait_for_reset( struct connection_in * in );
static GOOD_OR_BAD K1WM_wait_for_read( const struct connection_in * in );
static GOOD_OR_BAD K1WM_wait_for_write( const struct connection_in * in );
static GOOD_OR_BAD K1WM_wait_for_byte( const struct connection_in * in );
static GOOD_OR_BAD K1WM_sendback_byte(const BYTE * data, BYTE * resp, const struct connection_in * in ) ;

static GOOD_OR_BAD read_device_map_offset(const char *device_name, off_t *offset);
static GOOD_OR_BAD read_device_map_size(const char *device_name, size_t *size);
static GOOD_OR_BAD K1WM_select_channel(const struct connection_in * in, uint8_t channel);
static GOOD_OR_BAD K1WM_create_channels(struct connection_in *head, int channels_count);

static void K1WM_setroutines(struct connection_in *in)
{
	in->iroutines.detect = K1WM_detect;
	in->iroutines.reset = K1WM_reset;
	in->iroutines.next_both = K1WM_next_both;
	// K1WM doesn't have strong pull-up support
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.PowerBit = NO_POWERBIT_ROUTINE;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = K1WM_sendback_data;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = K1WM_reconnect ;
	in->iroutines.close = K1WM_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_default;
	in->bundling_length = UART_FIFO_SIZE;
}

// Search defines
#define SEARCH_BIT_ON		0x01

/* Setup DS1WM bus master structure */
// bus locking at a higher level
GOOD_OR_BAD K1WM_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	long long int prebase ;
	unsigned int prechannels_count;
	void * mm ;
	FILE_DESCRIPTOR_OR_ERROR mem_fd ;
	const char * mem_device = "/dev/uio0";

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("K1WM needs a memory location");
		return gbBAD;
	}

	in->Adapter = adapter_k1wm ;
	in->master.ds1wm.longline = 0 ; // longline timing
	// in->master.ds1wm.frequency = 0 ; // unused in k1wm
	in->master.ds1wm.presence_mask = 1 ; // pulse presence mask
	in->master.ds1wm.active_channel = 0;
	in->master.ds1wm.channels_count = 1;

	int param_count = sscanf( pin->init_data, "%lli,%u", &prebase, &prechannels_count);
	if ( param_count < 1 || param_count > 2) {
		LEVEL_DEFAULT("K1WM: Could not interpret <%s> as a memory address:channel_count pair", pin->init_data ) ;
		return gbBAD ;
	}

	in->master.ds1wm.channels_count = prechannels_count ;
	in->master.ds1wm.base = prebase ; // convert types long long int -> off_t
	if ( in->master.ds1wm.base == 0 ) {
		LEVEL_DEFAULT("K1WM: Illegal address 0x0000 from <%s>", pin->init_data ) ;
		return gbBAD ;
	}
	LEVEL_DEBUG("K1WM at address %p",(void *)in->master.ds1wm.base);
	LEVEL_DEBUG("K1WM channels: %u",in->master.ds1wm.channels_count);

	read_device_map_size(mem_device, &(in->master.ds1wm.mm_size));
	read_device_map_offset(mem_device, &(in->master.ds1wm.page_start));

	// open /dev/uio0
	mem_fd = open( mem_device, O_RDWR | O_SYNC ) ;

	if ( FILE_DESCRIPTOR_NOT_VALID(mem_fd) ) {
		LEVEL_DEFAULT("K1WM: Cannot open memory directly -- permissions problem?");
		return gbBAD ;
	}

	mm = mmap( NULL, in->master.ds1wm.mm_size, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, in->master.ds1wm.page_start );

	close(mem_fd) ; // no longer needed

	if ( mm == MAP_FAILED ) {
		LEVEL_DEFAULT("K1WM: Cannot map memory") ;
		return gbBAD ;
	}

	in->master.ds1wm.mm = mm ;

	/* Set up low-level routines */
	K1WM_setroutines(in);

	// Add channels
	K1WM_create_channels(in, in->master.ds1wm.channels_count);

	return K1WM_setup(in) ;
}

// set control pins and frequency for defauts and global settings 
static GOOD_OR_BAD K1WM_setup( struct connection_in * in )
{
	uint8_t control_register = DS1WM_control(in) ;
	LEVEL_DEBUG("[%s] control_register before setup: 0x%x", __FUNCTION__, control_register);

	// Set to channel
	K1WM_channel(in) = in->master.ds1wm.active_channel ;

	// set some defaults:
	UT_setbit( &control_register, e_ds1wm_ppm, in->master.ds1wm.presence_mask ) ; // pulse presence masked
	UT_setbit( &control_register, e_ds1wm_en_fow, 0 ) ; // no bit banging
	
	UT_setbit( &control_register, e_ds1wm_stpen, 0 ) ; // strong pullup not supported in K1WM
	UT_setbit( &control_register, e_ds1wm_stp_sply, 0 ) ; // not in strong pullup state, too
	
	in->master.ds1wm.byte_mode = 1 ; // default
	UT_setbit( &control_register, e_ds1wm_bit_ctl, 0 ) ; // byte mode
	
	UT_setbit( &control_register, e_ds1wm_od, in->overdrive ) ; // not overdrive
	UT_setbit( &control_register, e_ds1wm_llm, in->master.ds1wm.longline ) ; // set long line flag
	DS1WM_control(in) = control_register ;
	LEVEL_DEBUG("[%s] control_register after setup: 0x%x", __FUNCTION__, DS1WM_control(in));
	
	if ( DS1WM_control(in) != control_register ) {
		return gbBAD ;
	}

	return gbGOOD ;
}	

static GOOD_OR_BAD K1WM_reconnect(const struct parsedname * pn)
{
	LEVEL_DEBUG("Attempting reconnect on %s",SAFESTRING(DEVICENAME(pn->selected_connection)));
	return K1WM_setup(pn->selected_connection) ;
}

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
//          This routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B.
static RESET_TYPE K1WM_reset(const struct parsedname * pn)
{
	LEVEL_DEBUG("[%s] BUS reset", __FUNCTION__);
	struct connection_in * in = pn->selected_connection ; 
	if ( in->changed_bus_settings != 0) {
		in->changed_bus_settings = 0 ;
		K1WM_setup(in);	// reset paramters
	}

	// select channel
	K1WM_select_channel(in, in->master.ds1wm.active_channel);

	// read interrupt register to clear all bits
	(void) DS1WM_interrupt(in);

	UT_setbit( &DS1WM_command(in), e_ds1wm_1wr, 1 ) ;
	
	switch( K1WM_wait_for_reset(in) ) {
		case BUS_RESET_SHORT:
			return BUS_RESET_SHORT ;
		case BUS_RESET_OK:
			return BUS_RESET_OK ;
		default:
			return K1WM_wait_for_reset(in) ;
	}
}

/* search = normal and alarm */
static enum search_status K1WM_next_both(struct device_search *ds, const struct parsedname *pn)
{
	LEVEL_DEBUG("[%s] BUS search", __FUNCTION__);
	int mismatched;
	BYTE sn[SERIAL_NUMBER_SIZE];
	BYTE bitpairs[SERIAL_NUMBER_SIZE*2];
	BYTE dummy ;
	struct connection_in * in = pn->selected_connection ;
	int i;

	LEVEL_DEBUG("[%s] ds->LastDevice == true ?", __FUNCTION__);
	if (ds->LastDevice) {
		LEVEL_DEBUG("[%s] ds->LastDevice == true -> search_done", __FUNCTION__);
		return search_done;
	}

	LEVEL_DEBUG("[%s] BUS_select failed ?", __FUNCTION__);
	if ( BAD( BUS_select(pn) ) ) {
		LEVEL_DEBUG("[%s] BUS_select failed -> search_error", __FUNCTION__);
		return search_error;
	}

	// Standard SEARCH ROM using SRA
	// need the reset done in BUS-select to set AnyDevices
	if ( in->AnyDevices == anydevices_no ) {
		LEVEL_DEBUG("[%s] in->AnyDevices == anydevices_no -> search_done", __FUNCTION__);
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

	LEVEL_DEBUG("[%s] ds->LastDiscrepancy == %i", __FUNCTION__, ds->LastDiscrepancy);
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
	if ( BAD( K1WM_sendback_byte(&(ds->search), &dummy, in) ) ) {
		LEVEL_DEBUG("[%s] Sending SearchROM/SearchByte '0x%x' failed -> search_error", __FUNCTION__, ds->search);
		return search_error;
	}

	// Set search accelerator
	UT_setbit( &DS1WM_command(in), e_ds1wm_sra, 1 ) ;

	// send the packet
	// cannot use single-bit mode with search accerator
	// search OFF
	if ( BAD( K1WM_sendback_data(bitpairs, bitpairs, SERIAL_NUMBER_SIZE*2, pn) ) ) {
		LEVEL_DEBUG("[%s] Sending the packet (bitpairs) failed -> search_error", __FUNCTION__);
		return search_error;
	}

	// Turn off search accelerator
	UT_setbit( &DS1WM_command(in), e_ds1wm_sra, 0 ) ;

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
		LEVEL_DEBUG("[%s] sn == 0xFFFFFFFFFFFFFFFF -> search_error", __FUNCTION__);
		return search_done ;
	}

	// CRC check
	if (CRC8(sn, SERIAL_NUMBER_SIZE) || (ds->LastDiscrepancy == SERIAL_NUMBER_BITS-1) || (sn[0] == 0)) {
		LEVEL_DEBUG("[%s] CRC check failed -> search_error", __FUNCTION__);
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

static GOOD_OR_BAD K1WM_sendback_byte(const BYTE * data, BYTE * resp, const struct connection_in * in )
{
	LEVEL_DEBUG("[%s] sending byte: 0x%x", __FUNCTION__, data[0]);
	RETURN_BAD_IF_BAD( K1WM_wait_for_write(in) ) ;
	DS1WM_txrx(in) = data[0] ;
	RETURN_BAD_IF_BAD( K1WM_wait_for_read(in) ) ;
	resp[0] = DS1WM_txrx(in) ;
	LEVEL_DEBUG("[%s] received byte: 0x%x", __FUNCTION__, resp[0]);
	return gbGOOD ;
}

//
// sendback_data
//  Send data and return response block
static GOOD_OR_BAD K1WM_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	LEVEL_DEBUG("[%s]", __FUNCTION__);
	struct connection_in * in = pn->selected_connection ;
	size_t i ;

	K1WM_select_channel(in, in->master.ds1wm.active_channel);

	for (i=0 ; i<len ; ++i ) {
		RETURN_BAD_IF_BAD( K1WM_sendback_byte( data+i, resp+i, in ) ) ;
	}

	return gbGOOD ;
}

static void K1WM_close(struct connection_in *in)
{
	LEVEL_DEBUG("[%s] Closing BUS", __FUNCTION__);
	// the standard COM_free cleans up the connection
	munmap( in->master.ds1wm.mm, in->master.ds1wm.mm_size );
}

// wait for reset
static GOOD_OR_BAD K1WM_wait_for_byte( const struct connection_in * in )
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
static RESET_TYPE K1WM_wait_for_reset( struct connection_in * in )
{
	LEVEL_DEBUG("[%s]", __FUNCTION__);
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
		LEVEL_DEBUG("[%s] presence_detect bit == 0", __FUNCTION__);
		return BUS_RESET_ERROR ;
	}
	if ( UT_getbit( &interrupt, e_ds1wm_ow_short ) == 1 ) {
		LEVEL_DEBUG("[%s] short bit == 1", __FUNCTION__);
		return BUS_RESET_SHORT ;
	}

	in->AnyDevices = ( UT_getbit( &interrupt, e_ds1wm_pdr ) == 0 ) ? anydevices_yes : anydevices_no ;
	LEVEL_DEBUG("[%s] in->AnyDevices == %i", __FUNCTION__, in->AnyDevices);
	return BUS_RESET_OK ;
}

static GOOD_OR_BAD K1WM_wait_for_read( const struct connection_in * in )
{
	int i ;
	
	if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_rbf ) == 1 ) {
		return gbGOOD ;
	}

	for ( i=0 ; i < 5 ; ++i ) {
		RETURN_BAD_IF_BAD( K1WM_wait_for_byte(in) ) ;
		if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_rbf ) == 1 ) {
			return gbGOOD ;
		}
	}
	return gbBAD ;
}

static GOOD_OR_BAD K1WM_wait_for_write( const struct connection_in * in )
{
	int i ;
	
	if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_tbe ) == 1 ) {
		return gbGOOD ;
	}

	for ( i=0 ; i < 5 ; ++i ) {
		RETURN_BAD_IF_BAD( K1WM_wait_for_byte(in) ) ;
		if ( UT_getbit( &DS1WM_interrupt(in), e_ds1wm_tbe ) == 1 ) {
			return gbGOOD ;
		}
	}
	return gbBAD ;
}

static GOOD_OR_BAD read_device_map_offset(const char *device_name, off_t *offset)
{
	const unsigned char path_length = 100;
	FILE* file;
	unsigned int preoffset;

	// Get device filename (e.g. uio0)
	const char *uio_filename = strrchr(device_name, '/');
	if (uio_filename == NULL) {
		return gbBAD;
	}

	// Offset parameter file path
	char uio_map_offset_file[path_length];
	snprintf(uio_map_offset_file, path_length, "/sys/class/uio/%s/maps/map0/offset", uio_filename);

	// Read offset parameter
	file = fopen(uio_map_offset_file, "r");
	if (file == NULL) {
		return gbBAD;
	}
	if (fscanf(file, "%x", &preoffset) != 1) {
		fclose(file);
		return gbBAD;
	}
	fclose(file);
	*offset = preoffset;
	LEVEL_DEBUG("[%s] map offset: 0x%x", __FUNCTION__, preoffset);
	LEVEL_DEBUG("[%s] map offset: 0x%x", __FUNCTION__, *offset);

	return gbGOOD;
}

static GOOD_OR_BAD read_device_map_size(const char *device_name, size_t *size)
{
	const unsigned char path_length = 100;
	unsigned int usize ;
	FILE* file;

	// Get device filename (e.g. uio0)
	const char *uio_filename = strrchr(device_name, '/');
	if (uio_filename == NULL) {
		return gbBAD;
	}

	// Size parameter file path
	char uio_map_size_file[path_length];
	snprintf(uio_map_size_file, path_length, "/sys/class/uio/%s/maps/map0/size", uio_filename);

	// Read size parameter
	file = fopen(uio_map_size_file, "r");
	if (file == NULL) {
		return gbBAD;
	}
	if (fscanf(file, "%x", &usize) != 1){
		fclose(file);
		return gbBAD;
	}
	*size = usize ;
	fclose(file);
	LEVEL_DEBUG("[%s] map size: 0x%x", __FUNCTION__, *size);

	return gbGOOD;
}

static GOOD_OR_BAD K1WM_select_channel(const struct connection_in * in, uint8_t channel)
{
	LEVEL_DEBUG("[%s] Selecting channel %u", __FUNCTION__, channel);

	// in K1WM clock register is used as output multiplexer register
	K1WM_channel(in) = channel;
	return K1WM_channel(in) == channel ? gbGOOD : gbBAD;
}

static GOOD_OR_BAD K1WM_create_channels(struct connection_in *head, int channels_count)
{
	int i;

	static char *channel_names[] = {
		"K1WM(0)", "K1WM(1)", "K1WM(2)", "K1WM(3)", "K1WM(4)", "K1WM(5)", "K1WM(6)", "K1WM(7)",
		"K1WM(8)", "K1WM(9)", "K1WM(10)", "K1WM(11)", "K1WM(12)", "K1WM(13)", "K1WM(14)", "K1WM(15)",
		"K1WM(16)", "K1WM(17)", "K1WM(18)", "K1WM(19)", "K1WM(20)", "K1WM(21)", "K1WM(22)", "K1WM(23)",
		"K1WM(24)", "K1WM(25)", "K1WM(26)", "K1WM(27)", "K1WM(28)", "K1WM(29)", "K1WM(30)", "K1WM(31)",
		"K1WM(32)", "K1WM(33)", "K1WM(34)", "K1WM(35)", "K1WM(36)", "K1WM(37)", "K1WM(38)", "K1WM(39)",
		"K1WM(40)", "K1WM(41)", "K1WM(42)", "K1WM(43)", "K1WM(44)", "K1WM(45)", "K1WM(46)", "K1WM(47)",
		"K1WM(48)", "K1WM(49)", "K1WM(50)", "K1WM(51)", "K1WM(52)", "K1WM(53)", "K1WM(54)", "K1WM(55)",
		"K1WM(56)", "K1WM(57)", "K1WM(58)", "K1WM(59)", "K1WM(60)", "K1WM(61)", "K1WM(62)", "K1WM(63)"
	};
	head->master.ds1wm.active_channel = 0;
	head->adapter_name = channel_names[0] ;
	for (i = 1; i < channels_count; ++i) {
		struct connection_in * added = AddtoPort(head->pown);
		if (added == NO_CONNECTION) {
			return gbBAD;
		}
		added->master.ds1wm.active_channel = i;
		added->adapter_name = channel_names[i] ;
	}
	return gbGOOD;
}

