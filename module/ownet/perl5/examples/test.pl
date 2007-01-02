#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2007 Paul H. Alfille
# GPL license

# Start a test-server before running this script:
#    owserver -p 1234 --fake 10,10


use OWNet ;

die (
     "OWFS 1-wire OWNet perl program\n"
    ."  by Paul Alfille 2007 see http://www.owfs.org\n"
    ."Syntax:\n"
    ."\t$0 <host:port>\n"
    ."Example:\n"
    ."\t$0 127.0.0.1:1234\n"
    ) if ( $#ARGV != 0 ) ;

OWNet::Sock($ARGV[0]) ;
die ("Cannot open OWNet connection") if (!defined($OWNet::sock)) ;

print "Try dir\n" ;
OWNet::dir( $ARGV[0], "/" ) ;

print "Try present\n" ;
OWNet::present( $ARGV[0], "/10.67C6697351FF") ;

print "Try read\n" ;
OWNet::read($ARGV[0], "/10.67C6697351FF/temperature") ;

