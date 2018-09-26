<?php
$cmd = "hostname -I";
$resp = trim( shell_exec ( $cmd ) );

if(isset($_GET['json']))  {
	header('Content-Type: application/json');
	echo json_encode( explode(" ", $resp) );
} else {
	echo $resp;
}
//Plot json

?>
