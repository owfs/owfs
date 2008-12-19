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
static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int HA7E_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn);
static int HA7E_CR(const struct parsedname *pn);
static void HA7E_setroutines(struct connection_in *in);
static void HA7E_close(struct connection_in *in);
static int HA7E_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static int HA7E_select( const struct parsedname * pn ) ;

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
	in->iroutines.flags = ADAP_FLAG_2409path;
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

		COM_flush(&pn);

		in->Adapter = adapter_HA7E ;
		in->adapter_name = "HA7E/S";
		return 0;
	}
	LEVEL_DEFAULT("HA7E detection error\n");
	return -ENODEV;
}

static int HA7E_reset(const struct parsedname *pn)
{
	BYTE resp[1];

	COM_flush(pn);
    if (COM_write("R", 1, pn)) {
        LEVEL_DEBUG("Error sending HA7E reset\n");
        return -EIO;
    }
    if (COM_read(resp, 1, pn) || resp[0]!=0x0D) {
        LEVEL_DEBUG("Error reading HA7E reset\n");
        return -EIO;
    }
    return BUS_RESET_OK;
}

static int HA7E_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->connin.link.alarm) : &(pn->selected_connection->connin.link.main);

	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

	COM_flush(pn);

	if (ds->index == -1) {
        BUSLOCK(pn) ;
        ret = HA7E_directory(ds, db, pn) ;
        BUSUNLOCK(pn) ;
        if (ret) {
			return -EIO;
		}
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

static int HA7E_select( const struct parsedname * pn )
{
    char send_address[18] ;
    char resp_address[17] ;
    UCLIBCLOCK ;
    snprintf( send_address, 18, "A%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%c",pn->sn[7],pn->sn[6],pn->sn[5],pn->sn[4],pn->sn[3],pn->sn[2],pn->sn[1],pn->sn[0],0x0D) ;
    UCLIBCUNLOCK ;

    if ( COM_write(send_address,18,pn) ) {
        LEVEL_DEBUG("Error with sending HA7E select\n") ;
        return -EIO ;
    }
    if ( COM_read(resp_address,17,pn) ) {
        LEVEL_DEBUG("Error with reading HA7E select\n") ;
        return -EIO ;
    }
    if ( memcmp( &resp_address[0],&send_address[1],17) ) {
        LEVEL_DEBUG("Error with HA7E select response\n") ;
        return -EIO ;
    }
    return 0 ;
}

//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int HA7E_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t i;
	size_t left;
	BYTE *buf = pn->selected_connection->combuffer;

	if (size == 0) {
		return 0;
	}
	if (COM_write(HA7E_string("b"), 1, pn)) {
		LEVEL_DEBUG("HA7E_sendback_data error sending b\n");
		return -EIO;
	}
//    for ( i=0; ret==0 && i<size ; ++i ) ret = HA7E_byte_bounce( &data[i], &resp[i], pn ) ;
	for (left = size; left;) {
		i = (left > 16) ? 16 : left;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) buf, &data[size - left], i);
		if (COM_write(buf, i << 1, pn) || COM_read(buf, i << 1, pn)) {
			return -EIO;
		}
		string2bytes((char *) buf, &resp[size - left], i);
		left -= i;
	}
	return HA7E_CR(pn);
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) { printf( "%.2X ",b[i] ) ; }
    if ( size ) { printf("\n") ; }
}
*/

static int HA7E_byte_bounce(const BYTE * out, BYTE * in, const struct parsedname *pn)
{
	BYTE data[2];

	num2string((char *) data, out[0]);
	if (COM_write(data, 2, pn) || COM_read(data, 2, pn)) {
		return -EIO;
	}
	in[0] = string2num((char *) data);
	return 0;
}

static int HA7E_CR(const struct parsedname *pn)
{
	BYTE data[3];
	if (COM_write(HA7E_string("\r"), 1, pn) || COM_read(data, 2, pn)) {
		return -EIO;
	}
	LEVEL_DEBUG("HA7E_CR return 0\n");
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
	int ret;

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
		if ((ret = COM_write(current, 1, pn))) {
			return ret;
		}
		current = next ; // set up for next pass
		//One needs to check the first character returned.
		//If nothing is found, the link will timeout rather then have a quick
		//return.  This happens when looking at the alarm directory and
		//there are no alarms pending
		//So we grab the first character and check it.  If not an E leave it
		//in the resp buffer and get the rest of the response from the HA7E
		//device
		if ((ret = (COM_read(resp, 1, pn)))) {
			return ret;
		}
		if ( resp[0] == 0x0D ) {
			return 0 ; // end of list
		}
		if ((ret = (COM_read(&resp[1], 16, pn)))) {
			return ret;
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
        if ( resp[17]!=0x0D ) {
            return -EIO ;
        }

		// CRC check
		if (CRC8(sn, 8) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s\n", sn);
			return -EIO;
		}

		DirblobAdd(sn, db);
	}
}
