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


print "\nCall dir\n" ;
print OWNet::dir($ARGV[0] . " -v", "/") ;

print "\nCall read\n" ;
print OWNet::read($ARGV[0] . " -v", "/10.67C6697351FF/temperature") ;

print "\nCall read (Fahrenheit)\n" ;
print OWNet::read($ARGV[0] . " -v -F", "/10.67C6697351FF/temperature") ;

print "\n" ;
