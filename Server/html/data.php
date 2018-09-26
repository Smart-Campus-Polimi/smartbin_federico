<?php
require_once "src/PlotMap.php";

//~ header('Content-Type: application/json');

$useFakeData = true;
$mapData = new PlotMap($useFakeData);

//Extract data
$bins = $mapData->getData("20.0");

//Plot json
echo json_encode($bins);
?>
