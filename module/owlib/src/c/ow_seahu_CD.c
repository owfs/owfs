/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2016 Ondrej Lycka
	email: ondrej.lycka@seznam.cz
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* 
 * Supported ARDUINO chips:
 * 		with 1-Wire open source universal multi sensors and output deveices protocol (OneWireUni protocol)
 * 
 * supported family codes:
 * 		W1_FAMILY_SEAHU_CC  0xCC
 * 
 * 
 * Description
 * -----------
 * 1-Wire open source multi sensors protocol enable in one slave device
 * integrate more sensors and I/O entries (e.g.  thermal sensor, humidity senzor,
 * relay, RGB lights, ...).
 * Source code for firmware, example codes and detailed documentation for create
 * new slave devices is stored on https://github.com/seahu.
 * This drive support up to 32 subdevices into one slave device.
 */
 
/* ------------------------------------------------------------------------------------------------- */


/* Example slave -- 1-wire slave example to show developers how to write support for a new 1-wire device */

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
	( or a closely related family of chips )

	The connection to the larger program is through the "device" data structure,
	  which must be declared in the acompanying header file.

	The device structure holds the
	  family code,
	  name,
	  number of properties,
	  list of property structures, called "filetype".

	Each filetype structure holds the
	  name,
	  estimated length (in bytes), // defined in ow_filetype.h
	  aggregate structure pointer,
	  data format,
	  read function,
	  write funtion,
	  generic data pointer

	The aggregate structure, is present for properties that several members
	(e.g. pages of memory or entries in a temperature log. It holds:
	  number of elements
	  whether the members are lettered or numbered
	  whether the elements are stored together and split, or separately and joined
*/

// Example_slave

/* Notes for a new slave device -- what files to change

A. Property functions:
  1. Create this file with READ_FUNCTION and WRITE_FUNCTION for the different properties
  2. Create the filetype struct for each property
  3. Create the DeviceEntry or DeviceEntryExtended struct
  4. Write all the appropriate property functions
  5. Add any visibility functions if needed
  6. Add this file and the header file to the CVS
 
B. Header
  1. Create a similarly named header file in ../include
  2. Protect inclusion with #ifdef / #define / #endif
  3. Link the DeviceHeader* to the include file

C. Makefile and include
  1. Add this file to Makefile.am
  2. Add the header to ../include/Makefile.am
  3. Add the header file to ../include/devices.h

D. Device tree
  1. Add the appropriate entry to the device tree in
  * ow_tree.c
  * 
  SUMMARY:
  * create the slave_support file (this one) and the (simple) header file
  * add references to the slave in ow_tree.c ow_devices.h and both Makefile.am
  * Add the files the the CVS repository

F. my notice (czech language)
	je potreba vytvorit (new create files):
		module/owlib/src/c/ow_my_slave.c
		module/owlib/src/include/ow_my_slave.h
	dele je potreba upravit (edit files):
		module/owlib/src/c/ow_tree.c
		module/owlib/src/c/Makefile.am
		module/owlib/src/include/ow_devices.h
		module/owlib/src/include/Makefile.am
	pro ucely dokumentace je potreba napsat manualovou stranku a upravit makefile.am:
		src/man/man3/ow_my_slave.man
		src/man/man3/Makefile.am

	vsechny OW transakce (prikazy) jsou definvany v module/owlib/src/include/ow_transaction.h
	datove typy jsou definovany v 			module/owlib/src/include/ow_filetype.h
	zakladni datove typy v				module/owlib/src/include/ow_localtypes.h
	typy pro predavani dat s FS (napr. OWQ_buffer)		module/owlib/src/include/ow_onewirequery.h
	makra pro logy napr.( LEVEL_DEBUG, ERROR_CONNECT,..) v 	module/owlib/src/include/ow_debug.h
	* 
	pro debogovani spustit:
	* sudo /opt/owfs/bin/owhttpd -uall -p 8082 --debug

DEBUG LOG EXAMPLE:
LEVEL_DEBUG("+++++++++++++++++++++++ seahu  READ BEEPER +++++++ duration %s, %d",p_comma+1, general_beeper.duration) ;	
*/ 		
 

#include <config.h>
#include "owfs_config.h"
#include "ow_seahu_CD.h"
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

/* SOME USED CONSTANTS */
#define DEVICE_DESCRIPTION_LENGHT 64 // lenght of device description string
#define DESCRIPTION_LENGHT 64 // lenght of section description string
#define USER_NOTE_LENGHT 32 // lenght of section user note string
#define MAX_MEASURE_TIME CLOCKS_PER_SEC*2 // max. measure time. Test end of measured run in cycle. Every cycle wait 50us + time for 1Wire comunication.
#define TRXN_MAX_SIZE 1+1+64+2+1 // max. size for transaction log (1[cmd]+1[section]+64[data]+2[crc16]+1[confirm])

/* STATUS BYTE MASKS */
#define _MASK_ENABLE_READ		0b00000001
#define _MASK_ENABLE_WRITE		0b00000010
#define _MASK_GIVE_ALARM		0b00000100
#define _MASK_TYPE_VALUE		0b00011000
#define _MASK_STATUS_ALARM		0b00100000
#define _MASK_MIN_ALARM			0b01000000
#define _MASK_MAX_ALARM			0b10000000



/* OW COMMANDS CODE */
#define _1W_START_MEASURE			0x44 // measure command (measure selected section)
#define _1W_READ_DEVICE_DESCRIPTION	0xF1 // read factory description device (64Bytes)
#define _1W_READ_NUMBER_SECTIONS	0xF2 // read number of sections (8bit)
#define _1W_READ_CONTROL_BYTE		0xF3 // read from CONTROL register (8bit) - control register selected section
#define _1W_READ_ACTUAL_VALUE		0xF5 // read from ACTUAL_VALUE register (1-4Byte depends on setting in control register)
#define _1W_READ_DESCRIPTION		0xF7 // read from DESCRIPTION register (32Byte)
#define _1W_READ_USER_NOTE			0xF8 // read from NOTE register (64Bytes)
#define _1W_READ_MIN_ALARM_VALUE	0xFA // read from MIN_ALARM_VALUE register (1-4Bytes depends on setting in control register)
#define _1W_READ_MAX_ALARM_VALUE	0xFB // read from MAX_ALARM_VALUE register (1-4Bytes depends on setting in control register)
#define _1W_WRITE_CONTROL_BYTE		0x53 // write into CONTROL register (8bit) - control register selected section
#define _1W_WRITE_ACTUAL_VALUE		0x55 // write into ACTUAL_VALUE (1-4Bytes depends on setting in control register)
#define _1W_WRITE_MIN_ALARM_VALUE	0x5A // write into MIN_ALARM_VALUE (1-4 Byte depends on setting in control register)
#define _1W_WRITE_MAX_ALARM_VALUE	0x5B // write into MAX_ALARM_VALUE (1-4 Byte depends on setting in control register)
#define _1W_WRITE_USER_NOTE			0x58 // write into NOTE register (64Bytes)

/* OW DELAY */
#define DEVICE_DESCRIPTION_DELAY	100 // delay for slave to read device description from PROM to RAM
#define READ_MIN_MAX_ALARM_DELAY	50  // delay for slave to red min or max alarm value from EEPROM to RAM
#define DESCRIPTION_DELAY			100 // delay for slave to read section description from PROM to RAM
#define READ_USER_NOTE_DEALY		100 // delay for slave to read section user note from EEROM to RAM
#define WRITE_MIN_ALARM_DEALY		100 // delay for slave to store new min or max section value to EEPROM
#define WRITE_USER_NOTE_DELAY		100 // delay for slave to store new section user note to EEPROM


/* ------- Prototypes ----------- */

/* Here are the functions that perform the tasks of the listed properties
 * Unually there is on function for each read and write task, but there is
 * a data field that can be used to distinguish similar tasks and pool the code
 * 
 * The functions get the onw_wire_query structure that contains buffers for data or results
 * and the parsedname structure that contains information on the name, serial number, 
 * extra data and extension
 * 
 * sometimes it makes sense to have an internal (hidden) field that corresponds better
 * to the devices registers, and different "linked" properties that access the register
 * for presentation
 */

/* seahu_CD slave */
READ_FUNCTION(FS_r_device_description);
READ_FUNCTION(FS_r_number_sections); 
READ_FUNCTION(FS_r_control_byte);
READ_FUNCTION(FS_r_control_byte_flags);
READ_FUNCTION(FS_r_measure);
READ_FUNCTION(FS_r_actual_value);
READ_FUNCTION(FS_r_min_alarm_value);
READ_FUNCTION(FS_r_max_alarm_value);
READ_FUNCTION(FS_r_descritpion);
READ_FUNCTION(FS_r_user_note);
WRITE_FUNCTION(FS_w_control_byte);
WRITE_FUNCTION(FS_w_control_byte_flags);
WRITE_FUNCTION(FS_w_actual_value);
WRITE_FUNCTION(FS_w_min_alarm_value);
WRITE_FUNCTION(FS_w_max_alarm_value);
WRITE_FUNCTION(FS_w_user_note);

static enum e_visibility VISIBLE_X(UINT this_section, const struct parsedname * pn );

/* ------- Structures ----------- */

// union representating more types of actual value
typedef union {
    bool            Bool;
    UINT         	uInt;
    INT				Int;
    BYTE			Buf[4];
  } val;

/* Internal properties */
Make_SlaveSpecificTag(MEASURE_TIME, fc_stable);  // measure time

/* ------- Define macro for my F_function ----------- */
#define VISIILITY_FUNCTION(N) \
static enum e_visibility VISIBLE_ ## N(const struct parsedname * pn ) {\
	return VISIBLE_X((N), pn );\
}

