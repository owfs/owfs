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

#if OW_HA7

static struct timeval tvnet = { 0, 200000, };

struct toHA7 {
	ASCII *command;
	ASCII lock[10];
	ASCII conditional[1];
	ASCII address[16];
	ASCII *data;
	size_t length;
};

//static void byteprint( const BYTE * b, int size ) ;
static int HA7_write(int file_descriptor, const ASCII * msg, size_t size,
					 struct connection_in *in);
static void toHA7init(struct toHA7 *ha7);
static void setHA7address(struct toHA7 *ha7, BYTE * sn);
static int HA7_toHA7(int file_descriptor, const struct toHA7 *ha7,
					 struct connection_in *in);
static int HA7_getlock(int file_descriptor, struct connection_in *in);
static int HA7_releaselock(int file_descriptor, struct connection_in *in);
static int HA7_read(int file_descriptor, ASCII ** buffer);
static int HA7_reset(const struct parsedname *pn);
static int HA7_next_both(struct device_search *ds,
						 const struct parsedname *pn);
static int HA7_sendback_data(const BYTE * data, BYTE * resp,
							 const size_t len,
							 const struct parsedname *pn);
static int HA7_select(const struct parsedname *pn);
static void HA7_setroutines(struct interface_routines *f);
static void HA7_close(struct connection_in *in);
static int HA7_directory(BYTE search, struct dirblob *db,
						 const struct parsedname *pn);

static void HA7_setroutines(struct interface_routines *f)
{
	f->detect = HA7_detect;
	f->reset = HA7_reset;
	f->next_both = HA7_next_both;
//    f->overdrive = ;
//    f->testoverdrive = ;
	f->PowerByte = NULL;
//    f->ProgramPulse = ;
	f->sendback_data = HA7_sendback_data;
//    f->sendback_bits = ;
	f->select = HA7_select;
	f->reconnect = NULL;
	f->close = HA7_close;
	f->transaction = NULL;
	f->flags =
		ADAP_FLAG_overdrive | ADAP_FLAG_dirgulp | ADAP_FLAG_2409path;
}

int HA7_detect(struct connection_in *in)
{
	struct parsedname pn;
	int file_descriptor;
	struct toHA7 ha7;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;
	LEVEL_CONNECT("HA7 detect\n");
	/* Set up low-level routines */
	HA7_setroutines(&(in->iroutines));
	in->connin.ha7.locked = 0;
	/* Initialize dir-at-once structures */
	DirblobInit(&(in->connin.ha7.main));
	DirblobInit(&(in->connin.ha7.alarm));

	if (in->name == NULL)
		return -1;
	/* Add the port if it isn't there already */
	if (strchr(in->name, ':') == NULL) {
		ASCII *temp = realloc(in->name, strlen(in->name) + 3);
		if (temp == NULL)
			return -ENOMEM;
		in->name = temp;
		strcat(in->name, ":80");
	}
	if (ClientAddr(in->name, in))
		return -1;
	if ((file_descriptor = ClientConnect(in)) < 0)
		return -EIO;
	in->Adapter = adapter_HA7NET;

	toHA7init(&ha7);
	ha7.command = "ReleaseLock";
	if (HA7_toHA7(file_descriptor, &ha7, in) == 0) {
		ASCII *buf;
		if (HA7_read(file_descriptor, &buf) == 0) {
			in->adapter_name = "HA7Net";
			in->busmode = bus_ha7net;
			in->AnyDevices = 1;
			free(buf);
			close(file_descriptor);
			return 0;
		}
	}
	close(file_descriptor);
	return -EIO;
}

