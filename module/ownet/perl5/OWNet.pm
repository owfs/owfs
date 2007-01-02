#!/usr/bin/perl -w
# OWNet module for perl
# $Id$
#
# Paul H Alfille -- copyright 2007
# Part of the OWFS suite: 
#  http://www.owfs.org
#  http://owfs.sourceforge.net

=head1 NAME

B<OWNet>
Light weight access to B<owserver>

=head1 SYNOPSIS

=over

=item B<read>( I<address> , I<path> )

=item B<write>( I<address> , I<path> , I<value> )

=item B<dir>( I<address> , I<path> )

=item B<present>( I<address> , I<path> )

=back

=head2 I<address>

TCP/IP I<address> of B<owserver>. Valid forms:

=over

=item I<name> test.owfs.net:3001

=item I<quad> number: 123.231.312.213:3001

=item I<host> localhost:3001

=item I<port> 3001

=back

=head2 I<path>

I<path> to an item on the 1-wire bus. Valid forms:

=over

=item main directories

Used for the I<dir> method. E.g. "/" "/uncached" "/1F.321432320000/main"

=item device directory

Used for the I<dir> and I<present> method. E.g. "/10.4300AC220000" "/statistics"

=item device properties

Used to I<read>, I<write>. E.g. "/10.4300AC220000/temperature"

=back

=head2 I<value>

New I<value> for a device property. Used by I<write>.

=head1 METHODS

=over

=cut

package OWNet ;

BEGIN { }

use strict ;

use IO::Socket::INET ;
use bytes ;

my $sock ;

sub _Sock($) {
	my $addr = shift ;
	$OWNet::sock = IO::Socket::INET->new(PeerAddr=>$addr,Proto=>'tcp') || do {
		warn("Can't open $addr ($!) \n") ;
		$OWNet::sock = undef ;
	} ;
	return ( 258 ) ;
}

sub _ToServer ($$$$$$;$) {
	my ($ver, $pay, $typ, $sg, $siz, $off, $dat) = @_ ;
	my $f = "N6" ;
	$f .= 'Z'.$pay if ( $pay > 0 ) ; 
	print "Sock = $OWNet::sock  mesg:($ver,$pay,$typ,$sg,$siz,$off,$dat) \n" ;
	send( $OWNet::sock, pack($f,$ver,$pay,$typ,$sg,$siz,$off,$dat), MSG_DONTWAIT ) || do { 		warn("Send problem $! \n");
		return ;
	} ;
	return 1 ;
}

sub _FromServerLow ($) {
	my $length = shift ;
	return '' if $length == 0 ;
	my $sel = '' ;
	vec($sel,$OWNet::sock->fileno,1) = 1 ;
	my $len = $length ;
	my $ret = '' ;
	my $a ;
	#print "LOOOP for length $length \n" ;
	do {
		print "LOW: ".join(',',select($sel,undef,undef,1))." \n" ;
		return if vec($sel,$OWNet::sock->fileno,1) == 0 ;
#		return if $sel->can_read(1) == 0 ;
		defined( recv( $OWNet::sock, $a, $len, MSG_DONTWAIT ) ) || do {
			warn("Trouble getting data back $! after $len of $length") ;
			return ;
		} ;
		#print " length a=".length($a)." length ret=".length($ret)." length a+ret=".length($a.$ret)." \n" ;
		$ret .= $a ;
		$len = $length - length($ret) ;
		#print "_FromServerLow (a.len=".length($a)." $len of $length \n" ;
	} while $len > 0 ;
	return $ret ;
}

