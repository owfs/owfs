package OWNet ;

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

OWNet is an easy way to access the B<owserver> and thus the 1-wire bus.

Suppose a 1-wire adapter (serial DS9097U) attaches to a 1-wire bus with a single DS18S20 temperature sensor. B<owserver> was invoked with:

 owserver -d /dev/ttyS0 -p 3000

Then the following perl program prints the temperature:

 use OWNet ;
 print OWNET::read( "localhost:3000" , "/10.67C6697351FF/temperature" ) ."\n" ; 

There is the alternative object oriented form:

 use OWNet ;
 my $owserver = OWNET->new( "localhost:3000" ) ;
 print $owserver->read( "/10.67C6697351FF/temperature" ) ."\n" ; 

=head1 SYNTAX

=head2 methods

=over

=item B<new>

my $ownet = OWNet::B<new>( I<address> ) ;

=item B<read>

B<read>( I<address> , I<path> )

$ownet->B<read>( I<path> )

=item B<write>

B<write>( I<address> , I<path> , I<value> )

$ownet->B<write>( I<path> , I<value> )

=item B<dir>

B<dir>( I<address> , I<path> )

$ownet->B<dir>( I<path> )

=item B<present>

B<present>( I<address> , I<path> )

$ownet->B<present>( I<path> )

=back

=head2 I<address>

TCP/IP I<address> of B<owserver>. Valid forms:

=over

=item I<name> test.owfs.net:3001

=item I<quad> number: 123.231.312.213:3001

=item I<host> localhost:3001

=item I<port> 3001

=back

Temperature scale can also be specified in the I<address>. Same syntax as the other OWFS programs:

=over

=item -C Celsius (Centigrade)

=item -F Fahrenheit

=item -K Kelvin

=item -R Rankine

=back

Device display format (1-wire unique address) can also be specified in the I<address>, with the general form of -ff[.]i[[.]c] (I<f>amily I<i>d I<c>rc):

=over

=item -ff.i   /10.67C6697351FF (default)

=item -ffi    /1067C6697351FF

=item -ff.i.c /10.67C6697351FF.8D

=item -ff.ic  /10.67C6697351FF8D

=item -ffi.c  /1067C6697351FF.8D

=item -ffic   /1067C6697351FF8D

=back

Warning messages will only be display if verbose flag is specified in I<address>

=over

=item -v      verbose

=back

=head2 I<path>

B<owfs>-type I<path> to an item on the 1-wire bus. Valid forms:

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

BEGIN { }

use 5.008 ;
use warnings ;
use strict ;	

use IO::Socket::INET ;
use bytes ;

my $msg_read = 2 ;
my $msg_write = 3 ;
my $msg_dir = 4 ;
my $msg_presence = 6 ;
my $msg_dirall = 7 ;
my $msg_get = 8 ;


our $VERSION=(split(/ /,q[$Revision$]))[1] ;

sub _new($$) {
    my ($self,$addr) = @_ ;

    my $tempscale = 0 ;
    TEMPSCALE: {
        $tempscale = 0x00000 , last TEMPSCALE if $addr =~ /-C/ ;
        $tempscale = 0x10000 , last TEMPSCALE if $addr =~ /-F/ ;
        $tempscale = 0x20000 , last TEMPSCALE if $addr =~ /-K/ ;
        $tempscale = 0x30000 , last TEMPSCALE if $addr =~ /-R/ ;
    }

    my $format = 0 ;
    FORMAT: {
        $format = 0x2000000 , last FORMAT if $addr =~ /-ff\.i\.c/ ;
        $format = 0x4000000 , last FORMAT if $addr =~ /-ffi\.c/ ;
        $format = 0x3000000 , last FORMAT if $addr =~ /-ff\.ic/ ;
        $format = 0x5000000 , last FORMAT if $addr =~ /-ffic/ ;
        $format = 0x0000000 , last FORMAT if $addr =~ /-ff\.i/ ;
        $format = 0x1000000 , last FORMAT if $addr =~ /-ffi/ ;
        $format = 0x2000000 , last FORMAT if $addr =~ /-f\.i\.c/ ;
        $format = 0x4000000 , last FORMAT if $addr =~ /-fi\.c/ ;
        $format = 0x3000000 , last FORMAT if $addr =~ /-f\.ic/ ;
        $format = 0x5000000 , last FORMAT if $addr =~ /-fic/ ;
        $format = 0x0000000 , last FORMAT if $addr =~ /-f\.i/ ;
        $format = 0x1000000 , last FORMAT if $addr =~ /-fi/ ;
    }

	$self->{VERBOSE} = 1 if $addr =~ /-v/ ;

    $addr =~ s/-[\w\.]*//g ;
    $addr =~ s/ //g ;

    $addr = "127.0.0.1:".$addr unless $addr =~ /:/ ;
    $addr = "127.0.0.1".$addr if $addr =~ /^:/ ;

    $self->{ADDR} = $addr ;
    $self->{SG} = 0x0102 + $tempscale + $format ;
    $self->{VER} = 0 ;
}