VISIILITY_FUNCTION(0);
VISIILITY_FUNCTION(1);
VISIILITY_FUNCTION(2);
VISIILITY_FUNCTION(3);
VISIILITY_FUNCTION(4);
VISIILITY_FUNCTION(5);
VISIILITY_FUNCTION(6);
VISIILITY_FUNCTION(7);
VISIILITY_FUNCTION(8);
VISIILITY_FUNCTION(9);
VISIILITY_FUNCTION(10);
VISIILITY_FUNCTION(11);
VISIILITY_FUNCTION(12);
VISIILITY_FUNCTION(13);
VISIILITY_FUNCTION(14);
VISIILITY_FUNCTION(15);
VISIILITY_FUNCTION(16);
VISIILITY_FUNCTION(17);
VISIILITY_FUNCTION(18);
VISIILITY_FUNCTION(19);
VISIILITY_FUNCTION(20);
VISIILITY_FUNCTION(21);
VISIILITY_FUNCTION(22);
VISIILITY_FUNCTION(23);
VISIILITY_FUNCTION(24);
VISIILITY_FUNCTION(25);
VISIILITY_FUNCTION(26);
VISIILITY_FUNCTION(27);
VISIILITY_FUNCTION(28);
VISIILITY_FUNCTION(29);
VISIILITY_FUNCTION(30);
VISIILITY_FUNCTION(31);


