<?php 
require "src/ImageAnalyzer.php"; 
?>

<!DOCTYPE html>
<html>
	<body>
<?php
//~ var_dump($_POST);

//~ if ( !empty($_FILES) ) {
	//~ echo "ricevuto _FILES";
	//~ var_dump($_FILES);
//~ }

if ( !empty($_FILES) && !empty($_POST["bin_id"]) ) {
	
	$bin_id = $_POST["bin_id"];
	
	try {
		//Create Analyzer
		$analyzer = new ImageAnalyzer($bin_id);
		//Analyze image
		$outcome = $analyzer->analyzeImage($_FILES);
		echo "%$outcome%";
	} catch(Exception $e) {
		echo "<p>ERROR: ".$e->getMessage()."</p>";
	}
	
} else {
	//Nothing send - Manula upload
	echo "<p>No image or bin id sent</p>";
	echo <<<EOT
	<div class='upload_section'><form method="post" enctype="multipart/form-data">
		<p>Select image to upload:</p>
		<p>Bin ID: <input type="number" name="bin_id" value=100001></input></p>
		<p><input type="file" name="bin_image" id="bin_image"></p>
		<p><input type="submit" value="Upload Image" name="submit"></p>
	</form></div>
EOT;
}
?>

	</body>
</html>
