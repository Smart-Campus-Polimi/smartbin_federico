<?php 
require "src/Smartbin.php"; 
require "src/Rekognition.php"; 

//Crate smartbin object
$smartbin = new Smartbin();

$smartbin->storeImageAndOutcome(null, null);

//Decoding input json - How does it works?
//~ $sent_data = json_decode(file_get_contents('php://input'), true);
$sent_data = $_POST;

if ( !empty($_GET) && array_key_exists("action", $_GET) && $_GET["action"] == "ADD_BIN") {
	if ( ! array_key_exists("name", $_GET) ) {
		$_GET["name"] = "Unknown";
	}
	//Add new bin
	$id = $smartbin->addBin($_GET);
	echo "$id";
}
//Storing data
if ( !empty($sent_data) && array_key_exists("action", $sent_data) && $sent_data["action"] == "ADD_BIN") {
	//NOT USED
} else if ( !empty($sent_data) ) {
	//Add new data
	$smartbin->addBinData($sent_data);
	echo "Data added";
} else {
	if (!empty($_POST)) {
		echo "Invalid POST";
		var_dump($_POST);
	} else {
		//~ echo "This is a GET request, try a POST";
	}
}
?>
