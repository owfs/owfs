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

OWNet is an easy way to access B<owserver> and thence the 1-wire bus.

Dallas Semiconductor's 1-wire system uses simple wiring and unique addresses for it's interesting devices. The B<One Wire File System> is a suite of programs that hides 1-wire details behind a file system metaphor. B<owserver> connects to the 1-wire bus and provides network access.

B<OWNet> is a perl module that connects to B<owserver> and allows reading, writing and listing the 1-wire bus.

Then the following perl program prints the temperature:

 use OWNet ;
 print OWNET::read( "localhost:3000" , "/10.67C6697351FF/temperature" ) ."\n" ; 

There is the alternative object oriented form:

 use OWNet ;
 my $owserver = OWNET->new( "localhost:4304" ) ;
 print $owserver->read( "/10.67C6697351FF/temperature" ) ."\n" ; 

=head1 SYNTAX

=head2 methods

=over

=item B<new>

 my $owserver = OWNet -> new( address ) ;

=item B<read>

 read( address, path )
 $owserver -> read( path )

=item B<write>

 write( address, path, value )
 $owserver -> write( path, value )

=item B<dir>

 dir( address, path )
 $owserver -> dir( path )

=item B<present>

 present( address, path )
 $owserver -> present( path )

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

my $persistence_bit = 0x04 ;
my $default_sg = 0x102 ;
my $default_block = 4096 ;

#if ( !defined(&MSG_DONTWAIT) ) {
#  sub MSG_DONTWAIT { return 0 ; }
#}

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

    my $port ;
    if ( $addr =~ /(.*):(.*?)/ ) {
      $addr = $1 ;
      $port = $2 ;
    } elsif ( $addr =~/\D/ ) {
      $port = '' ;
    } else {
      $port = $addr ;
      $addr = '' ;
    }

    $self->{ADDR} = $addr ;
    $self->{PORT} = $port ;
    $self->{SG} = $default_sg + $tempscale + $format ;
    $self->{VER} = 0 ;
}

sub _Sock($) {
    my $self = shift ;
    # persistent socket already there?
    if ( defined($self->{SOCK} && $self->{PERSIST} != 0  ) ) { 
        return 1 ; 
    }
    # defaults
    my $addr = $self->{ADDR} ;
    my $port = $self->{PORT} ;
    $addr = '127.0.0.1' if $addr eq '' ;
    $port = 'owserver(4304)' if $port eq '' ;
    # New socket
    $self->{SOCK} = IO::Socket::INET->new(
            PeerAddr=>$addr,
            PeerPort=>$port,
            Proto=>'tcp')
        || do {
            warn("Can't open $addr:$port ($!) \n") if $self->{VERBOSE} ;
            $self->{SOCK} = undef ;
            return ;
        } ;
    return 1 ;
}

sub _self($) {
    my $addr = shift ;
    my $self ;
    if ( ref($addr) ) {
        $self = $addr ;
        $self->{PERSIST} = $persistence_bit ;
    } else {
        $self = {} ;
        _new($self,$addr)  ;
        $self->{PERSIST} = 0 ;
    }
    if ( ($self->{ADDR} eq '') && ($self->{PORT} eq '') ) {
        _BonjourLookup($self) || _Sock($self) || return ;
    } else {
        _Sock($self) || return ;
    }
    return $self;
}

sub _BonjourLookup($) {
    my $self = shift ;
    eval { require Net::Rendezvous; }; 
    if ($@) { 
        print "$@\n" if $self->{VERBOSE} ;
        return ;
    }
    my $owservers = Net::Rendezvous->new('owserver') || do {
        print "Unable to start owserver discovery via Net::Rendezvous $!\n" if $self->{VERBOSE} ;
        return ;
    } ;
    $owservers->discover ;
    my $owserver_selected = $owservers->shift_entry || do {
        print "No owserver discovered by Net::Rendezvous\n" if $self->{VERBOSE} ;
        return ;
    } ;
    print $owserver_selected->host.":".$owserver_selected->port."\n" ;
    # New socket
    $self->{SOCK} = IO::Socket::INET->new(PeerAddr=>$owserver_selected->host,PeerPort=>$owserver_selected->port,Proto=>'tcp') || do {
        warn("Can't open Bonjour (autodiscovered) port ($!) \n") if $self->{VERBOSE} ;
        $self->{SOCK} = undef ;
        return ;
    } ;
    $self->{ADDR} = $self->{SOCK}->peeraddr ;
    $self->{PORT} = $self->{SOCK}->peerport ;
    return 1 ;
}