static int HA7_reset(const struct parsedname *pn)
{
	ASCII *resp = NULL;
	int file_descriptor = ClientConnect(pn->selected_connection);
	int ret = BUS_RESET_OK;
	struct toHA7 ha7;

	if (file_descriptor < 0) {
		STAT_ADD1_BUS(BUS_reset_errors, pn->selected_connection);
		return -EIO;
	}

	toHA7init(&ha7);
	ha7.command = "Reset";
	if (HA7_toHA7(file_descriptor, &ha7, pn->selected_connection)) {
		STAT_ADD1_BUS(BUS_reset_errors, pn->selected_connection);
		ret = -EIO;
	} else if (HA7_read(file_descriptor, &resp)) {
		STAT_ADD1_BUS(BUS_reset_errors, pn->selected_connection);
		ret = -EIO;
	}
	if (resp)
		free(resp);
	close(file_descriptor);
	return ret;
}

static int HA7_directory(BYTE search, struct dirblob *db,
						 const struct parsedname *pn)
{
	int file_descriptor;
	int ret = 0;
	struct toHA7 ha7;
	ASCII *resp = NULL;

	DirblobClear(db);
	if ((file_descriptor = ClientConnect(pn->selected_connection)) < 0) {
		db->troubled = 1;
		return -EIO;
	}

	toHA7init(&ha7);
	ha7.command = "Search";
    if (search == _1W_CONDITIONAL_SEARCH_ROM)
		ha7.conditional[0] = '1';
	if (HA7_toHA7(file_descriptor, &ha7, pn->selected_connection)) {
		ret = -EIO;
	} else if (HA7_read(file_descriptor, &resp)) {
		ret = -EIO;
	} else {
		BYTE sn[8];
		ASCII *p = resp;
		while ((p = strstr(p, "<INPUT CLASS=\"HA7Value\" NAME=\"Address_"))
			   && (p = strstr(p, "VALUE=\""))) {
			p += 7;
			if (strspn(p, "0123456789ABCDEF") < 16) {
				ret = -EIO;
				break;
			}
			sn[7] = string2num(&p[0]);
			sn[6] = string2num(&p[2]);
			sn[5] = string2num(&p[4]);
			sn[4] = string2num(&p[6]);
			sn[3] = string2num(&p[8]);
			sn[2] = string2num(&p[10]);
			sn[1] = string2num(&p[12]);
			sn[0] = string2num(&p[14]);
			if (CRC8(sn, 8)) {
				ret = -EIO;
				break;
			}
			DirblobAdd(sn, db);
		}
		free(resp);
	}
	close(file_descriptor);
	return ret;
}

static int HA7_next_both(struct device_search *ds,
						 const struct parsedname *pn)
{
    struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->connin.ha7.alarm) : &(pn->selected_connection->connin.ha7.main);
	int ret = 0;

	printf("NextBoth %s\n", pn->path);
	if (!pn->selected_connection->AnyDevices)
		ds->LastDevice = 1;
	if (ds->LastDevice)
		return -ENODEV;

	if (++(ds->LastDiscrepancy) == 0) {
		if (HA7_directory(ds->search, db, pn))
			return -EIO;
	}
	ret = DirblobGet(ds->LastDiscrepancy, ds->sn, db);
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
	return ret;
}

