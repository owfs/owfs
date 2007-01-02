#!/usr/bin/perl -w
# OWNet module for perl
# $Id$
#
# Paul H Alfille -- copyright 2007
# Part of the OWFS suite: 
#  http://www.owfs.org
#  http://owfs.sourceforge.net

package OWNet ;

BEGIN { }

use strict ;

use IO::Socket::INET ;

my $sock ;

sub Sock($) {
	my $addr = shift ;
	$OWNet::sock = IO::Socket::INET->new(PeerAddr=>$addr,Proto=>'tcp') || do {
		warn("Can't open $addr ($!) \n") ;
		$OWNet::sock = undef ;
	} ;
	return ( 258 ) ;
}

sub ToServer ($$$$$$;$) {
	my ($ver, $pay, $typ, $sg, $siz, $off, $dat) = @_ ;
	my $f = "N6" ;
	$f .= 'Z'.$pay if ( $pay > 0 ) ; 
	print "Sock = $OWNet::sock  mesg:($ver,$pay,$typ,$sg,$siz,$off,$dat) \n" ;
	do { use bytes ; print "To server length = ".length(pack($f,$ver,$pay,$typ,$sg,$siz,$off,$dat))." \n" ; } ;
	send( $OWNet::sock, pack($f,$ver,$pay,$typ,$sg,$siz,$off,$dat), MSG_DONTWAIT ) || do { 		warn("Send problem $! \n");
		return ;
	} ;
	return 1 ;
}

sub FromServerLow ($) {
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
		do { use bytes ; $len = $length - length($ret); } ;
		#print "FromServerLow (a.len=".length($a)." $len of $length \n" ;
	} while $len > 0 ;
	return $ret ;
}

sub FromServer () {
	my ( $ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
	do {
		my $r = FromServerLow( 24 ) || do {
			warn("Trouble getting header $!") ;
			return ;
		} ;
		($ver, $pay, $ret, $sg, $siz, $off) = unpack('N6', $r ) ;
		# returns unsigned (though originals signed
		# assume anything above 66000 is an error
		return if $ret > 66000 ;
		#print "From Server, size = $siz, ret = $ret payload = $pay \n" ;
	} while $pay > 66000 ;
	$dat = FromServerLow( $pay ) ;
	if ( !defined($dat) ) { 
		warn("Trouble getting payload $!") ;
		return ;
	} ;
	$dat = substr($dat,0,$siz) ;
	#print "From Server, payload retrieved <$dat> \n" ;
	return ($ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
}

sub read($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print ToServer(0,length($path)+1,2,$sg,4096,0,$path) ;
	my @r = FromServer() ;
	return $r[6] ;
}

sub write($$$) {
	local $OWNet::sock ;
	my ( $addr,$path,$val ) = @_ ;
	my $siz = length($val) ;
	my $s1 = length($path)+1 ;
	my $dat = pack( 'Z'.$s1.'A'.$siz,$path,$val ) ;
	my ( $sg ) = Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print ToServer(0,length($dat),3,$sg,$siz,0,$dat) ;
	my @r = FromServer() ;
	return $r[2]>=0 ;
}

sub present($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		warn "Sock problem $OWNet::sock \n" ;
		return ;
	} ;
	print ToServer(0,length($path)+1,6,$sg,4096,0,$path) ;
	my @r = FromServer() ;
	return $r[2]>=0 ;
}

sub dir($$) {
	local $OWNet::sock ;
	my ( $addr,$path ) = @_ ;
	my ( $sg ) = Sock($addr)  ;
	if ( !defined($OWNet::sock) ) {
		return ;
	} ;
	ToServer(0,length($path)+1,4,$sg,4096,0,$path) || do {
		warn "Couldn't SEND directory request to $addr.\n" ;
		return ;
	} ;
	my $ret = '' ;
	while (1) {
		my @r = FromServer() ;
		if (!@r) { return ; } ;
		return substr($ret,1) if $r[1] == 0 ;
		$ret .= ','.$r[6] ;
	}
}

return 1 ;

END { }