sub _Sock($) {
    my $self = shift ;
    $self->{SOCK} = IO::Socket::INET->new(PeerAddr=>$self->{ADDR},Proto=>'tcp') || do {
	warn("Can't open $self->{ADDR} ($!) \n") if $self->{VERBOSE} ;
	$self->{SOCK} = undef ;
    } ;
    return ( 258 ) ;
}

sub _ToServer ($$$$$;$) {
    my ($self, $pay, $typ, $siz, $off, $dat) = @_ ;
    my $f = "N6" ;
    $f .= 'Z'.$pay if ( $pay > 0 ) ; 
    send( $self->{SOCK}, pack($f,$self->{VER},$pay,$typ,$self->{SG},$siz,$off,$dat), MSG_DONTWAIT ) || do {
	warn("Send problem $! \n") if $self->{VERBOSE} ;
	return ;
    } ;
    return 1 ;
}

sub _FromServerLow($$) {
    my $self = shift ;
    my $length = shift ;
    return '' if $length == 0 ;
    my $sock = $self->{SOCK} ;
    my $sel = '' ;
    vec($sel,$sock->fileno,1) = 1 ;
    my $len = $length ;
    my $ret = '' ;
    my $a ;
    #print "LOOOP for length $length \n" ;
    do {
	select($sel,undef,undef,1) ;
	return if vec($sel,$sock->fileno,1) == 0 ;
#	return if $sel->can_read(1) == 0 ;
	defined( recv( $self->{SOCK}, $a, $len, MSG_DONTWAIT ) ) || do {
	    warn("Trouble getting data back $! after $len of $length") if $self->{VERBOSE} ;
	    return ;
	} ;
	#print "reading=".$a."\n";
	#print " length a=".length($a)." length ret=".length($ret)." length a+ret=".length($a.$ret)." \n" ;
	$ret .= $a ;
	$len = $length - length($ret) ;
	#print "_FromServerLow (a.len=".length($a)." $len of $length \n" ;
    } while $len > 0 ;
    return $ret ;
}