static int HA7_read(int file_descriptor, ASCII ** buffer)
{
	ASCII buf[4097];
	ASCII *start;
	int ret = 0;
	ssize_t r, s;
	struct timeval tvnetfirst = { Global.timeout_network, 0, };

	*buffer = NULL;
	buf[4096] = '\0';			// just in case
	if ((r = tcp_read(file_descriptor, buf, 4096, &tvnetfirst)) < 0) {
		LEVEL_CONNECT("HA7_read (ethernet) error = %d\n", r);
		write(1, buf, r);
		ret = -EIO;
	} else if (strncmp("HTTP/1.1 200 OK", buf, 15)) {	//Bad HTTP return code
		ASCII *p = strchr(&buf[15], '\n');
		if (p == NULL)
			p = &buf[15 + 32];
		LEVEL_DATA("HA7 response problem:%.*s\n", p - buf - 15, &buf[15]);
		ret = -EIO;
	} else if ((start = strstr(buf, "<body>")) == NULL) {
		LEVEL_DATA("HA7 response no HTTP body to parse\n");
		ret = -EIO;
	} else {
		// HTML body found, dump header
		s = buf + r - start;
		//write( 1, start, s) ;
		if ((*buffer = malloc(s)) == NULL) {
			ret = -ENOMEM;
		} else {
			memcpy(*buffer, start, s);
			while (r == 4096) {
				if ((r = tcp_read(file_descriptor, buf, 4096, &tvnet)) < 0) {
					LEVEL_DATA("Couldn't get rest of HA7 data (err=%d)\n",
							   r);
					ret = -EIO;
					break;
				} else {
					ASCII *temp = realloc(*buffer, s + r);
					if (temp) {
						*buffer = temp;
						memcpy(&((*buffer)[s]), buf, r);
						s += r;
					} else {
						ret = -ENOMEM;
						break;
					}
				}
			}
		}
	}
	if (ret) {
		if (*buffer)
			free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

static int HA7_write(int file_descriptor, const ASCII * msg, size_t length,
					 struct connection_in *in)
{
	ssize_t r, sl = length;
	ssize_t size = sl;
	while (sl > 0) {
		r = write(file_descriptor, &msg[size - sl], sl);
		if (r < 0) {
			if (errno == EINTR) {
				STAT_ADD1_BUS(BUS_write_interrupt_errors, in);
				continue;
			}
			ERROR_CONNECT("Trouble writing data to HA7: %s\n",
						  SAFESTRING(in->name));
			break;
		}
		sl -= r;
	}
	gettimeofday(&(in->bus_write_time), NULL);
	if (sl > 0) {
		STAT_ADD1_BUS(BUS_write_errors, in);
		return -EIO;
	}
	return 0;
}

static int HA7_toHA7(int file_descriptor, const struct toHA7 *ha7,
					 struct connection_in *in)
{
	int first = 1;

	LEVEL_DEBUG
		("To HA7 command=%s address=%.16s data(%d)=%.*s conditional=%.1s lock=%.10s\n",
		 SAFESTRING(ha7->command), SAFESTRING(ha7->address), ha7->length,
		 ha7->length, SAFESTRING(ha7->data), SAFESTRING(ha7->conditional),
		 SAFESTRING(ha7->lock));
	if (ha7->command == NULL)
		return -EINVAL;

	if (HA7_write(file_descriptor, "GET /1Wire/", 11, in))
		return -EIO;

	if (HA7_write(file_descriptor, ha7->command, strlen(ha7->command), in))
		return -EIO;
	if (HA7_write(file_descriptor, ".html", 5, in))
		return -EIO;

	if (ha7->address[0]) {
		if (HA7_write(file_descriptor, first ? "?" : "&", 1, in))
			return -EIO;
		first = 0;
		if (HA7_write(file_descriptor, "Address=", 8, in))
			return -EIO;
		if (HA7_write(file_descriptor, ha7->address, 16, in))
			return -EIO;
	}

	if (ha7->conditional[0]) {
		if (HA7_write(file_descriptor, first ? "?" : "&", 1, in))
			return -EIO;
		first = 0;
		if (HA7_write(file_descriptor, "Conditional=", 12, in))
			return -EIO;
		if (HA7_write(file_descriptor, ha7->conditional, 1, in))
			return -EIO;
	}

	if (ha7->data) {
		if (HA7_write(file_descriptor, first ? "?" : "&", 1, in))
			return -EIO;
		first = 0;
		if (HA7_write(file_descriptor, "Data=", 5, in))
			return -EIO;
		if (HA7_write(file_descriptor, ha7->data, ha7->length, in))
			return -EIO;
	}

	if (ha7->lock[0]) {
		if (HA7_write(file_descriptor, first ? "?" : "&", 1, in))
			return -EIO;
		first = 0;
		if (HA7_write(file_descriptor, "LockID=", 7, in))
			return -EIO;
		if (HA7_write(file_descriptor, ha7->lock, 10, in))
			return -EIO;
	}

	return HA7_write(file_descriptor, " HTTP/1.0\n\n", 11, in);
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int HA7_sendback_data(const BYTE * data, BYTE * resp,
							 const size_t size,
							 const struct parsedname *pn)
{
	int file_descriptor;
	ASCII *r;
	struct toHA7 ha7;
	int ret = -EIO;

	if ((MAX_FIFO_SIZE >> 1) < size) {
		size_t half = size >> 1;
		if (HA7_sendback_data(data, resp, half, pn))
			return -EIO;
		return HA7_sendback_data(&data[half], &resp[half], size - half,
								 pn);
	}

	if ((file_descriptor = ClientConnect(pn->selected_connection)) < 0)
		return -EIO;
	bytes2string((ASCII *) pn->selected_connection->combuffer, data, size);

	toHA7init(&ha7);
	ha7.command = "WriteBlock";
	ha7.data = (ASCII *) pn->selected_connection->combuffer;
	ha7.length = 2 * size;
	if (HA7_toHA7(file_descriptor, &ha7, pn->selected_connection) == 0 && HA7_read(file_descriptor, &r) == 0) {
		ASCII *p = r;
		if ((p = strstr(p, "<INPUT TYPE=\"TEXT\" NAME=\"ResultData_0\""))
			&& (p = strstr(p, "VALUE=\""))) {
			p += 7;
			LEVEL_DEBUG("HA7_sendback_data received(%d): %.*s\n", size * 2,
						size * 2, p);
			if (strspn(p, "0123456789ABCDEF") >= size << 1) {
				string2bytes(p, resp, size);
				ret = 0;
			}
		}
		free(r);
	}
	close(file_descriptor);
	return ret;
}

static void setHA7address(struct toHA7 *ha7, BYTE * sn)
{
	num2string(&(ha7->address[0]), sn[7]);
	num2string(&(ha7->address[2]), sn[6]);
	num2string(&(ha7->address[4]), sn[5]);
	num2string(&(ha7->address[6]), sn[4]);
	num2string(&(ha7->address[8]), sn[3]);
	num2string(&(ha7->address[10]), sn[2]);
	num2string(&(ha7->address[12]), sn[1]);
	num2string(&(ha7->address[14]), sn[0]);
}

static int HA7_select(const struct parsedname *pn)
{
	int ret = -EIO;

	if (pn->selected_device) {
		int file_descriptor = ClientConnect(pn->selected_connection);

		if (file_descriptor >= 0) {
			struct toHA7 ha7;
			toHA7init(&ha7);
			ha7.command = "AddressDevice";
			setHA7address(&ha7, pn->sn);
			if (HA7_toHA7(file_descriptor, &ha7, pn->selected_connection) == 0) {
				ASCII *buf;
				if (HA7_read(file_descriptor, &buf) == 0) {
					free(buf);
					ret = 0;
				}
			}
			close(file_descriptor);
		}
	} else {
		return HA7_reset(pn);
	}
	return ret;
}

static void HA7_close(struct connection_in *in)
{
	DirblobClear(&(in->connin.ha7.main));
	DirblobClear(&(in->connin.ha7.alarm));
	FreeClientAddr(in);
}

static int HA7_getlock(int file_descriptor, struct connection_in *in)
{
	(void) file_descriptor;
	(void) in;
}

static int HA7_releaselock(int file_descriptor, struct connection_in *in)
{
	(void) file_descriptor;
	(void) in;
}

static void toHA7init(struct toHA7 *ha7)
{
	ha7->command = ha7->data = NULL;
	ha7->length = 0;
	ha7->conditional[0] = ha7->lock[0] = ha7->address[0] = '\0';
}

#endif							/* OW_HA7 */
