#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL v2 license

# $Id$

use strict ;

die (
	"$0 -- part of the 1-Wire Filesystem Suite (OWFS)\n"
	."Syntax:\n"
	."\t$0 < mem.txt\n"
	."where mem.txt is the output from owserver\n\n"
	."Basically, this program parses the memory allocation output, matching\n"
	."allocation and free, and marking the unmatched lines with '!'\n\n"
	."To use, owserver must be compiled with OW_ALLOC_DEBUG and invoked\n"
	."\towserver -u -p 4304 --foreground > mem.txt\n\n"
	."By Paul H Alfille 2009\n"
	."See http://www.owfs.org\n"
) if ( $#ARGV >= 0 ) ;

my @lines = reverse(<>) ;
my @addr ;
my @status ;

foreach my $line (@lines) {
	$line =~ /^([0123456789abcdefABCDEFxXS]+) / ;
	$addr[++$#addr] = $1 ;
	if ($line =~ m/FREE/) {
		$status[++$#status] = 1 ;
	} else {
		$status[++$#status] = 0 ;
	}
}

my $count = $#lines ;
foreach (my $index = 0; $index <= $count ; ++$index) {
	if ($status[$index] == 0) {
		$lines[$index]="! ".$lines[$index] ;
	} elsif ($status[$index] == 1) {
		for (my $j = $index+1 ; $j <= $count ; ++$j) {
			if ($addr[$index] eq $addr[$j]) {
				if ($status[$j]==0) {
					$lines[$index]="< ".$lines[$index] ;
					$lines[$j]="> ".$lines[$j] ;
					$status[$j] = -1 ;
				} else {
					$lines[$index]="! ".$lines[$index] ;
				}
				last ;
			}
		}
	}
}

@lines = reverse(@lines);
printf join('',@lines) ;
