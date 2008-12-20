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
#include "ow_codes.h"

struct HA7E_id {
	char verstring[36];
	char name[30];
	enum adapter_type Adapter;
};

//static void byteprint( const BYTE * b, int size ) ;
static int HA7E_reset(const struct parsedname *pn);
static int HA7E_next_both(struct device_search *ds, const struct parsedname *pn);
static int HA7E_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;
static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void HA7E_setroutines(struct connection_in *in);
static void HA7E_close(struct connection_in *in);
static int HA7E_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static int HA7E_select( const struct parsedname * pn ) ;
static int HA7E_resync( const struct parsedname * pn ) ;

static void HA7E_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA7E_detect;
	in->iroutines.reset = HA7E_reset;
	in->iroutines.next_both = HA7E_next_both;
	in->iroutines.PowerByte = NULL;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = HA7E_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = HA7E_select ;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = HA7E_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_2409path | ADAP_FLAG_dir_auto_reset ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

#define HA7E_string(x)  ((BYTE *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
// bus locking done at a higher level
// Looks up the device by comparing the version strings to the ones in the
// HA7E_id_table
int HA7E_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	HA7E_setroutines(in);

	/* Initialize dir-at-once structures */
	DirblobInit(&(in->connin.ha7e.main));
	DirblobInit(&(in->connin.ha7e.alarm));


	/* Open the com port */
	if (COM_open(in)) {
		return -ENODEV;
	}

	// set the baud rate to 9600. (Already set to 9600 in COM_open())
	COM_speed(B9600, &pn);
	//COM_flush(&pn);
	if (HA7E_reset(&pn) == BUS_RESET_OK ) {
		in->Adapter = adapter_HA7E ;
		in->adapter_name = "HA7E/S";
		in->AnyDevices = 1 ;
		return 0;
	} else if (HA7E_reset(&pn) == BUS_RESET_OK ) {
		in->Adapter = adapter_HA7E ;
		in->adapter_name = "HA7E/S";
		in->AnyDevices = 1 ;
		return 0;
	}
	LEVEL_DEFAULT("HA7E detection error\n");
	return -ENODEV;
}

static int HA7E_reset(const struct parsedname *pn)
{
	BYTE resp[1];

	COM_flush(pn);
	if (COM_write((BYTE*)"R", 1, pn)) {
		LEVEL_DEBUG("Error sending HA7E reset\n");
		return -EIO;
	}
	if (COM_read(resp, 1, pn)) {
		LEVEL_DEBUG("Error reading HA7E reset\n");
		return -EIO;
	}
	if (resp[0]!=0x0D) {
		LEVEL_DEBUG("Error HA7E reset bad <cr>\n");
		return -EIO;
	}
	return BUS_RESET_OK;
}

