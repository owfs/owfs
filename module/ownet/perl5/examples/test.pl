#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2007 Paul H. Alfille
# GPL license

# Start a test-server before running this script:
#    owserver -p 4304 --fake 10,10


use OWNet ;

die (
     "OWFS 1-wire OWNet perl program\n"
    ."  by Paul Alfille 2007 see http://www.owfs.org\n"
    ."Syntax:\n"
    ."\t$0 <host:port>\n"
    ."Example:\n"
    ."\t$0 127.0.0.1:4304\n"
    ) if ( $#ARGV != 0 ) ;

my $ow ;
$ow = OWNet->new($ARGV[0]) ;

print "\nTry dir\n" ;
print $ow->dir("/") ;
die ("Cannot open OWNet connection") if (!defined($ow->{SOCK})) ;

print "\nTry present\n" ;
print $ow->present("/10.67C6697351FF") ;

print "\nTry read\n" ;
print $ow->read("/10.67C6697351FF/temperature") ;

print "\n" ;
