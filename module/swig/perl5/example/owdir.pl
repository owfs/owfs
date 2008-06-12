#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL license

use OW ;

die (
     "OWFS 1-wire owdir perl program\n"
    ."Syntax:\n"
    ."\t$0 1wire-adapter [dir]\n"
    ."  1wire-adapter (required):\n"
    ."\t'u' for USB -or-\n"
    ."\t--fake=10,28 for a fake-adapter with DS18S20 and DS18B20-sensor -or-\n"
    ."\t/dev/ttyS0 (or whatever) serial port -or-\n"
    ."\t4304 for remote-server at port 4304\n"
    ."\t192.168.1.1:4304 for remote-server at 192.168.1.1:4304\n"
    ) if ( $#ARGV < 0 ) ;

OW::init($ARGV[0]) or die "Cannot open 1wire port $ARGV[0]" ;
#OW::set_error_print(2);
#OW::set_error_level(9);

if ( $#ARGV >= 1 ) {
    $dir = $ARGV[1];
} else {
    $dir = "/";
}

my $res = OW::get($dir) or print "Error reading directory\n" ;
print $res . "\n" if defined($res) ;

OW::finish() ;

