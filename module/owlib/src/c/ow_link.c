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

struct LINK_id {
	char verstring[36];
	char name[30];
	enum adapter_type Adapter;
	};

// Steven Bauer added code for the VM links
static struct LINK_id LINK_id_tbl[] = {
	{"1.0","LINK v1.0",adapter_LINK_10},
	{"1.1","LINK v1.1",adapter_LINK_11},
	{"1.2","LINK v1.2",adapter_LINK_12},
	{"VM12a","LINK OEM v1.2a",adapter_LINK_12},
	{"VM12","LINK OEM v1.2",adapter_LINK_12},
	{"0","0",0}};


//static void byteprint( const BYTE * b, int size ) ;
static int LINK_read(BYTE * buf, const size_t size,
					 const struct parsedname *pn);
static int LINK_write(const BYTE * buf, const size_t size,
                      const struct parsedname *pn);
static int LINK_reset(const struct parsedname *pn);
static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn);
static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn);
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t len,
							  const struct parsedname *pn);
static int LINK_byte_bounce(const BYTE * out, BYTE * in,
							const struct parsedname *pn);
static int LINK_CR(const struct parsedname *pn);
static int LINK_setroutines(struct connection_in *in);
static void LINK_close (struct connection_in *in);
static int LINK_directory(struct device_search *ds, struct dirblob *db, 
			  const struct parsedname *pn);

static int LINK_setroutines(struct connection_in *in)
{
	in->iroutines.detect = LINK_detect;
    in->iroutines.reset = LINK_reset;
    in->iroutines.next_both = LINK_next_both;
    in->iroutines.PowerByte = LINK_PowerByte;
//    in->iroutines.ProgramPulse = ;
    in->iroutines.sendback_data = LINK_sendback_data;
//    in->iroutines.sendback_bits = ;
    in->iroutines.select = NULL;
    in->iroutines.reconnect = NULL;
    in->iroutines.close = LINK_close;
    in->iroutines.transaction = NULL;
    in->iroutines.flags = ADAP_FLAG_2409path;
    in->combuffer_length = 0 ; // no buffer needed
    return 0 ;
}

#define LINK_string(x)  ((BYTE *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
// bus locking done at a higher level
// Looks up the device by comparing the version strings to the ones in the
// LINK_id_table
int LINK_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
    if ( LINK_setroutines(in) ) {
        return -ENOMEM ;
    }

	/* Initialize dir-at-once structures */
	DirblobInit (&(in->connin.link.main));
	DirblobInit (&(in->connin.link.alarm));


	/* Open the com port */
	if (COM_open(in))
		return -ENODEV;

	// set the baud rate to 9600. (Already set to 9600 in COM_open())
	COM_speed(B9600, &pn);
	//COM_flush(&pn);
	if (LINK_reset(&pn) == BUS_RESET_OK 
		&& LINK_write(LINK_string(" "), 1, &pn) == 0) {

		BYTE version_read_in[36] = "(none)";
		char *version_pointer = (char *) version_read_in;

		/* read the version string */
		LEVEL_DEBUG ("Checking LINK version\n");

		memset(version_read_in, 0, 36);
		LINK_read(version_read_in, 36, &pn);	// ignore return value -- will time out, probably
		Debug_Bytes("Read version from link",version_read_in,36);

		COM_flush(&pn);

		/* Now find the dot for the version parsing */
		if (version_pointer) {
            int version_index ;
            for (version_index=0; LINK_id_tbl[version_index].verstring[0]!='0'; version_index++ ) {
                if (strstr(version_pointer,LINK_id_tbl[version_index].verstring)!=NULL) {
        			LEVEL_DEBUG("Link version Found %s\n",LINK_id_tbl[version_index].verstring);
                    in->Adapter = LINK_id_tbl[version_index].Adapter;
                    in->adapter_name=LINK_id_tbl[version_index].name;
                    return 0;
    			}
		    }
		}
	}
	LEVEL_DEFAULT("LINK detection error\n");
	return -ENODEV;
}