sub _ToServer ($$$$$;$) {
    my ($self, $payload_length, $msg_type, $size, $offset, $payload_data) = @_ ;
    my $f = "N6" ;
    $f .= 'Z'.$payload_length if ( $payload_length > 0 ) ; 
    my $message = pack($f,$self->{VER},$payload_length,$msg_type,$self->{SG}|$self->{PERSIST},$size,$offset,$payload_data) ;

    # try to send
#    send( $self->{SOCK}, $message, MSG_DONTWAIT ) && return 1 ;
    send( $self->{SOCK}, $message, 0 ) && return 1 ;

    # maybe bad persistent connection
    if ( $self->{PERSIST} != 0 ) {
        $self->{SOCK} = undef ;
        _Sock($self) || return ;
#        send( $self->{SOCK}, $message, MSG_DONTWAIT ) && return 1 ;
        send( $self->{SOCK}, $message, 0 ) && return 1 ;
    }

	warn("Send problem $! \n") if $self->{VERBOSE} ;
	return ;
}

sub _FromServerLow($$) {
    my $self = shift ;
    my $length_wanted = shift ;
    return '' if $length_wanted == 0 ;
    my $fileno = $self->{SOCK}->fileno ;
    my $selectreadbits = '' ;
    vec($selectreadbits,$fileno,1) = 1 ;
    my $remaininglength = $length_wanted ;
    my $fullread = '' ;
    #print "LOOOP for length $length \n" ;
    do {
        select($selectreadbits,undef,undef,1) ;
        return if vec($selectreadbits,$fileno,1) == 0 ;
    #	return if $sel->can_read(1) == 0 ;
        my $partialread ;
#        defined( recv( $self->{SOCK}, $partialread, $remaininglength, MSG_DONTWAIT ) ) || do {
        defined( recv( $self->{SOCK}, $partialread, $remaininglength, 0 ) ) || do {
            warn("Trouble getting data back $! after $remaininglength of $length_wanted") if $self->{VERBOSE} ;
            return ;
        } ;
        #print "reading=".$a."\n";
        #print " length a=".length($a)." length ret=".length($ret)." length a+ret=".length($a.$ret)." \n" ;
        $fullread .= $partialread ;
        $remaininglength = $length_wanted - length($fullread) ;
        #print "_FromServerLow (a.len=".length($a)." $len of $length \n" ;
    } while $remaininglength > 0 ;
    return $fullread ;
}

