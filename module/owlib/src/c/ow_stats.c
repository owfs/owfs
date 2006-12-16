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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
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

/* Stats are a pseudo-device -- they are a file-system entry and handled as such,
     but have a different caching type to distiguish their handling */

#include <config.h>
#include "owfs_config.h"
#include "ow_stats.h"
#include "ow_counters.h"

/* ----------------- */
/* ---- Globals ---- */
/* ----------------- */
UINT cache_flips = 0;
UINT cache_adds = 0;
struct average old_avg = { 0L, 0L, 0L, 0L, };
struct average new_avg = { 0L, 0L, 0L, 0L, };
struct average store_avg = { 0L, 0L, 0L, 0L, };
struct cache cache_ext = { 0L, 0L, 0L, 0L, 0L, };
struct cache cache_int = { 0L, 0L, 0L, 0L, 0L, };
struct cache cache_dir = { 0L, 0L, 0L, 0L, 0L, };
struct cache cache_sto = { 0L, 0L, 0L, 0L, 0L, };
struct cache cache_dev = { 0L, 0L, 0L, 0L, 0L, };

UINT read_calls = 0;
UINT read_cache = 0;
UINT read_bytes = 0;
UINT read_cachebytes = 0;
UINT read_array = 0;
UINT read_tries[3] = { 0, 0, 0, };
UINT read_success = 0;
struct average read_avg = { 0L, 0L, 0L, 0L, };

UINT write_calls = 0;
UINT write_bytes = 0;
UINT write_array = 0;
UINT write_tries[3] = { 0, 0, 0, };
UINT write_success = 0;
struct average write_avg = { 0L, 0L, 0L, 0L, };

struct directory dir_main = { 0L, 0L, };
struct directory dir_dev = { 0L, 0L, };
UINT dir_depth = 0;
struct average dir_avg = { 0L, 0L, 0L, 0L, };

/* max delay between a write and when reading first char */
struct timeval max_delay = { 0, 0, };

// ow_locks.c
//struct timeval bus_pause = {0, 0, } ;
struct timeval total_bus_time = { 0, 0, };
UINT total_bus_locks = 0;
UINT total_bus_unlocks = 0;

// ow_crc.c
UINT CRC8_tries = 0;
UINT CRC8_errors = 0;
UINT CRC16_tries = 0;
UINT CRC16_errors = 0;

// ow_net.c
UINT NET_accept_errors = 0;
UINT NET_connection_errors = 0;
UINT NET_read_errors = 0;

// ow_bus.c
UINT BUS_reconnects = 0;	// sum from all adapters
UINT BUS_reconnect_errors = 0;	// sum from all adapters
UINT BUS_send_data_errors = 0;
UINT BUS_send_data_memcmp_errors = 0;
UINT BUS_readin_data_errors = 0;
UINT BUS_select_low_errors = 0;
UINT BUS_select_low_branch_errors = 0;
UINT BUS_detect_errors = 0;
UINT BUS_open_errors = 0;
UINT BUS_PowerByte_errors = 0;
UINT BUS_level_errors = 0;
UINT BUS_write_errors = 0;
UINT BUS_write_interrupt_errors = 0;
UINT BUS_read_errors = 0;
UINT BUS_read_interrupt_errors = 0;
UINT BUS_read_select_errors = 0;
UINT BUS_read_timeout_errors = 0;
UINT BUS_next_errors = 0;
UINT BUS_next_alarm_errors = 0;
UINT BUS_reset_errors = 0;
UINT BUS_short_errors = 0;
UINT BUS_bit_errors = 0;
UINT BUS_byte_errors = 0;
UINT BUS_echo_errors = 0;
UINT BUS_ProgramPulse_errors = 0;
UINT BUS_Overdrive_errors = 0;
UINT BUS_TestOverdrive_errors = 0;
UINT BUS_tcsetattr_errors = 0;
UINT BUS_status_errors = 0;

// ow_ds9097U.c
UINT DS2480_send_cmd_errors = 0;
UINT DS2480_send_cmd_memcmp_errors = 0;
UINT DS2480_sendout_data_errors = 0;
UINT DS2480_sendout_cmd_errors = 0;
UINT DS2480_sendback_cmd_errors = 0;
UINT DS2480_write_interrupted = 0;
UINT DS2480_read_fd_isset = 0;
UINT DS2480_read_null = 0;
UINT DS2480_read_read = 0;
UINT DS2480_level_docheck_errors = 0;


