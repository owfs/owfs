#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL v2 license

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

# read in all the lines, revered
my @lines = reverse(<>) ;

# array of addresses (for each line)
my @addr ;
# array of results (for each line)
my @status ;

foreach my $line (@lines) {
	$line =~ /^([0123456789abcdefABCDEFxXS]+) / ;
	if ( defined($1)) {
		$addr[++$#addr] = $1 ;
	} else {
		$addr[++$#addr] = "txt" ;
	}
	if ($line =~ m/^\(nil\)/) {
		$status[++$#status] = "nil" ;
	} elsif ($line =~ m/FREE/) {
		$status[++$#status] = "free" ;
	} elsif ($line =~ m/MALLOC/) {
		$status[++$#status] = "alloc" ;
	} elsif ($line =~ m/REALLOC/) {
		if ($line =~ /free/) {
			$status[++$#status] = "free" ;
		} else {
			$status[++$#status] = "alloc" ;
		}
	} elsif ($line =~ m/STRDUP/) {
		$status[++$#status] = "alloc" ;
	} elsif ($line =~ m/CALLOC /) {
		$status[++$#status] = "alloc" ;
	} else {
		# probably a debug message, not memory data
		$status[++$#status] = "unknown" ;
		$addr[$#addr] = "txt" ;
	}
}

my $total_lines = $#lines ;
foreach (my $back_to_front = 0; $back_to_front <= $total_lines ; ++$back_to_front) {
	if ($status[$back_to_front] eq "nil") {
		$lines[$back_to_front]="= ".$lines[$back_to_front] ;
	} elsif ($status[$back_to_front] eq "alloc") {
		$lines[$back_to_front]="! ".$lines[$back_to_front] ;
	} elsif ($status[$back_to_front] eq "free") {
		for (my $prior_line = $back_to_front+1 ; $prior_line <= $total_lines ; ++$prior_line) {
			if ($addr[$back_to_front] eq $addr[$prior_line]) {
				if ($status[$prior_line] eq "alloc") {
					$lines[$back_to_front]="< ".$lines[$back_to_front] ;
					$lines[$prior_line]="> ".$lines[$prior_line] ;
					$status[$prior_line] = "matched" ;
				} else {
					$lines[$back_to_front]="! ".$lines[$back_to_front] ;
				}
				last ;
			}
		}
	}
}

@lines = reverse(@lines);
printf join('',@lines) ;
