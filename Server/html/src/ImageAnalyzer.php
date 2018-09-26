<?php
require_once "Database.php";
require_once "Rekognition.php"; 
require_once "Analyzer.php"; 

Class ImageAnalyzer {
	
	//Db connection
	private $db_connection;
	
	//Rekognizer class
	private $rekognizer;
	
	private $bin_id;

	//Create a TrashImage object with a valid DB Connection
	public function __construct($bin_id) {
        //Create DB connection
        $this->db_connection = Database::getInstance()->getConnection();
        //Use AWS Rekognition
        $this->rekognizer = new Rekognition();
        //The bin id that created the analyzer
        $this->bin_id = $bin_id;
	}
	
	/*
	 * Analyze an image
	 */
	public function analyzeImage($image) {
		$start = microtime(true);
		//Control if the image has any problem
		$this->checkImage($image);
		$end = microtime(true);
	echo "<p>Time: ".round((($end-$start)*1000))." ms</p>";
		//Get image labels
		$labels = $this->getImageLabels($image);
		$end = microtime(true);
	echo "<p>Time: ".round((($end-$start)*1000))." ms</p>";
		//~ $this->debugLabels($labels);
		
		//Get appropiate bin
		$expectedBin = $this->getExpectedBin($labels);
		$end = microtime(true);
	echo "<p>Time: ".round((($end-$start)*1000))." ms</p>";
		//Store the image in the DB
		try {
			$imagePath = $this->moveImage($image);
			$this->storeImage($imagePath, $expectedBin, $labels);
		} catch(Exception $e) {
			//TODO Store this problem
		}
		
		//Return the expected bin
		return $expectedBin;
	}
	
	/*
	 * Control if it is a valid image
	 */
	private function checkImage($image) {
		//No image received
		if (empty($image["bin_image"]["tmp_name"])) {
			throw new Exception("CANNOT UPLOAD - Empty name!");
		}
		//Control that is an image
		$check = getimagesize($image["bin_image"]["tmp_name"]);
		if($check === false) {
			throw new Exception("CANNOT UPLOAD - File is not an image.");
		}
	}
	
	/*
	 * Retrieve the image labels
	 */
	private function getImageLabels($image) {
		$imageName = $image["bin_image"]["name"];
		$imageContent = file_get_contents($image["bin_image"]["tmp_name"]);
		
		//Using wrap classes for AWS Rekognition 
		$labels = $this->rekognizer->detectImageLabel($imageContent);
		
		return $labels;
	}
		
	/*
	 * Return the expected bin from a list of Labels
	 */
	private function getExpectedBin($labels) {
		$analyzer = new Analyzer();
		return $analyzer->getExpectedBin($labels);
	}
	
	/*
	 * Move the image in the appropiate folder and return it's path
	 */
	private function moveImage($image) {
		//Create the file name of the stored image
		$timestamp = date_timestamp_get(date_create());
		$path = "../temp/";
		$target_file = $path . $timestamp . "_" . basename($image["bin_image"]["name"]);
		
		$imageMoved = move_uploaded_file($image["bin_image"]["tmp_name"], $target_file);
		
		if ($imageMoved) {
			return $target_file;
		} else {
			throw new Exception("Cannot move the image - Unexpected error");
		}
	}
	
	private function debugLabels($labels) {
		foreach($labels as $label) {
			$label_name = $label['Name'];
			$label_confidence = round($label['Confidence'], 2);
			echo "<p>$label_name [$label_confidence]</p>\n";
		}
		echo "<br>\n";
	}
	
	/*
	 * Store the image position and the given label in the DB
	 */
	 private function storeImage($imagePath, $expectedBin, $labels) {
		//Store image
		$sql = "INSERT INTO bin_images (bin_id, image_name, expected_bin) 
				VALUES (?, ?, ?)";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("iss", $this->bin_id, $imagePath, $expectedBin);
		$stmt->execute();
		//Retrieve image id
		$image_id = $this->db_connection->insert_id;
		$stmt->close();
		
		
		//Store associated labels
		if (!empty($labels)) {
			$sql = "INSERT INTO image_labels (image_id, image_label, accuracy) 
				VALUES (?, ?, ?)";
			$stmt = $this->db_connection->prepare($sql);
			$stmt->bind_param("isd", $image_id, $label_name, $label_confidence);
			foreach($labels as $label) {
				$label_name = $label['Name'];
				$label_confidence = round($label['Confidence'], 2);
				$stmt->execute();
			}
			$stmt->close();
		}
	}
	
}
?> 
