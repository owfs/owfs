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
    ."\t$0 1wire-adapter\n"
    ."  1wire-adapter (required):\n"
    ."\t'u' for USB -or-\n"
    ."\t/dev/ttyS0 (or whatever) serial port -or-\n"
    ."\t1234 for remote-server at port 1234\n"
    ."\t192.168.1.1:1234 for remote-server at 192.168.1.1:1234\n"
    ) if ( $#ARGV != 0 ) ;

OW::init($ARGV[0]) or die "Cannot open 1wire port $ARGV[0]" ;

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