struct average all_avg = { 0L, 0L, 0L, 0L, };

/* ------- Prototypes ----------- */
/* Statistics reporting */
uREAD_FUNCTION(FS_stat);
uREAD_FUNCTION(FS_stat_p);
fREAD_FUNCTION(FS_time);
fREAD_FUNCTION(FS_time_p);
uREAD_FUNCTION(FS_elapsed);

/* -------- Structures ---------- */
struct filetype stats_cache[] = {
  {"flips", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_flips},},
  {"additions", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_adds},},
  {"primary", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"primary/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&new_avg.
	    current},},
  {"primary/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&new_avg.
	    sum},},
  {"primary/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&new_avg.
	    count},},
  {"primary/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&new_avg.
	    max},},
  {"secondary", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"secondary/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&old_avg.
	    current},},
  {"secondary/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&old_avg.
	    sum},},
  {"secondary/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&old_avg.
	    count},},
  {"secondary/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&old_avg.
	    max},},
  {"persistent", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"persistent/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&store_avg.
	    current,}},
  {"persistent/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&store_avg.
	    sum},},
  {"persistent/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&store_avg.
	    count},},
  {"persistent/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&store_avg.
	    max},},
  {"external", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"external/tries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_ext.
	    tries},},
  {"external/hits", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_ext.
	    hits},},
  {"external/added", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_ext.
	    adds,}},
  {"external/expired", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_ext.
	    expires,}},
  {"external/deleted", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_ext.
	    deletes,}},
  {"internal", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"internal/tries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_int.
	    tries},},
  {"internal/hits", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_int.
	    hits},},
  {"internal/added", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_int.
	    adds,}},
  {"internal/expired", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_int.
	    expires,}},
  {"internal/deleted", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_int.
	    deletes,}},
  {"directory", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"directory/tries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dir.
	    tries},},
  {"directory/hits", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dir.
	    hits},},
  {"directory/added", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dir.
	    adds},},
  {"directory/expired", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dir.
	    expires,}},
  {"directory/deleted", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dir.
	    deletes,}},
  {"device", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"device/tries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dev.
	    tries},},
  {"device/hits", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dev.
	    hits},},
  {"device/added", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dev.
	    adds},},
  {"device/expired", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dev.
	    expires,}},
  {"device/deleted", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&cache_dev.
	    deletes,}},
};

struct device d_stats_cache =
    { "cache", "cache", 0, NFT(stats_cache), stats_cache };
	// Note, the store hit rate and deletions are not shown -- too much information!

struct aggregate Aread = { 3, ag_numbers, ag_separate, };
struct filetype stats_read[] = {
  {"calls", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_calls},},
  {"cachesuccess", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_cache},},
  {"cachebytes", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_cachebytes},},
  {"success", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_success},},
  {"bytes", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_bytes},},
  {"tries", 15, &Aread, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_tries},},
}

;
struct device d_stats_read =
    { "read", "read", 0, NFT(stats_read), stats_read };

struct filetype stats_write[] = {
  {"calls", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_calls},},
  {"success", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_success},},
  {"bytes", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_bytes},},
  {"tries", 15, &Aread, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_tries},},
}

;
struct device d_stats_write =
    { "write", "write", 0, NFT(stats_write), stats_write };

struct filetype stats_directory[] = {
  {"maxdepth", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_depth},},
  {"bus", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"bus/calls", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_main.
	    calls},},
  {"bus/entries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_main.
	    entries},},
  {"device", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"device/calls", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_dev.
	    calls},},
  {"device/entries", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_dev.
	    entries},},
}

;
struct device d_stats_directory =
    { "directory", "directory", 0, NFT(stats_directory), stats_directory };

struct filetype stats_thread[] = {
  {"directory", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"directory/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_avg.
	    current},},
  {"directory/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_avg.
	    sum},},
  {"directory/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_avg.
	    count},},
  {"directory/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&dir_avg.
	    max},},
  {"overall", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"overall/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&all_avg.
	    current},},
  {"overall/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&all_avg.
	    sum},},
  {"overall/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&all_avg.
	    count},},
  {"overall/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&all_avg.
	    max},},
  {"read", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"read/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_avg.
	    current},},
  {"read/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_avg.
	    sum},},
  {"read/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_avg.
	    count},},
  {"read/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&read_avg.
	    max},},
  {"write", 0, NULL, ft_subdir, fc_statistic, {v: NULL}, {v: NULL}, {v:NULL},},
  {"write/now", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_avg.
	    current,}},
  {"write/sum", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_avg.
	    sum},},
  {"write/num", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_avg.
	    count},},
  {"write/max", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&write_avg.
	    max},},
}