sub _FromServer($) {
    my $self = shift ;
    my ( $version, $payload_length, $return_status, $sg, $size, $offset, $payload_data ) ;
    do {
	my $r = _FromServerLow( $self,24 ) || do {
	    warn("Trouble getting header $!") if $self->{VERBOSE} ;
	    return ;
	} ;
	($version, $payload_length, $return_status, $sg, $size, $offset) = unpack('N6', $r ) ;
	# returns unsigned (though originals signed
	# assume anything above 66000 is an error
	return if $return_status > 66000 ;
	#print "From Server, size = $siz, ret = $ret payload = $pay \n" ;
    } while $payload_length > 66000 ;
    $payload_data = _FromServerLow( $self,$payload_length ) ;
    if ( !defined($payload_data) ) { 
    	warn("Trouble getting payload $!") if $self->{VERBOSE} ;
      return ;
    } ;
    $payload_data = substr($payload_data,0,$size) ;
    #print "From Server, payload retrieved <$dat> \n" ;
    $self->{PERSIST} = $sg & $persistence_bit ;
    return ($version, $payload_length, $return_status, $sg, $size, $offset, $payload_data ) ;
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

sub new($$) {
    my $class = shift ;
    my $addr = shift || "" ;
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
    my $self = _self(shift) || return ;
    my $path = shift ;
    _ToServer($self,length($path)+1,$msg_read,$default_block,0,$path) ;
	my @r = _FromServer($self) ;
		return if !@r ;
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
    my $self = _self(shift) || return ;
    my $path = shift ;
    my $val = shift ;

	my $value_length = length($val) ;
	my $path_length = length($path)+1 ;
	my $payload = pack( 'Z'.$path_length.'A'.$value_length,$path,$val ) ;
	_ToServer($self,length($payload),$msg_write,$value_length,0,$payload) ;
	my @r = _FromServer($self) ;
		return if !@r ;
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
    my $self = _self(shift) || return ;
    my $path = shift ;
	_ToServer($self,length($path)+1,$msg_presence,$default_block,0,$path) ;
	my @r = _FromServer($self) ;
		return if !@r ;
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
    my $self = _self(shift) || return ;
    my $path = shift ;

    # new msg_dirall method -- single packet
    _ToServer($self,length($path)+1,$msg_dirall,$default_block,0,$path) || return ;
    my @r = _FromServer($self) ;
    if (@r) {
        $self->{SOCK} = undef if $self->{PERSIST} == 0 ;
        return $r[6] ;
    } ;

    # old msg_dir method -- many packets
    _Sock($self) || return ;
    _ToServer($self,length($path)+1,$msg_dir,$default_block,0,$path) || return ;
	my $dirlist = '' ;
	while (1) {
		@r = _FromServer($self) || return ;
		return if !@r ;
        if ( $r[1] == 0 ) { # last null packet
            $self->{SOCK} = undef if $self->{PERSIST} == 0 ;
            return substr($dirlist,1) ; # not starting comma
        }
		$dirlist .= ','.$r[6] ;
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

I<OWNet> can be used in either a classical (non-object-oriented) manner, or with objects. The object stored the ip address of the B<owserver> and a network socket to communicate. I<OWNet> will use persistent tcp connections for the object form -- potentially a performance boost over a slow network.

=head1 EXAMPLES

=head2 owserver

owserver is a separate process that must be accessible on the network. It allows multiple clients, and can connect to many physical 1-wire adapters and 1-wire devices. It's address must be discoverable -- either set on the command line, or at it's default location, or by using Bonjour (zeroconf) service discovery.

An example owserver invocation for a serial adapter and explicitly the default port:

 owserver -d /dev/ttyS0 -p 4304

=head2 OWNet

 use OWNet ;
 
 # Create owserver object
 my $owserver = OWNet->new('localhost:4304 -v -F') ; #default location, verbose errors, Fahrenheit degrees
 # my $owserver = OWNet->new() ; #simpler, again default location, no error messages, default Celsius

 #print directory
 print $owserver->dir('/') ;

 #print temperature from known device (DS18S20,  ID: 10.13224366A280)
 print "Temperature: ".$owserver->read('/uncached/10.13224366A280/temperature') ;

 # Now for some fun -- a tree of everything:
 sub Tree($$) {
   my $ow = shift ;
   my $path = shift ;

   print "$path\t" ;
  
   # first try to read
   my $value = $ow->read($path) ;
   if ( defined($value) ) { 
     print "$value\n"; 
     return ;
   } 

   # not readable, try as directory
   my $dirstring = $ow->dir($path) ;
   if ( defined($dirstring) ) { 
     print "<directory>\n" ; 
     my @dir = split /,/ ,  $ow->dir($path) ;
     foreach (@dir) {
        Tree($ow,$_) ;
     }
     return ;
   }
  
   # can't read, not directory
   print "<write-only>\n" ;
   return ;
 }

 Tree( $owserver, '/' ) ;

=head1 INTERNALS

=head2 Object properties (All private)

=item ADDR

literal sting for the IP address, in dotted-quad or host format. This property is also used to indicate a substantiated object.

=item PORT

string for the port number (or service name). Service name must be specified as :owserver or the like.

=item SG

Flag sent to server, and returned, that encodes temperature scale and display format. Persistence is also encoded in this word in the actual tcp message, but kept separately in the object.

=item VERBOSE

Print error messages? Set by "-v" in object invocation.

=item SOCK

Socket address (object) for communication. Stays defined for persistent connections, else deleted between calls.

=head2 Private methods

=item _self

Takes either the implicit object reference (if called on an object) or the ip address in non-object format. In either case a socket is created, the persistence bit is property set, and the address parsed. Returns the object reference, or undef on error. Called by each external method (read,write,dir) on the first parameter.

=item _new

Takes command line invocation parameters (for an object or not) and properly parses and sets up the properties in a hash array.

=item _Sock

Socket processing, including tests for persistence, and opening.
If no host is specified, localhost (127.0.0.1) is used.
If no port is specified, uses the IANA allocated well known port (4304) for owserver. First looks in /etc/services, then just tries 4304.

=item _ToServer

Sends formats and sends a message to owserver. If a persistent socket fails, retries after new socket created.

=item _FromServerLow

Reads a specified length from server

=item _FromServer

Reads whole packet from server, using _FromServerLow (first for header, then payload/tokens). Discards ping packets silently.

Uses the IANA allocated well known port (4304) for owserver. First looks in /etc/services, then just tries 4304.

=item _BonjourLookup

Uses the mDNS service discovery protocol to find an available owserver.
Employs NET::Rendezvous (an earlier name or Apple's Bonjour)
This module is loaded only if available using the method of http://sial.org/blog/2006/12/optional_perl_module_loading.html

Bounjour details for owserver at: 

=head1 AUTHOR

Paul H Alfille paul.alfille @ gmail . com

=head1 BUGS

Support for proper timeout using the "select" function seems broken in perl. This might leave the routines vulnerable to network timing errors.

=head1 SEE ALSO

=over

=item http://www.owfs.org

Documentation for the full B<owfs> program suite, including man pages for each of the supported 1-wire devices, and more extensive explanatation of owfs components.

=item http://owfs.sourceforge.net

Location where source code is hosted.

=back

=head1 COPYRIGHT

Copyright (c) 2007 Paul H Alfille. All rights reserved.
 This program is free software; you can redistribute it and/or
 modify it under the same terms as Perl itself.

=cut
