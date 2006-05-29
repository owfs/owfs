<?php

#load the php extension
require "load_php_OW.php";

#initialize the 1-wire bus (usb bus)
$init_result = init( "u" );
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
