#!/usr/bin/perl -w

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL v2 license

# $Id$

use OW ;

die (
	"OWFS 1-wire owdata perl program\n"
	."Syntax:\n"
	."\t$0 1wire-adapter [dir] \n"
	."  1wire-adapter (required):\n"
	."\t'-u' for USB -or-\n"
	."\t--fake=10,28 for a fake-adapter with DS18S20 and DS18B20-sensor -or-\n"
	."\t-d /dev/ttyS0 (or whatever) serial port -or-\n"
	."\t-s 4304 for remote-server at port 4304\n"
	."\t-s 192.168.1.1:4304 for remote-server at 192.168.1.1:4304\n"
	) if ( $#ARGV < 0 ) ;

OW::init($ARGV[0]) or die "Cannot open 1wire port $ARGV[0]" ;
#OW::set_error_print(2);
#OW::set_error_level(9);

my $dir = "/" ;
if ( $#ARGV >= 1 ) {
	$dir = $ARGV[1];
}

# show a property (if readable)
# input leading_tabs, path, property
sub property_read {
	my $prefix = shift ;
	my $path = shift ;
	my $property = shift ;

	my $value = OW::get($path.$property) or return ;
	print $prefix.$property."\t".$value."\n";
}

# process a directory
# input leading_tabs, path, file
sub directory_read {
	my $prefix = shift ;
	my $path = shift ;
	my $file = shift ;

	my $subprefix = $prefix."\t" ;
	my $fullpath = $path.$file ;
	my $subpath = OW::get($fullpath) or return ;

	# remove boring entries
	$subpath =~ s/r_id//g ;
	$subpath =~ s/id//g ;
	$subpath =~ s/r_address//g ;
	$subpath =~ s/address//g ;
	$subpath =~ s/r_locator//g ;
	$subpath =~ s/locator//g ;
	$subpath =~ s/family//g ;
	$subpath =~ s/crc8//g ;
	$subpath =~ s/present//g ;
	$subpath =~ s/,+/,/g ;
	$subpath =~ s/^,//g ;

	print $prefix.$file."\n";

	foreach (split(/,/,$subpath)) {
		if (m/\/$/) {
			directory_read($subprefix,$fullpath,$_) ;
		} else {
			property_read($subprefix,$fullpath,$_);
		}
	}
}

# main routine
my $maindir = OW::get($dir) or print "Error reading directory\n"  or die ("Cannot read directory");

# remove noisy directories
$maindir =~ s/bus\.\d+\///g ;
$maindir =~ s/uncached\///g ;
$maindir =~ s/structure\///g ;
$maindir =~ s/simultaneous\///g ;
$maindir =~ s/alarm\///g ;
$maindir =~ s/,+/,/g ;

# process main dir
foreach my $subdir (split(/,/,$maindir)) {
	directory_read( "" , $dir , $subdir ) ;    
}

OW::finish() ;