static int HA7E_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->connin.link.alarm) : &(pn->selected_connection->connin.link.main);

	if (ds->LastDevice) {
		return -ENODEV;
	}

	COM_flush(pn);

	if (ds->index == -1) {
		printf("First pass -- fill the dirblob\n");
		BUSLOCK(pn) ;
		ret = HA7E_directory(ds, db, pn) ;
		BUSUNLOCK(pn) ;
		if (ret) {
			return -EIO;
		}
	} else {
		printf("Not First pass\n");
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d\n", ds->index);

	ret = DirblobGet(ds->index, ds->sn, db);
	LEVEL_DEBUG("DirblobGet %d\n", ret);
	switch (ret) {
	case 0:
		if ((ds->sn[0] & 0x7F) == 0x04) {
			/* We found a DS1994/DS2404 which require longer delays */
			pn->selected_connection->ds2404_compliance = 1;
		}
		break;
	case -ENODEV:
		ds->LastDevice = 1;
		break;
	}

	LEVEL_DEBUG("HA7E_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return ret;
}

static int HA7E_resync( const struct parsedname * pn )
{
	COM_flush(pn);
	HA7E_reset(pn);
	COM_flush(pn);
	pn->selected_connection->connin.ha7e.sn[0] = 0 ; // so won't match
	return -EIO ;
}

static int HA7E_select( const struct parsedname * pn )
{
	char send_address[18] ;
	char resp_address[17] ;

	UCLIBCLOCK ;
	snprintf( send_address, 18, "A%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%c",pn->sn[7],pn->sn[6],pn->sn[5],pn->sn[4],pn->sn[3],pn->sn[2],pn->sn[1],pn->sn[0],0x0D) ;
	UCLIBCUNLOCK ;

	if ( memcmp( pn->sn, pn->selected_connection->connin.ha7e.sn, 8 ) ) {
		if ( COM_write((BYTE*)send_address,18,pn) ) {
			LEVEL_DEBUG("Error with sending HA7E A-ddress\n") ;
			return HA7E_resync(pn) ;
		}
	} else {
		if ( COM_write((BYTE*)"M",1,pn) ) {
			LEVEL_DEBUG("Error with sending HA7E M-atch\n") ;
			return HA7E_resync(pn) ;
		}
	}
	if ( COM_read((BYTE*)resp_address,17,pn) ) {
		LEVEL_DEBUG("Error with reading HA7E select\n") ;
		return HA7E_resync(pn) ;
	}
	if ( memcmp( &resp_address[0],&send_address[1],17) ) {
		LEVEL_DEBUG("Error with HA7E select response\n") ;
		return HA7E_resync(pn) ;
	}
	return 0 ;
}

//  Send data and return response block -- up to 32 bytes
static int HA7E_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	char send_data[1+2+32*2+1] ;
	char get_data[32*2+1] ;
	size_t i ;
	char * send_pointer = send_data ;

	*(send_pointer++) = 'W' ;

	num2string( send_pointer, size ) ;
	send_pointer += 2 ;

	for ( i=0 ; i<size ; ++i ) {
		num2string( send_pointer, data[1] ) ;
		send_pointer += 2 ;
	}

	*send_pointer = 0x0D ;
	
	if ( COM_write((BYTE*)send_data,size*2+4,pn) ) {
		LEVEL_DEBUG("Error with sending HA7E block\n") ;
		return HA7E_resync(pn) ;
	}
	if ( COM_read((BYTE*)get_data,size*2+1,pn) ) {
		LEVEL_DEBUG("Error with reading HA7E block\n") ;
		return HA7E_resync(pn) ;
	}

	for ( i=0 ; i<size ; ++i ) {
		resp[i] = string2num( &get_data[2*i] ) ;
	}
	return 0 ;
}
	


static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;

	for ( left=size ; left>0 ; left -= 32 ) {
		size_t start = size - left ;
		size_t pass_size = (left>32)?32:left ;
		if ( HA7E_sendback_part( &data[start], &resp[start], pass_size, pn ) ) {
			return -EIO ;
		}
	}
	return 0;
}

/********************************************************/
/* HA7E_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void HA7E_close(struct connection_in *in)
{
	DirblobClear(&(in->connin.ha7e.main));
	DirblobClear(&(in->connin.ha7e.alarm));
	COM_close(in);

}

/************************************************************************/
/*									                                     */
/*	HA7E_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/*									                                     */
/************************************************************************/
static int HA7E_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	char resp[17];
	BYTE sn[8];
	char *first, *next, *current ;

	DirblobClear(db);

	//Depending on the search type, the HA7E search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching

	// Send the configuration command and check response
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		first = "C" ;
		next = "c" ;
	} else {
		first = "S" ;
		next = "s" ;
	}
	current = first ;

	while (1) {
		if (COM_write((BYTE*)current, 1, pn)) {
			return HA7E_resync(pn) ;
		}
		current = next ; // set up for next pass
		//One needs to check the first character returned.
		//If nothing is found, the link will timeout rather then have a quick
		//return.  This happens when looking at the alarm directory and
		//there are no alarms pending
		//So we grab the first character and check it.  If not an E leave it
		//in the resp buffer and get the rest of the response from the HA7E
		//device
		if (COM_read((BYTE*)resp, 1, pn)) {
			return HA7E_resync(pn) ;
		}
		if ( resp[0] == 0x0D ) {
			return 0 ; // end of list
		}
		if (COM_read((BYTE*)&resp[1], 16, pn)) {
			return HA7E_resync(pn) ;
		}
		sn[7] = string2num(&resp[0]);
		sn[6] = string2num(&resp[2]);
		sn[5] = string2num(&resp[4]);
		sn[4] = string2num(&resp[6]);
		sn[3] = string2num(&resp[8]);
		sn[2] = string2num(&resp[10]);
		sn[1] = string2num(&resp[12]);
		sn[0] = string2num(&resp[14]);

		memcpy( pn->selected_connection->connin.ha7e.sn, sn, 8) ;
		LEVEL_DEBUG("HA7E_directory SN found: " SNformat "\n", SNvar(sn));
		if ( resp[16]!=0x0D ) {
			return HA7E_resync(pn) ;
		}

		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s\n", sn);
			return HA7E_resync(pn) ;
		}

		DirblobAdd(sn, db);
	}
}