sub _FromServer($) {
    my $self = shift ;
    my ( $ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
    do {
	my $r = _FromServerLow( $self,24 ) || do {
	    warn("Trouble getting header $!") if $self->{VERBOSE} ;
	    return ;
	} ;
	($ver, $pay, $ret, $sg, $siz, $off) = unpack('N6', $r ) ;
	# returns unsigned (though originals signed
	# assume anything above 66000 is an error
	return if $ret > 66000 ;
	#print "From Server, size = $siz, ret = $ret payload = $pay \n" ;
    } while $pay > 66000 ;
    $dat = _FromServerLow( $self,$pay ) ;
    if ( !defined($dat) ) { 
	warn("Trouble getting payload $!") if $self->{VERBOSE} ;
	return ;
    } ;
    $dat = substr($dat,0,$siz) ;
    #print "From Server, payload retrieved <$dat> \n" ;
    return ($ver, $pay, $ret, $sg, $siz, $off, $dat ) ;
}

=item I<new>

B<new>( I<address> )

Create a new OWNet object -- corresponds to an B<owserver>.

Error (and undef return value) if:

=over

=item 1 Badly formed tcp/ip I<address>

=item 2 No <B>owserver at I<address>

=back

=cut

sub newtest($$) {
    my $class = $_[0];
#    my addr => $_[1];
    my $objref = { addr => $_[1], };
#    _new($self, $addr) ;
#    if ( !defined($self->{SOCK}) ) {
#        return ;
#    } ;
    bless $objref, $class ;
    return $objref ;
}

sub new($$) {
    my $class = shift ;
    my $addr = shift ;
    my $self = {} ;
    _new($self,$addr) ;
    if ( !defined($self->{ADDR}) ) {
        return ;
    } ;
    bless $self, $class ;
    return $self ;
}

=item I<read>

=over

=item Non object-oriented:

B<read>( I<address> , I<path> )

=item Object-oriented:

$ownet->B<read>( I<path> )

=back


Read the value of a 1-wire device property. Returns the (scalar string) value of the property.

Error (and undef return value) if:

=over

=item 1 (Non object) No B<owserver> at I<address>

=item 2 (Object form) Not called with a valid OWNet object

=item 3 Bad I<path> 

=item 4 I<path> not a readable device property

=back

=cut

sub read($$) {
	my ( $addr,$path ) = @_ ;
	my $self ;
	if ( ref($addr) ) {
		$self = $addr ;
} else {
		$self = {} ;
		_new($self,$addr)  ;
	}
    _Sock($self) ;
	if ( !defined($self->{SOCK}) ) {
		return ;
	} ;
	_ToServer($self,length($path)+1,$msg_read,4096,0,$path) ;
	my @r = _FromServer($self) ;
	return $r[6] ;
}

=item I<write>

=over

=item Non object-oriented:

B<write>( I<address> , I<path> , I<value> )

=item Object-oriented:

$ownet->B<write>( I<path> , I<value> )

=back

Set the value of a 1-wire device property. Returns "1" on success.

Error (and undef return value) if:

=over

=item 1 (Non object) No B<owserver> at I<address>

=item 2 (Object form) Not called with a valid OWNet object

=item 3 Bad I<path> 

=item 4 I<path> not a writable device property

=item 5 I<value> incorrect size or format

=back

=cut

sub write($$$) {
	my ( $addr,$path, $val ) = @_ ;
	my $self ;
	if ( ref($addr) ) {
		$self = $addr ;
} else {
		$self = {} ;
		_new($self,$addr)  ;
	}
    _Sock($self) ;
	if ( !defined($self->{SOCK}) ) {
		return ;
	} ;
	my $siz = length($val) ;
	my $s1 = length($path)+1 ;
	my $dat = pack( 'Z'.$s1.'A'.$siz,$path,$val ) ;
	_ToServer($self,length($dat),$msg_write,$siz,0,$dat) ;
	my @r = _FromServer($self) ;
	return $r[2]>=0 ;
}

=item I<present>

=over

=item Non object-oriented:

B<present>( I<address> , I<path> )

=item Object-oriented:

$ownet->B<present>( I<path> )

=back

Test if a 1-wire device exists.

Error (and undef return value) if:

=over

=item 1 (Non object) No B<owserver> at I<address>

=item 2 (Object form) Not called with a valid OWNet object

=item 3 Bad I<path> 

=item 4 I<path> not a device

=back

=cut

sub present($$) {
	my ( $addr,$path ) = @_ ;
	my $self ;
	if ( ref($addr) ) {
		$self = $addr ;
} else {
		$self = {} ;
		_new($self,$addr)  ;
	}
    _Sock($self) ;
	if ( !defined($self->{SOCK}) ) {
		return ;
	} ;
	_ToServer($self,length($path)+1,$msg_presence,4096,0,$path) ;
	my @r = _FromServer($self) ;
	return $r[2]>=0 ;
}

=item I<dir>

=over

=item Non object-oriented:

B<dir>( I<address> , I<path> )

=item Object-oriented:

$ownet->B<dir>( I<path> )

=back

Return a comma-separated list of the entries in I<path>. Entries are equivalent to "fully qualified names" -- full path names.

Error (and undef return value) if:

=over

=item 1 (Non object) No B<owserver> at I<address>

=item 2 (Object form) Not called with a valid OWNet object

=item 3 Bad I<path> 

=item 4 I<path> not a directory

=back

=cut

sub dir($$) {
	my ( $addr,$path ) = @_ ;
	my $self ;
	if ( ref($addr) ) {
		$self = $addr ;
	} else {
		$self = {} ;
		_new($self,$addr)  ;
	}
    # new msg_dirall method -- single packet
    _Sock($self) ;
    if ( !defined($self->{SOCK}) ) {
        return ;
    } ;
    _ToServer($self,length($path)+1,$msg_dirall,4096,0,$path) || do {
        warn "Couldn't SEND directory all request to $addr.\n" if $self->{VERBOSE} ;
        return ;
    } ;
    my @r = _FromServer($self) ;
    if (@r) {
        return $r[6] ;
    } ;
    # old msg_dir method -- many packets
    _Sock($self) ;
    if ( !defined($self->{SOCK}) ) {
        return ;
    } ;
    _ToServer($self,length($path)+1,$msg_dir,4096,0,$path) || do {
        warn "Couldn't SEND directory request to $addr.\n" if $self->{VERBOSE} ;
        return ;
    } ;
	my $ret = '' ;
	while (1) {
		@r = _FromServer($self) ;
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

B<OWNet> is a light-weight module. It connects only to an B<owserver>, does not link in the I<OWFS> library, and should be more portable..

=head2 Object-oriented

I<OWNet> can be used in either a classical (non-object-oriented) manner, or with objects. The object stored the ip address of the B<owserver> and a network socket to communicate.

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

=head1 COPYRIGHT

Copyright (c) 2007 Paul H Alfille. All rights reserved.
 This program is free software; you can redistribute it and/or
 modify it under the same terms as Perl itself.

=cut