static int LINK_reset(const struct parsedname *pn)
{
	BYTE resp[5];
	int ret ;
	
	COM_flush(pn);
	//if (LINK_write(LINK_string("\rr"), 2, pn) || LINK_read(resp, 4, pn, 1)) {
	//Response is 3 bytes:  1 byte for code + \r\n
	if (LINK_write(LINK_string("r"), 1, pn) || LINK_read(resp, 3, pn)) {
		LEVEL_DEBUG("Error resetting LINK device\n");
		return -EIO;
	}
	
    switch (resp[0]) {

	case 'P':
		LEVEL_DEBUG("LINK reset ok, devices Present\n");
		ret = BUS_RESET_OK ;
		pn->selected_connection->AnyDevices = 1;
		break;

	case 'N':
		LEVEL_DEBUG("LINK reset ok, devices Not present\n");
		ret = BUS_RESET_OK ;
		pn->selected_connection->AnyDevices = 0;
		break;

	case 'S':
		LEVEL_DEBUG("LINK reset short, Short circuit on 1-wire bus!\n");
		ret = BUS_RESET_SHORT ;
		break ;

	default:
		LEVEL_DEBUG("LINK reset bad, Unknown LINK response %c\n",resp[0]);
		ret = -EIO ;
		break ;
	}
	
	return ret;
}

