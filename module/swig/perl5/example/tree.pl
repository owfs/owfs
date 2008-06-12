#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL license

use OW ;

die (
     "OWFS 1-wire tree perl program\n"
    ."  by Paul Alfille 2004 see http://www.owfs.org\n"
    ."Syntax:\n"
    ."\t$0 1wire-adapter\n"
    ."  1wire-adapter (required):\n"
    ."\t'u' for USB -or-\n"
    ."\t--fake=10,28 for a fake-adapter with DS18S20 and DS18B20-sensor -or-\n"
    ."\t/dev/ttyS0 (or whatever) serial port -or-\n"
    ."\t4304 for remote-server at port 4304\n"
    ."\t192.168.1.1:4304 for remote-server at 192.168.1.1:4304\n"
    ) if ( $#ARGV != 0 ) ;

OW::init($ARGV[0]) or die "Cannot open 1wire port $ARGV[0]" ;
#OW::set_error_print(2);
#OW::set_error_level(9);

# for "paging" 
#my $lin = 0 ;

sub treelevel {
    my $lev = shift ;
    my $path = shift ;
#sleep 1;
#print "lev=$lev path=$path \n" ;
    my $res = OW::get($path) or return ;
    for (split(',',$res)) {
        for (1..$lev) { print("\t") } ;
#for paging
#if (++$lin == 30 ) {
#$lin=0 ;
#getc();
#}
        print $_ ;
        if ( m{/$} ) {
#            print "[".OW::get($path.$_)."]" ;
            print "\n" ;
            treelevel($lev+1,$path.$_) ;
} else {
            my $r = OW::get($path.$_) ;
            print ": $r" if defined($r) ;
            print "\n" ;
        }
    }
}

treelevel(0,"/") ;
OW::finish() ;

