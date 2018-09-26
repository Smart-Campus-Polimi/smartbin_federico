<?php
require_once "src/Smartbin.php";

header('Content-Type: application/json');

$smartbin = new Smartbin();

$tolerance = 0.05;

if ( array_key_exists("id", $_GET) && !empty($_GET['id']) ) {
	//Get id
	$id = $_GET['id'];
	//Extract data
	$heavier = $smartbin->heavierThanExpected($id, $tolerance);

	//Create response
	$response = [
		"heavier" => $heavier,
		"tolerance" => $tolerance
	];
} else {
	$response = "Missing bin id";
}

//Plot json
echo json_encode($response);
?>
