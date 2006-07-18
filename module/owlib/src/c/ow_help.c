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

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

void ow_help( enum opt_program op ) {
    printf(
    "1-wire access programs\n"
    "By Paul H Alfille and others. See http://owfs.sourceforge.net\n"
    "\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf("Syntax: %s [options] device mountpoint\n",progname);
        break ;
    case opt_httpd:
    case opt_server:
        printf("Syntax: %s [options] device clientport\n",progname);
        break ;
    case opt_nfsd:
    case opt_ftpd:
    default:
        printf("Syntax: %s [options] device\n",progname);
        break ;
    }
    printf(
    "\n"
    "1-wire side (device):\n"
    "  -d --device    devicename      |Serial port device name of 1-wire adapter\n"
    "                                 |  e.g: -d /dev/ttyS0\n");
#if OW_USB
    printf("  -u --usb       [number]|all    |USB 1-wire adapter. Choose first one unless number specified\n");
#endif /* OW_USB */
    printf("                                 |  e.g: -u -u3  first and third USB 1wire adapters\n"
    "  -s --server    [host:]port     |owserver program that talks to adapter. Give tcp/ip address.\n"
    "                                 |  e.g: -s embeddedhost:3030\n"
    "\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf(
        "\n"
        "Client side:\n"
        "  mountpoint                     |Directory the 1-wire filesystem is mounted\n"
        ) ;
        break ;
    case opt_httpd:
    case opt_ftpd:
    case opt_nfsd:
    case opt_server:
    printf(
        "\n"
        "Client side:\n"
        "  -p --port                      |tcp/ip address that program can be found\n"
        ) ;
    break ;
    default:
    break ;
}
    printf(
    "\n"
    "  -h --help                      |This basic help page\n"
    "     --morehelp                  |Optional items help page\n"
    ) ;
}

void ow_morehelp( enum opt_program op ) {
    printf(
    "1-wire access programs\n"
    "By Paul H Alfille and others. See http://owfs.sourceforge.net\n"
    "\n"
    ) ;
    printf(
    "  -C --Celsius                   |Celsius(default) temperature scale\n"
    "  -F --Fahrenheit                |Fahrenheit temperature scale\n"
    "  -K --Kelvin                    |Kelvin temperature scale\n"
    "  -R --Rankine                   |Rankine temperature scale\n"
    "  -P --pid_file                  |file where program id (pid) will be stored\n"
    "     --background                |become a deamon process(default)\n"
    "     --foreground                |stay in foreground\n"
    "  -r --readonly                  |no writing to 1-wire bus\n"
    "  -w --write                     |allow reading and writing to bus(default)\n"
    "     --fake                      |with list of emulated device family codes\n"
    "     --link | --nolink           |LINK adapters in ascii|emulation mode (emulation)\n");
#if OW_CACHE
    printf("  -t                             |cache timeout (in seconds)\n");
#endif /* OW_CACHE */    
    printf("  -f --format                    |format for 1-wire unique serial IDs display\n"
    "                                 |  f[.]i[[.]c] f-amily i-d c-rc (all in hex)\n"
    "     --error_print               |Where information/error messages are printed\n"
    "                                 |  0-mixed(default) 1-syslog 2-stderr 3-suppressed\n"
    "     --error_level               |What kind of information is printed\n"
    "                                 |  0-fatal 1-connections 2-calls 3-data 4-detail\n"    
    "     --msec_read                 | millisecond timeout for reads on bus (serial and i2c)\n"
    "  -V --version                   |Program and library versions\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf(
        "     --fuse_opt                  |Options to send to fuse_mount (must be quoted)\n"
        "                                 |  e.g: --fuse_opt=\"-x -f\"\n"
        "     --fuse_open_opt             |Options to send to fuse_open (must be quoted)\n"
        "                                 |  e.g: --fuse_open_opt=\"direct_io\"\n"
        ) ;
        break ;
    case opt_httpd:
    case opt_server:
        printf(
        "\n"
        "Client side:\n"
        "  -p --port                      |tcp/ip address that program can be found\n"
        ) ;
        break ;
    case opt_ftpd:
        printf(
        "\n"
        "Client side:\n"
        "  -p --port                      |tcp/ip address that program can be found\n"
        "  --ftp_timeout                  |logoff time (sec) for inactive client\n"
        "  --max_clients                  |maximum simultaneous clients (1-300)\n"
            ) ;
        break ;
    case opt_nfsd:
        printf(
        "     --NFS_program               |NFS: 100003, default OWNFS 300003\n"
        "     --NFS_version               |default 3\n"
        "     --tcp_only                  |no udp transport (overides udp_only)\n"
        "     --udp_only                  |no tcp transport (overides tcp_only)\n"
        "     --NFS_port                  |default 0=auto, 2049 traditional\n"
        "     --mount_port                |default 0=auto, usually same as NFS_port.\n"
        "     --no_portmapper             |Don't use the standard portmapper.\n"
        ) ;
        break ;
    default:
        break ;
    }
    printf(
    "\n"
    "  -h --help                      |Basic help page\n"
    "     --morehelp                  |This optional items help page\n"
    ) ;
}
