<?php
require_once "src/Smartbin.php";

header('Content-Type: application/json');

if ( array_key_exists("id", $_GET) && !empty($_GET['id']) ) {
	
	//Get id
	$id = $_GET['id'];
	
	//Create smartbin object
	$smartbin = new Smartbin();
	
	//Extract data
	$ratio = $smartbin->getFillStatus($id);

	$response = [
		"ratio" => $ratio
	];
} else {
	$response = "Missing bin id";
}
//Plot json
echo json_encode($response);
?>