static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn)
{
	int ret=0;
	struct dirblob * db = (ds->search ==_1W_CONDITIONAL_SEARCH_ROM) ?
				&(pn->selected_connection->connin.link.alarm) :
				&(pn->selected_connection->connin.link.main);

	if (!pn->selected_connection->AnyDevices)
		ds->LastDevice = 1;
	if (ds->LastDevice)
		return -ENODEV;

	COM_flush(pn);

	if (ds->index == -1) {
        if (LINK_directory(ds,db, pn)) {
			return -EIO;
        }
    }
    
    // LOOK FOR NEXT ELEMENT
    ++ds->index ;

	LEVEL_DEBUG("Index %d\n",ds->index);

	ret = DirblobGet (ds->index, ds->sn, db);
	LEVEL_DEBUG("DirblobGet %d\n",ret);
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

	LEVEL_DEBUG("LINK_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return ret;
}

/* Assymetric */
/* Read from LINK with timeout on each character */
// NOTE: from PDkit, reead 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK
// returns 0=good 1=bad
/* return 0=good,
          -errno = read error
          -EINTR = timeout
 */
static int LINK_read(BYTE * buf, const size_t size,
						 const struct parsedname *pn)
{
    int error_return = 0 ;
    size_t bytes_left = size;

    if (pn->selected_connection == NULL) {
        STAT_ADD1(DS2480_read_null);
        return -EIO ;
    }
    //printf("LINK read attempting %d bytes on %d time %ld\n",(int)size,pn->selected_connection->file_descriptor,(long int)Global.timeout_serial);
    while (bytes_left > 0) {
        int select_return = 0 ;
        fd_set fdset;
        struct timeval tval;
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
		 */

		// if byte available read or return bytes read
        select_return = select(pn->selected_connection->file_descriptor + 1, &fdset, NULL, NULL, &tval);
        //printf("Link Read select = %d\n",select_return) ;
        if (select_return > 0) {
            ssize_t read_return ;
			if (FD_ISSET(pn->selected_connection->file_descriptor, &fdset) == 0) {
                error_return = -EIO;		/* error */
				STAT_ADD1(DS2480_read_fd_isset);
				break;
			}
//            update_max_delay(pn);
            read_return = read(pn->selected_connection->file_descriptor, &buf[size - bytes_left], bytes_left);
            Debug_Bytes( "LINK read",&buf[size - bytes_left], read_return ) ;
            if (read_return < 0) {
				if (errno == EINTR) {
					/* read() was interrupted, try again */
					STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
					continue;
				}
				ERROR_CONNECT("LINK read error: %s\n",
							  SAFESTRING(pn->selected_connection->name));
                error_return = -errno;	/* error */
				STAT_ADD1(DS2480_read_read);
				break;
			}
            bytes_left -= read_return;
        } else if (select_return < 0) {
			if (errno == EINTR) {
				/* select() was interrupted, try again */
				STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
				continue;
			}
			ERROR_CONNECT("LINK select error: %s\n",
						  SAFESTRING(pn->selected_connection->name));
			STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
			return -EINTR;
		} else {
			ERROR_CONNECT("LINK timeout error: %s\n",
						  SAFESTRING(pn->selected_connection->name));
			STAT_ADD1_BUS(e_bus_timeouts, pn->selected_connection);
			return -EINTR;
		}
	}
	if (bytes_left > 0) {			/* signal that an error was encountered */
		ERROR_CONNECT("LINK read short error: %s\n",
					  SAFESTRING(pn->selected_connection->name));
		STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
        return error_return;				/* error */
	}
	//printf("Link_Read_Low <%*s>\n",(int)size,buf) ;
	return 0;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
   Special processing for the remote hub (add 0x0A)
 */
static int LINK_write(const BYTE * buf, const size_t size,
					  const struct parsedname *pn)
{
    ssize_t left_to_write = size ;

    Debug_Bytes( "LINK write", buf, size) ;
//    COM_flush(pn) ;
    //printf("Link write attempting %d bytes on %d\n",(int)size,pn->selected_connection->file_descriptor) ;
    while ( left_to_write > 0 ) {
        ssize_t write_or_error = write(pn->selected_connection->file_descriptor, buf, left_to_write);
        Debug_Bytes("Link write",buf,left_to_write);
        //printf("Link write = %d\n",(int)write_or_error);
        if (write_or_error < 0) {
            ERROR_CONNECT("Trouble writing data to LINK: %s\n",
                        SAFESTRING(pn->selected_connection->name));
            STAT_ADD1_BUS(e_bus_write_errors, pn->selected_connection);
            return write_or_error ;
        }
	LEVEL_DEBUG("ltow %d woe %d\n",left_to_write,write_or_error);
        left_to_write -= write_or_error ;
    }
    tcdrain(pn->selected_connection->file_descriptor);
    gettimeofday(&(pn->selected_connection->bus_write_time), NULL);
	
	return 0;
}

static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn)
{

	if (LINK_write(LINK_string("p"), 1, pn)
		|| LINK_byte_bounce(&data, resp, pn)) {
		return -EIO;			// send just the <CR>
	}
	// delay
	UT_delay(delay);

	// flush the buffers
	return LINK_CR(pn);			// send just the <CR>
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t size,
							  const struct parsedname *pn)
{
	size_t i;
	size_t left;
	BYTE *buf = pn->selected_connection->combuffer;

	if (size == 0)
		return 0;
	if (LINK_write(LINK_string("b"), 1, pn))
	{
		LEVEL_DEBUG ("LINK_sendback_data error sending b\n");
		return -EIO;
	}
//    for ( i=0; ret==0 && i<size ; ++i ) ret = LINK_byte_bounce( &data[i], &resp[i], pn ) ;
	for (left = size; left;) {
		i = (left > 16) ? 16 : left;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) buf, &data[size - left], i);
		if (LINK_write(buf, i << 1, pn) || LINK_read(buf, i << 1, pn))
			return -EIO;
		string2bytes((char *) buf, &resp[size - left], i);
		left -= i;
	}
	return LINK_CR(pn);
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) printf( "%.2X ",b[i] ) ;
    if ( size ) printf("\n") ;
}
*/

static int LINK_byte_bounce(const BYTE * out, BYTE * in,
							const struct parsedname *pn)
{
	BYTE data[2];

	num2string((char *) data, out[0]);
	if (LINK_write(data, 2, pn) || LINK_read(data, 2, pn))
		return -EIO;
	in[0] = string2num((char *) data);
	return 0;
}

static int LINK_CR(const struct parsedname *pn)
{
	BYTE data[3];
	if (LINK_write(LINK_string("\r"), 1, pn) || LINK_read(data, 2, pn))
		return -EIO;
	LEVEL_DEBUG("LINK_CR return 0\n");
	return 0;
}

/********************************************************/
/* LINK_close  ** clean local resources before      	*/
/*                closing the serial port           	*/
/*							*/
/********************************************************/