;
struct device d_stats_thread =
    { "threads", "threads", 0, NFT(stats_thread), stats_thread };

struct filetype stats_bus[] = {
  {"elapsed_time", 15, NULL, ft_unsigned, fc_statistic, {u: FS_elapsed}, {v: NULL}, {v:NULL},},
  {"bus_time", 12, &Asystem, ft_float, fc_statistic, {f: FS_time_p}, {v: NULL}, {i:0},},
  {"bus_locks", 15, &Asystem, ft_unsigned, fc_statistic, {u: FS_stat_p}, {v: NULL}, {i:0},},
  {"bus_unlocks", 15, &Asystem, ft_unsigned, fc_statistic, {u: FS_stat_p}, {v: NULL}, {i:1},},
  {"reconnect", 12, &Asystem, ft_unsigned, fc_statistic, {u: FS_stat_p}, {v: NULL}, {i:2},},
  {"reconnect_errors", 12, &Asystem, ft_unsigned, fc_statistic, {u: FS_stat_p}, {v: NULL}, {i:3},},
  {"other_bus_errors", 12, &Asystem, ft_unsigned, fc_statistic, {u: FS_stat_p}, {v: NULL}, {i:4},},
  {"total_bus_time", 12, NULL, ft_float, fc_statistic, {f: FS_time}, {v: NULL}, {v:&total_bus_time},},
  {"total_bus_unlocks", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&total_bus_unlocks,}},
  {"total_bus_locks", 15, NULL, ft_unsigned, fc_statistic, {u: FS_stat}, {v: NULL}, {v:&total_bus_locks},},
};
struct device d_stats_bus = { "bus", "bus", 0, NFT(stats_bus), stats_bus };

#define FS_stat_ROW(var) {"" #var "",      15, NULL  , ft_unsigned, fc_statistic, {u:FS_stat}, {v:NULL}, {v: & var,} }

struct filetype stats_errors[] = {
  {"max_delay", 12, NULL, ft_float, fc_statistic, {f: FS_time}, {v: NULL}, {v:&max_delay},},

// ow_net.c
    FS_stat_ROW(NET_accept_errors),
    FS_stat_ROW(NET_read_errors),
    FS_stat_ROW(NET_connection_errors),

// ow_bus.c
    FS_stat_ROW(BUS_reconnects),
    FS_stat_ROW(BUS_reconnect_errors),
    FS_stat_ROW(BUS_send_data_errors),
    FS_stat_ROW(BUS_send_data_memcmp_errors),
    FS_stat_ROW(BUS_readin_data_errors),
    FS_stat_ROW(BUS_select_low_errors),
    FS_stat_ROW(BUS_select_low_branch_errors),
    FS_stat_ROW(BUS_open_errors),
    FS_stat_ROW(BUS_detect_errors),
    FS_stat_ROW(BUS_PowerByte_errors),
    FS_stat_ROW(BUS_level_errors),
    FS_stat_ROW(BUS_write_errors),
    FS_stat_ROW(BUS_write_interrupt_errors),
    FS_stat_ROW(BUS_read_errors),
    FS_stat_ROW(BUS_read_interrupt_errors),
    FS_stat_ROW(BUS_read_select_errors),
    FS_stat_ROW(BUS_read_timeout_errors),
    FS_stat_ROW(BUS_next_errors),
    FS_stat_ROW(BUS_next_alarm_errors),
    FS_stat_ROW(BUS_reset_errors),
    FS_stat_ROW(BUS_short_errors),
    FS_stat_ROW(BUS_bit_errors),
    FS_stat_ROW(BUS_byte_errors),
    FS_stat_ROW(BUS_echo_errors),
    FS_stat_ROW(BUS_ProgramPulse_errors),
    FS_stat_ROW(BUS_Overdrive_errors),
    FS_stat_ROW(BUS_TestOverdrive_errors),
    FS_stat_ROW(BUS_tcsetattr_errors),
    FS_stat_ROW(BUS_status_errors),

// ow_ds9097U.c
    FS_stat_ROW(DS2480_send_cmd_errors),
    FS_stat_ROW(DS2480_send_cmd_memcmp_errors),
    FS_stat_ROW(DS2480_sendout_data_errors),
    FS_stat_ROW(DS2480_sendout_cmd_errors),
    FS_stat_ROW(DS2480_sendback_cmd_errors),
    FS_stat_ROW(DS2480_write_interrupted),
    FS_stat_ROW(DS2480_read_fd_isset),
    FS_stat_ROW(DS2480_read_null),
    FS_stat_ROW(DS2480_read_read),
    FS_stat_ROW(DS2480_level_docheck_errors),