#define F_SUB(N,section,control_byte,enable_read,enable_write,give_alarm,status_alarm,enable_min_alarm,enable_max_alarm,measure,actual_value,min_alarm_value,max_alarm_value,description,user_note)	\
	{#section, PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=(N),}, },\
	{#control_byte, PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_control_byte, FS_w_control_byte, VISIBLE_ ## N, {.u=(N),}, },\
	{#enable_read, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=_MASK_ENABLE_READ*256+(N),}, },\
	{#enable_write, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=_MASK_ENABLE_WRITE*256+(N),}, },\
	{#give_alarm, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=_MASK_GIVE_ALARM*256+(N),}, },\
	{#status_alarm, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=_MASK_STATUS_ALARM*256+(N),}, },\
	{#enable_min_alarm, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, FS_w_control_byte_flags, VISIBLE_ ## N, {.u=_MASK_MIN_ALARM*256+(N),}, },\
	{#enable_max_alarm, PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, FS_w_control_byte_flags, VISIBLE_ ## N, {.u=_MASK_MAX_ALARM*256+(N),}, },\
	{#measure, PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_uncached, FS_r_measure, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=(N),}, },\
	{#actual_value, PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_actual_value, FS_w_actual_value, VISIBLE_ ## N, {.u=(N),}, },\
	{#min_alarm_value, PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_min_alarm_value, FS_w_min_alarm_value, VISIBLE_ ## N, {.u=(N),}, },\
	{#max_alarm_value, PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_max_alarm_value, FS_w_max_alarm_value, VISIBLE_ ## N, {.u=(N),}, },\
	{#description, DESCRIPTION_LENGHT, NON_AGGREGATE, ft_ascii, fc_read_stable, FS_r_descritpion, NO_WRITE_FUNCTION, VISIBLE_ ## N, {.u=(N),}, },	\
	{#user_note, USER_NOTE_LENGHT, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_user_note, FS_w_user_note, VISIBLE_ ## N, {.u=(N),}, }

#define FF_SUB(N) F_SUB(N, section ## N , section ## N/control_byte, section ## N/enable_read, section ## N/enable_write, section ## N/give_alarm, section ## N/status_alarm, section ## N/enable_min_alarm, section ## N/enable_max_alarm, section ## N/measure, section ## N/actual_value, section ## N/min_alarm_value, section ## N/max_alarm_value, section ## N/description, section ## N/user_note)

static struct filetype seahu_CD[] = {
	F_STANDARD,
	{"device_description", DEVICE_DESCRIPTION_LENGHT, NON_AGGREGATE, ft_ascii, fc_read_stable, FS_r_device_description, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"number_sections", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_read_stable, FS_r_number_sections, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	FF_SUB(0),
	FF_SUB(1),
	FF_SUB(2),
	FF_SUB(3),
	FF_SUB(4),
	FF_SUB(5),
	FF_SUB(6),
	FF_SUB(7),
	FF_SUB(8),
	FF_SUB(9),
	FF_SUB(10),
	FF_SUB(11),
	FF_SUB(12),
	FF_SUB(13),
	FF_SUB(14),
	FF_SUB(15),
	FF_SUB(16),
	FF_SUB(17),
	FF_SUB(18),
	FF_SUB(19),
	FF_SUB(20),
	FF_SUB(21),
	FF_SUB(22),
	FF_SUB(23),
	FF_SUB(24),
	FF_SUB(25),
	FF_SUB(26),
	FF_SUB(27),
	FF_SUB(28),
	FF_SUB(29),
	FF_SUB(30),
	FF_SUB(31),
	/*
	{"control_byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_control_byte, FS_w_control_byte, VISIBLE, {.u=0,}, },
	{"actual_control_byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_control_byte, FS_w_control_byte, VISIBLE, {.u=0,}, },
	{"enable_read", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE, {.u=_MASK_ENABLE_READ*256+0,}, },
	{"enable_write", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE, {.u=_MASK_ENABLE_WRITE*256+0,}, },
	{"give_alarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE, {.u=_MASK_GIVE_ALARM*256+0,}, },
	{"status_alarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, NO_WRITE_FUNCTION, VISIBLE, {.u=_MASK_STATUS_ALARM*256+0,}, },
	{"status_min_alarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, FS_w_control_byte_flags, VISIBLE, {.u=_MASK_MIN_ALARM*256+0,}, },
	{"status_max_alarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_uncached, FS_r_control_byte_flags, FS_w_control_byte_flags, VISIBLE, {.u=_MASK_MAX_ALARM*256+0,}, },
	{"measure", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_measure, NO_WRITE_FUNCTION, VISIBLE, {.u=0,}, },
	{"actual_value", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_actual_value, FS_w_actual_value, VISIBLE, {.u=0,}, },
	{"min_alarm_value", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_min_alarm_value, FS_w_min_alarm_value, VISIBLE, {.u=0,}, },
	{"max_alarm_value", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_max_alarm_value, FS_w_max_alarm_value, VISIBLE, {.u=0,}, },
	{"description", DESCRIPTION_LENGHT, NON_AGGREGATE, ft_ascii, fc_read_stable, FS_r_descritpion, NO_WRITE_FUNCTION, VISIBLE, {.u=0,}, },	
	{"user_note", USER_NOTE_LENGHT, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_user_note, FS_w_user_note, VISIBLE, {.u=0,}, },
	{"section0", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_0, {.u=0,}, },
	{"section0/actual_control_byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_control_byte, FS_w_control_byte, VISIBLE_0, {.u=0,}, },
	{"section1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_1, {.u=1,}, },
	{"section1/actual_control_byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_control_byte, FS_w_control_byte, VISIBLE_1, {.u=1,}, },
	{"section2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_2, {.u=2,}, },
	{"section2/actual_control_byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_control_byte, FS_w_control_byte, VISIBLE_2, {.u=2,}, },
	*/
};

DeviceEntryExtended(CD, seahu_CD, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* --------Cache functions ----*/

/*
 * get section number from *owq , 
 * section read from  *pn->selected_filetype->data.u  of *owq 
 * auxliary function use in cahe and other functions
 */
BYTE get_section_number( struct one_wire_query *owq )
{
	struct parsedname *pn = PN(owq);
	UINT this_section = pn->selected_filetype->data.u; // mask contain _MASK_FLAG*0xFF + number of section
	this_section =  this_section & 0x00FF; // filter only fris byte define this section
	return (BYTE)this_section;
}


/*
 * Get controlByte from subdirectory file. By modify function FS_r_sibling_U to add name of actual subdirectory into sibling name
 */
ZERO_OR_ERROR get_controlByte_from_SUB_FSS(  UINT *U, struct one_wire_query *owq){
	char path[PATH_MAX+3]; // BYTE for sprintf is 0-255 => max +3 digits (bytes)
	sprintf(path,"section%d/control_byte", get_section_number(owq) );
	return FS_r_sibling_U( U, path, owq );
}

/*
 * Set controlByte from subdirectory file. By modify function FS_r_sibling_U to add name of actual subdirectory into sibling name
 */
ZERO_OR_ERROR set_controlByte_from_SUB_FSS(  UINT U, struct one_wire_query *owq){
	char path[PATH_MAX+3]; // BYTE for sprintf is 0-255 => max +3 digits (bytes)
	sprintf(path,"section%d/control_byte",get_section_number(owq));
	return FS_w_sibling_U( U, path, owq );
}

/* 
 * get number of sections by *pn 
 * (can't use *owq becouse this function is used for VISIBILITY and at this time *owq is not exist) 
 */
GOOD_OR_BAD get_number_sectionss_from_SUB_FSS( UINT *p_number_sections, const struct parsedname * pn )
{
	UINT number;
	LEVEL_DEBUG("Checking number of sections %s",SAFESTRING(pn->path)) ;
	struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
	if ( owq == NULL) return gbBAD;
	if (FS_r_sibling_U( &number, "number_sections", owq ) != 0) {
		OWQ_destroy(owq) ;
		return gbBAD;
	}
	*p_number_sections=number;
	OWQ_destroy(owq) ;
	return gbGOOD;
}

/* ------- VISIBILITY ------------ */
static enum e_visibility VISIBLE_X(UINT this_section, const struct parsedname * pn )
{
	UINT number_sections=0;
	get_number_sectionss_from_SUB_FSS( &number_sections, pn ); // get number section trought cache
	if (this_section<number_sections) return visible_now ;
	else return visible_not_now ;
}

/* ------- Functions ------------ */
/*---------------------------- auxliary functions ---------------------------------------- */

/*
 * caculate crc16 - inspirated by module/owlib/src/c/ow_crc.c
 */

UINT crc16(const BYTE * bytes, const size_t length, const UINT seed)
{
	static UINT crc16_table[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };
	UINT crc = seed;
	size_t i;

	for (i = 0; i < length; ++i) {
		UINT c = (bytes[i] ^ (crc & 0xFF)) & 0xFF;
		crc >>= 8;
		if (crc16_table[c & 0x0F] ^ crc16_table[c >> 4]) {
			crc ^= 0xC001;
		}
		crc ^= (c <<= 6);
		crc ^= (c << 1);
	}
	return crc;
}

/* 
 * read x bytes from slave device with check (withow section number)
 * 
 * @param cmd    - command code
 * @param buf    - pointer to will be data store
 * @param Delay  - delay in [us] to slave prepare data
 * @param pn     - pointer to parsename structure
 * @return       - gbGOOD or gbBAD or gbOTHER
 */
GOOD_OR_BAD OW_read_from_device(BYTE cmd, void * buf, UINT len, UINT Delay, const struct parsedname *pn)
{
	BYTE p[TRXN_MAX_SIZE]={cmd, }; // p[1+len+2]
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[0]), // send cmd
		TRXN_DELAY(Delay), // some time for slave to sum CRC 
		TRXN_READ(&p[1],len), // read data
		TRXN_READ(&p[1+len],2), // read crc16
		TRXN_CRC16(p,1+len+2), // check crc16
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn));
	memcpy(buf,&p[1],len);
	return gbGOOD;
}

/*
 *  read x bytes from slave device with check with section number
 * 
 * @param cmd     - command code
 * @param section - number of section
 * @param buf     - pointer to will be data store
 * @param Delay   - delay in [us] to slave prepare data
 * @param pn      - pointer to parsename structure
 * @return        - gbGOOD or gbBAD or gbOTHER
 */
GOOD_OR_BAD OW_read_from_section(BYTE cmd, BYTE section ,void * buf, UINT len, UINT Delay, const struct parsedname *pn)
{
	BYTE p[2+64+2]={cmd, section, }; // p[2+len+2]

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[0]), // send cmd
		TRXN_WRITE1(&p[1]), // send section
		TRXN_DELAY(Delay), // some time for slave to sum CRC 
		TRXN_READ(&p[2],len), // read data
		TRXN_READ(&p[2+len],2), // read crc16
		TRXN_CRC16(p,2+len+2), // check crc16
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn));
	memcpy(buf, &p[2], len);
	return gbGOOD;
}

/* 
 * write x bytes to slave ow device with section number and check
 * 
 * @param cmd     - command code
 * @param section - number of section
 * @param buf     - pointer to will be data store
 * @param Delay   - delay in [us] to slave prepare data
 * @param pn      - pointer to parsename structure
 * @return        - gbGOOD or gbBAD or gbOTHER
 */
GOOD_OR_BAD OW_write_to_section(BYTE cmd, BYTE section, void *buf, UINT len, UINT Delay, const struct parsedname *pn)
{
	BYTE p[2+64+2+1]={cmd, section, }; // p[2+len+2+1]
	memcpy(&p[2], buf, len);
	UINT * crc=(UINT *)&p[2+len]; // define crc pointer placed into p[]
	*crc=~crc16(&p[0],2+len,0); // crc from cmd, number section and data (from buf)
	BYTE check_OK=0xAA; // prepare value to check

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[0]), // send cmd
		TRXN_WRITE1(&p[1]), // send section
		TRXN_WRITE(&p[2],len), // write data
		TRXN_WRITE(&p[2+len],2), // write crc16
		TRXN_DELAY(Delay), // some time for slave to store data
		TRXN_READ1(&p[2+len+2]), // check answer (0xAA = OK)
		TRXN_COMPARE(&p[2+len+2], &check_OK, 1), // compare check answer with 0xAA
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn));
	return gbGOOD;
}

/*
 * READ VALUE (used for actual, min and max value)
 * cmd must be code for command read/write actualValue, min_alarm_value or max_alarm_value
 * (number of section is get from *owg)
 * 
 * @param cmd     - command code
 * @param Delay   - delay in [us] to slave prepare data
 * @param owq     - pointer to one wire query structure
 * @return        - 0 for succesfully or other int value as error
 */
static ZERO_OR_ERROR read_value(BYTE cmd, UINT Delay, struct one_wire_query *owq){
	UINT controlByte;
	val value;
	value.uInt=0; // prepare zerro value

	BYTE section = get_section_number( owq ); // get section number for *owq
	RETURN_ERROR_IF_BAD( get_controlByte_from_SUB_FSS(  &controlByte, owq) ); // get control byte through cache
	// read data
	RETURN_ERROR_IF_BAD( OW_read_from_section(cmd, section, &value.Buf, 4, Delay, PN(owq)) ); // OWW_read_bytes(cmd, *section, *buf, len, Delay, *pn)
	// output format data
	if ( (controlByte & _MASK_TYPE_VALUE) == 0b00011000 ) OWQ_U(owq) = value.Int; // return signet value
	else OWQ_I(owq) = value.uInt;	//return unsigned value
	return 0;
}

/*
 * WRITE VALUE (used for actual, min and max value)
 * cmd must be code for command read/write actualValue, min_alarm_value or max_alarm_value
 * (number of section is get from *owg)
 * 
 * @param cmd     - command code
 * @param Delay   - delay in [us] to slave prepare data
 * @param owq     - pointer to one wire query structure
 * @return        - 0 for succesfully or other int value as error
 */
static ZERO_OR_ERROR write_value(BYTE cmd, UINT Delay, struct one_wire_query *owq){
	UINT controlByte;
	val value;

	BYTE section = get_section_number( owq ); // get section number for *owq
	RETURN_ERROR_IF_BAD( get_controlByte_from_SUB_FSS(  &controlByte, owq) ); // get control byte through cache
	// fill up value union
	if ( (controlByte & _MASK_TYPE_VALUE) == 0b00011000 ) value.uInt = OWQ_U(owq); // get signet value
	else value.Int = OWQ_I(owq);	//get  unsigned value
	//write filled data
	RETURN_ERROR_IF_BAD( OW_write_to_section(cmd, section, &value.Buf, 4, Delay, PN(owq)) );
	return 0;
}

/*------- other auxliary functions is in ow_seahu.h ------- */

/* ---------------------------- FS FUNCTION ------------------------------------------*/

/*
 * read user note
 */
static ZERO_OR_ERROR FS_r_device_description(struct one_wire_query *owq)
{
	if ( OWQ_offset(owq) != 0 )					return -ERANGE;
	if ( OWQ_size(owq) != DEVICE_DESCRIPTION_LENGHT )	return -ERANGE;
	RETURN_ERROR_IF_BAD( OW_read_from_device(_1W_READ_DEVICE_DESCRIPTION, OWQ_buffer(owq), DEVICE_DESCRIPTION_LENGHT, DEVICE_DESCRIPTION_DELAY, PN(owq)) );
	OWQ_length(owq) = OWQ_size(owq);
	return 0;
}

/* 
 * Read number of sections (every section representate one sensor or output device). Return int value. 
 */
static ZERO_OR_ERROR FS_r_number_sections(struct one_wire_query *owq)
{
	BYTE number;
	RETURN_ERROR_IF_BAD( OW_read_from_device(_1W_READ_NUMBER_SECTIONS, &number, 1, 0, PN(owq)) );
	OWQ_U(owq) = number;
	return 0 ; // no error
}

/* 
 * Read control register. This register control type device (input, ouutput or input/output, use or no use alarm, define bit resolution, ...).
 */
static ZERO_OR_ERROR FS_r_control_byte(struct one_wire_query *owq)
{
	BYTE control_byte;
	BYTE section = get_section_number( owq ); // get section number for *owq
	
	RETURN_ERROR_IF_BAD( OW_read_from_section(_1W_READ_CONTROL_BYTE, section, &control_byte, 1, 0, PN(owq)) );
	OWQ_U(owq) = control_byte;
	return 0 ; // no error
}

/* 
 * Return flag status of control byte 
 * used for detec if section have readable actual value, writeable actual value, have alarm functionality, is it in alarm
 * have set min or max alarm
 */
static ZERO_OR_ERROR FS_r_control_byte_flags(struct one_wire_query *owq)
{
	UINT controlByte;
	struct parsedname *pn = PN(owq);
	UINT mask = pn->selected_filetype->data.u; // mask contain _MASK_FLAG*0xFF + number of section
	mask = (mask & 0xFF00) >> 8; // filter only _MASK_FLAG

	RETURN_ERROR_IF_BAD( get_controlByte_from_SUB_FSS(&controlByte, owq) ); // get control byte through cache
	if ( controlByte & mask ) 	OWQ_U(owq) = 1;
	else 						OWQ_U(owq) = 0;
	return 0 ; // no error
}


/* 
 * Read measured value. Send 1-Wire measure command and repadly wait while will ready to read and return int value. 
 * Time for wait will be store to save nest one wire communication trafick
 */
static ZERO_OR_ERROR FS_r_measure(struct one_wire_query *owq)
// v prubehu cekani by to chtelo chackovat control_byte a take ulozit delku cekani, aby se priste mohl omezil pocet dotazu pri cekani na novaou hodnotu
// toto je jedna z nejslozitejsich fci zatim ji osidim pak to budu zkoumat jak to udelat lip
{
	//return FS_r_actual_value(owq);
	clock_t t1, t2, t3; // timestamp in millis
	BYTE section = get_section_number( owq ); // get section number for *owq
	
	// send measure command
	RETURN_ERROR_IF_BAD( OW_write_to_section(_1W_START_MEASURE, section, NULL, 0, 0, PN(owq)) );

	// try read new value
	BYTE control_byte;
	t1=clock();
	if( BAD(Cache_Get_SlaveSpecific(&t3, sizeof(clock_t), SlaveSpecificTag(MEASURE_TIME), PN(owq)) )) t3=0; // get last saved measure time
	UT_delay(1000*t3/CLOCKS_PER_SEC); // wait some time as last (save 1-wire bus trafic)
	// start waiting cycle to measure be ready
	while ( (clock()-t1) < MAX_MEASURE_TIME ) {
		if ( OW_read_from_section(_1W_READ_CONTROL_BYTE, section, &control_byte, 1, 0, PN(owq))==gbGOOD ) {
			if (control_byte & _MASK_ENABLE_READ) {
				t2=clock()-t1;
				if (t2>t3 ) t3=t2;
				if (t3>MAX_MEASURE_TIME) t3=0;
				Cache_Add_SlaveSpecific(&t3, sizeof(clock_t), SlaveSpecificTag(MEASURE_TIME), PN(owq));
				return FS_r_actual_value(owq); //measure done
			}
		}
		UT_delay(50);
	}
	// error to log measure time
	t3=clock()-t1;
	t3=0;
	Cache_Add_SlaveSpecific(&t3, sizeof(clock_t), SlaveSpecificTag(MEASURE_TIME), PN(owq));
	ERROR_DATA ("To long measure.");
	return -EINVAL ;
}

/*
 * read actual value
 */
static ZERO_OR_ERROR FS_r_actual_value(struct one_wire_query *owq)
{
	return read_value(_1W_READ_ACTUAL_VALUE, 0, owq);
}

/*
 * read minimal alarm value
 */
static ZERO_OR_ERROR FS_r_min_alarm_value(struct one_wire_query *owq)
{
	return read_value(_1W_READ_MIN_ALARM_VALUE, READ_MIN_MAX_ALARM_DELAY, owq);
}

/*
 * read maximal alarm value
 */
static ZERO_OR_ERROR FS_r_max_alarm_value(struct one_wire_query *owq)
{
	return read_value(_1W_READ_MAX_ALARM_VALUE, READ_MIN_MAX_ALARM_DELAY, owq);
}

/*
 * read description
 */
static ZERO_OR_ERROR FS_r_descritpion(struct one_wire_query *owq)
{
	if ( OWQ_offset(owq) != 0 )					return -ERANGE;
	if ( OWQ_size(owq) != DESCRIPTION_LENGHT )	return -ERANGE;
	BYTE section = get_section_number( owq ); // get section number for *owq

	RETURN_ERROR_IF_BAD( OW_read_from_section(_1W_READ_DESCRIPTION, section, OWQ_buffer(owq), DESCRIPTION_LENGHT, DESCRIPTION_DELAY, PN(owq)) );
	OWQ_length(owq) = OWQ_size(owq);
	return 0;
}

/*
 * read user note
 */
static ZERO_OR_ERROR FS_r_user_note(struct one_wire_query *owq)
{
	if ( OWQ_offset(owq) != 0 )					return -ERANGE;
	if ( OWQ_size(owq) != USER_NOTE_LENGHT )	return -ERANGE;
	BYTE section = get_section_number( owq ); // get section number for *owq

	RETURN_ERROR_IF_BAD( OW_read_from_section(_1W_READ_USER_NOTE, section, OWQ_buffer(owq), USER_NOTE_LENGHT, READ_USER_NOTE_DEALY, PN(owq)) );
	OWQ_length(owq) = OWQ_size(owq);
	return 0;
}


/*
 * write control byte
 */
static ZERO_OR_ERROR FS_w_control_byte(struct one_wire_query *owq)
{
	BYTE section = get_section_number( owq ); // get section number for *owq
	
	RETURN_ERROR_IF_BAD( OW_write_to_section(_1W_WRITE_CONTROL_BYTE, section, &OWQ_U(owq), 1, 0, PN(owq)) );
	return 0 ; // no error
}

/* 
 * write into control byte help flags mask. Used for setting min and max alarm .
 */
static ZERO_OR_ERROR FS_w_control_byte_flags(struct one_wire_query *owq)
{
	UINT controlByte;
	// get actual controlByte through cache
	RETURN_ERROR_IF_BAD( get_controlByte_from_SUB_FSS(&controlByte, owq) ); // get control byte through cache
	// get mask form filetype parametrs
	struct parsedname *pn = PN(owq);
	UINT mask = pn->selected_filetype->data.u; // mask contain _MASK_FLAG*0xFF + number of section
	mask = (mask & 0xFF00) >> 8; // filter only _MASK_FLAG
	//make new controlByte value
	if ( OWQ_U(owq) == 1 ) 	controlByte=(controlByte | mask);
	else 					controlByte=(controlByte & (~mask));
	// write new value controlByte through cache
	RETURN_ERROR_IF_BAD( set_controlByte_from_SUB_FSS(controlByte, owq) ); // get control byte through cache
	return 0 ; // no error
}

/*
 * write actual value
 */
static ZERO_OR_ERROR FS_w_actual_value(struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD( write_value(_1W_WRITE_ACTUAL_VALUE, 0, owq) );
	return 0 ; // no error
}

/*
 * write minimum alarm value
 */
static ZERO_OR_ERROR FS_w_min_alarm_value(struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD( write_value(_1W_WRITE_MIN_ALARM_VALUE, WRITE_MIN_ALARM_DEALY, owq) );
	return 0 ; // no error
}

/*
 * write maximum alarm value
 */
static ZERO_OR_ERROR FS_w_max_alarm_value(struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD( write_value(_1W_WRITE_MAX_ALARM_VALUE, WRITE_MIN_ALARM_DEALY, owq) );
	return 0 ; // no error
}

/*
 * write user note
 */
static ZERO_OR_ERROR FS_w_user_note(struct one_wire_query *owq)
{
	if ( OWQ_offset(owq) != 0 )					return -ERANGE;
	if ( OWQ_size(owq) > USER_NOTE_LENGHT )		return -ERANGE;
	BYTE section = get_section_number( owq ); // get section number for *owq
	BYTE buf[USER_NOTE_LENGHT]; // I must write all 64B (USER_NOTE_LENGHT)

	memset(buf,0,USER_NOTE_LENGHT);
	memcpy(buf,OWQ_buffer(owq),OWQ_size(owq));
	
	RETURN_ERROR_IF_BAD( OW_write_to_section(_1W_WRITE_USER_NOTE, section, buf, USER_NOTE_LENGHT, WRITE_USER_NOTE_DELAY, PN(owq)) );
	OWQ_length(owq) = 0;
	return 0 ; // no error
}
