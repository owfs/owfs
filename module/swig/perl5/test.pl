#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL license

use OW ;

die (
     "OWFS 1-wire tree perl program\n"
    ."  by Paul Alfille 2004 see http://owfs.sourceforge.net\n"
    ."Syntax:\n"
    ."\t$0 1wire-port\n"
    ."  1wire-port (required):\n"
    ."\t'u' for USB -or-\n"
    ."\t/dev/ttyS0 (or whatever) serial port\n"
    ) if ( $#ARGV != 0 ) ;

OW::init($ARGV[0]) or die "Cannot open 1wire port $ARGV[0]" ;

sub treelevel {
    my $lev = shift ;
    my $path = shift ;
#print "lev=$lev path=$path \n" ;
    my $res = OW::get($path) or return ;
    for (split(',',$res)) {
        for (1..$lev) { print("\t") } ;
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