    FS_stat_ROW(CRC8_errors),
    FS_stat_ROW(CRC8_tries),
    FS_stat_ROW(CRC16_errors),
    FS_stat_ROW(CRC16_tries),

}

;

struct device d_stats_errors =
    { "errors", "errors", 0, NFT(stats_errors), stats_errors };


/* ------- Functions ------------ */

static int FS_stat(UINT * u, const struct parsedname *pn)
{
    int dindex = pn->extension;
    if (dindex < 0)
	dindex = 0;
    if (pn->ft == NULL)
	return -ENOENT;
    if (pn->ft->data.v == NULL)
	return -ENOENT;
    STATLOCK;
    u[0] = ((UINT *) pn->ft->data.v)[dindex];
    STATUNLOCK;
    return 0;
}

static int FS_stat_p(UINT * u, const struct parsedname *pn)
{
    int dindex = pn->extension;
    UINT *ptr;
    struct connection_in *c;
    if (dindex < 0)
	dindex = 0;
    c = find_connection_in(dindex);
    if (!c)
	return -ENOENT;

    if (pn->ft == NULL)
	return -ENOENT;
    switch (pn->ft->data.i) {
    case 0:
	ptr = &c->bus_locks;
	break;
    case 1:
	ptr = &c->bus_unlocks;
	break;
    case 2:
	ptr = &c->bus_reconnect;
	break;
    case 3:
	ptr = &c->bus_reconnect_errors;
	break;
    case 4:
	ptr = &c->bus_errors;
	break;
    default:
	return -ENOENT;
    }
    STATLOCK;
    u[0] = *ptr;
    STATUNLOCK;
    return 0;
}

static int FS_time_p(_FLOAT * u, const struct parsedname *pn)
{
    _FLOAT f;
    int dindex = pn->extension;
    struct timeval *tv;
    struct connection_in *c;
    if (dindex < 0)
	dindex = 0;
    c = find_connection_in(dindex);
    if (!c)
	return -ENOENT;

    if (pn->ft == NULL)
	return -ENOENT;
    switch (pn->ft->data.i) {
    case 0:
	tv = &c->bus_time;
	break;
    default:
	return -ENOENT;
    }
    /* to prevent simultaneous changes to bus timing variables */
    STATLOCK;
    f = (_FLOAT) tv->tv_sec + ((_FLOAT) (tv->tv_usec / 1000)) / 1000.0;
    STATUNLOCK;
//printf("FS_time sec=%ld usec=%ld f=%7.3f\n",tv[dindex].tv_sec,tv[dindex].tv_usec, f) ;
    u[0] = f;
    return 0;
}

static int FS_time(_FLOAT * u, const struct parsedname *pn)
{
    _FLOAT f;
    int dindex = pn->extension;
    struct timeval *tv;
    if (dindex < 0)
	dindex = 0;
    if (pn->ft == NULL)
	return -ENOENT;
    tv = (struct timeval *) pn->ft->data.v;
    if (tv == NULL)
	return -ENOENT;

    /* to prevent simultaneous changes to bus timing variables */
    STATLOCK;
    f = (_FLOAT) tv[dindex].tv_sec +
	((_FLOAT) (tv[dindex].tv_usec / 1000)) / 1000.0;
    STATUNLOCK;
//printf("FS_time sec=%ld usec=%ld f=%7.3f\n",tv[dindex].tv_sec,tv[dindex].tv_usec, f) ;
    u[0] = f;
    return 0;
}

static int FS_elapsed(UINT * u, const struct parsedname *pn)
{
//printf("ELAPSE start=%u, now=%u, diff=%u\n",start_time,time(NULL),time(NULL)-start_time) ;
    (void) pn;
    STATLOCK;
    u[0] = time(NULL) - start_time;
    STATUNLOCK;
    return 0;
}
