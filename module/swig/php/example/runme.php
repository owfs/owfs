<?php

#load the php extension
require "load_php_OW.php";

if ( $argc > 1 )
{
  $arg = $argv[1];
} else {
  $arg = "u";
}

#initialize the 1-wire bus with given argument
$init_result = init( $arg );
if ( $init_result != '1' )
{
  echo "could not initialize the 1-wire bus.\n";
}
else
{
  $list_bus = get( "/" );
  echo "list_bus result: $list_bus\n";

  finish( );
}

?>
