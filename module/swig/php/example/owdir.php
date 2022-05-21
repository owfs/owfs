<?php

#load the php extension
require "load_php_OW.php";

if ( $argc < 2 ) {
    echo "Usage:\n";
    echo "\tphp owdir.php 1wire-adapter [dir]\n";
    echo "  1wire-adapter (required):\n";
    echo "\t'u' for USB -or-\n";
    echo "\t--fake=10,28 for a fake-adapter with DS18S20 and DS18B20-sensor -or-\n";
    echo "\t/dev/ttyS0 (or whatever) serial port -or-\n";
    echo "\t4304 for remote-server at port 4304\n";
    echo "\t192.168.1.1:4304 for remote-server at 192.168.1.1:4304\n";
    exit;
}

$adapter = $argv[1];

if ( $argc > 2 )
{
  $dir = $argv[2];
} else {
  $dir = "/";
}

#initialize the 1-wire bus with given argument
$init_result = init( $adapter );
if ( $init_result != '1' )
{
  echo "could not initialize the 1-wire bus.\n";
} else {
  $list_bus = get( $dir );
  echo "list_bus result: $list_bus\n";
}
finish();

?>