static void LINK_close (struct connection_in *in)
{
   DirblobClear (&(in->connin.link.main));
   DirblobClear (&(in->connin.link.alarm));
   COM_close(in);

}

/************************************************************************/
/*									                                     */
/*	LINK_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/*									                                     */	
/************************************************************************/
static int LINK_directory(struct device_search *ds, struct dirblob *db, 
			  const struct parsedname *pn)
{
	char resp[21];
	BYTE sn[8];
	int ret;

    DirblobClear(db);

    //Depending on the search type, the LINK search function
	//needs to be selected
	//tEC -- Conditional searching
	//tF0 -- Normal searching
    
    // Send the configuration command and check response
    if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		if ((ret = LINK_write(LINK_string("tEC"),3,pn)))
			return ret;
		if ((ret = (LINK_read(LINK_string(resp),5,pn))))
		   	return ret;
		if (strncmp(resp,",EC",3)!=0){
			LEVEL_DEBUG("Did not change to conditional search");
			return -EIO;
			}
		LEVEL_DEBUG("LINK set for conditional search\n");
	}
	else	{
		if ((ret = LINK_write(LINK_string("tF0"),3,pn)))
			return ret;
		if ((ret = (LINK_read(LINK_string(resp),5,pn))))
		   	return ret;
		if (strncmp(resp,",F0",3)!=0){
			LEVEL_DEBUG("Did not change to normal search");
			return -EIO;
			}
		LEVEL_DEBUG("LINK set for normal search\n");
	}

    if ((ret = LINK_write(LINK_string("f"), 1, pn))) {
        return ret;
	}

	//One needs to check the first character returned.
	//If nothing is found, the link will timeout rather then have a quick 
	//return.  This happens when looking at the alarm directory and
	//there are no alarms pending
	//So we grab the first character and check it.  If not an E leave it
	//in the resp buffer and get the rest of the response from the LINK
	//device

	if ((ret = LINK_read(LINK_string(resp),1,pn)))
		return ret;

	if (resp[0]=='E'){
		if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		 	LEVEL_DEBUG ("LINK returned E: No devices in alarm\n");
			return 0;
		}
	}
    
	if ((ret = LINK_read(LINK_string((resp+1)), 19, pn))) 
		return ret;

    // Check if we should start scanning
    switch (resp[0]) {
        case '-':
        case '+':
            if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
                pn->selected_connection->AnyDevices = 1;
            }
            break;
        case 'N':
            LEVEL_DEBUG ("LINK returned N: Empty bus\n");
            if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
                pn->selected_connection->AnyDevices = 0;
            }
        case 'X':
        default:
            LEVEL_DEBUG("LINK_search default case\n");
            return -EIO;
    }

    /* Join the loop after the first query -- subsequent handled differently */
    while ( (resp[0]=='+') || (resp[0]=='-') ) {
        sn[7] = string2num(&resp[2]);
        sn[6] = string2num(&resp[4]);
        sn[5] = string2num(&resp[6]);
        sn[4] = string2num(&resp[8]);
        sn[3] = string2num(&resp[10]);
        sn[2] = string2num(&resp[12]);
        sn[1] = string2num(&resp[14]);
        sn[0] = string2num(&resp[16]);
        LEVEL_DEBUG("LINK_directory SN found: " SNformat "\n", SNvar(sn));

	// CRC check
        if (CRC8(sn, 8) || (sn[0] == 0)) {
            /* A minor "error" and should perhaps only return -1 */
            /* to avoid reconnect */
            LEVEL_DEBUG("sn = %s\n",sn);
            return -EIO;
        }

    	DirblobAdd (sn, db);
    
        switch (resp[0]) {
            case '+':
                // get next element
                if ((ret = LINK_write(LINK_string("n"), 1, pn))) {
                    return ret;
                }
                if ((ret = LINK_read(LINK_string((resp)), 20, pn))) {
                    return ret;
                }
                break;
            case '-':
                return 0 ;
            default:
                break ;
        }
    }
	return 0 ;
}