sub _FromServer () {
	my ( $ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
	do {
		my $r = _FromServerLow( 24 ) || do {
			warn("Trouble getting header $!") ;
			return ;
		} ;
		($ver, $pay, $ret, $sg, $siz, $off) = unpack('N6', $r ) ;
		# returns unsigned (though originals signed
		# assume anything above 66000 is an error
		return if $ret > 66000 ;
		#print "From Server, size = $siz, ret = $ret payload = $pay \n" ;
	} while $pay > 66000 ;
	$dat = _FromServerLow( $pay ) ;
	if ( !defined($dat) ) { 
		warn("Trouble getting payload $!") ;
		return ;
	} ;
	$dat = substr($dat,0,$siz) ;
	#print "From Server, payload retrieved <$dat> \n" ;
	return ($ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
}

=item I<read>

B<read>( I<address> , <I>path )

Read the value of a 1-wire device property. Returns the (scalar string) value of the property.

Error (and undef return value) if:

=over

=item 1 No <B>owserver at I<address>

=item 2 Bad I<path> 

=item 3 I<path> not a readable device property

=back

=cut

sub read($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = _Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print _ToServer(0,length($path)+1,2,$sg,4096,0,$path) ;
	my @r = _FromServer() ;
	return $r[6] ;
}

=item I<write>

B<write>( I<address> , <I>path  , <I>write )

Set the value of a 1-wire device property. Returns "1" on success.

Error (and undef return value) if:

=over

=item 1 No <B>owserver at I<address>

=item 2 Bad I<path> 

=item 3 I<path> not a writable device property

=item 4 I<value> incorrect size or format

=back

=cut

sub write($$$) {
	local $OWNet::sock ;
	my ( $addr,$path,$val ) = @_ ;
	my $siz = length($val) ;
	my $s1 = length($path)+1 ;
	my $dat = pack( 'Z'.$s1.'A'.$siz,$path,$val ) ;
	my ( $sg ) = _Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print _ToServer(0,length($dat),3,$sg,$siz,0,$dat) ;
	my @r = _FromServer() ;
	return $r[2]>=0 ;
}

=item I<present>

B<present>( I<address> , <I>path )

Test if a 1-wire device exists.

Error (and undef return value) if:

=over

=item 1 No <B>owserver at I<address>

=item 2 Bad I<path> 

=item 3 I<path> not a device

=back

=cut

sub present($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = _Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print _ToServer(0,length($path)+1,6,$sg,4096,0,$path) ;
	my @r = _FromServer() ;
	return $r[2]>=0 ;
}

=item I<dir>

B<dir>( I<address> , <I>path )

Return a comma-separated list of the entries in I<path>. Entries are equivalent to "fully qualified names" -- full path names.

Error (and undef return value) if:

=over

=item 1 No <B>owserver at I<address>

=item 2 Bad I<path> 

=item 3 I<path> not a directory

=back

=cut

sub dir($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = _Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		return ;
	} ;
	_ToServer(0,length($path)+1,4,$sg,4096,0,$path) || do {
		warn "Couldn't SEND directory request to $addr.\n" ;
		return ;
	} ;
	my $ret = '' ;
	while (1) {
		my @r = _FromServer() ;
		if (!@r) { return ; } ;
		return substr($ret,1) if $r[1] == 0 ;
		$ret .= ','.$r[6] ;
	}
}

return 1 ;

END { }

=back

=head1 DESCRIPTION

=head2 OWFS

I<OWFS> is a suite of programs that allows easy access to I<Dallas Semiconductor>'s 1-wire bus and devices. I<OWFS> provides a consistent naming scheme, safe multplexing of 1-wire traffice, multiple methods of access and display, and network access. The basic I<OWFS> metaphor is a file-system, with the bus beinng the root directory, each device a subdirectory, and the the device properties (e.g. voltage, temperature, memory) a file.

=head2 1-Wire

I<1-wire> is a protocol allowing simple connection of inexpensive devices. Each device has a unique ID number (used in it's OWFS address) and is individually addressable. The bus itself is extremely simple -- a data line and a ground. The data line also provides power. 1-wire devices come in a variety of packages -- chips, commercial boxes, and iButtons (stainless steel cans). 1-wire devices have a variety of capabilities, from simple ID to complex voltage, temperature, current measurements, memory, and switch control.

=head2 Programs

Connection to the 1-wire bus is either done by bit-banging a digital pin on the processor, or by using a bus master -- USB, serial, i2c, parallel. The heavy-weight I<OWFS> programs: B<owserver> B<owfs> B<owhttpd> B<owftpd> and the heavy-weight perl module B<OW> all link in the full I<OWFS> library and can connect directly to the bus master(s) and/or to B<owserver>.  

B<OWNet> is a light-weight module. It connects only to an B<owserver>, does not link in the I<OWFS> library, and should be more portable.

=head1 AUTHOR

Paul H Alfille paul.alfille @ gmail . com

=head1 BUGS

Support for proper timeout using the "select" function seems broken in perl. This lease the routines vulnerable to network timing errors.

=head1 SEE ALSO

=over

=item http://www.owfs.org

Documentation for the full B<owfs> program suite, including man pages for each of the supported 1-wire devices, nand more extensive explanatation of owfs components.

=item http://owfs.sourceforge.net

Location where source code is hosted.

=back

=cut
