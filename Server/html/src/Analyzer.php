<?php
require_once "Database.php";

Class Analyzer {
	
	/*
	 * A list of useless object to remove from the prediction
	 */
	private static $uselessLabel = array("Human", "People", "Person", "Wall", "Furniture", "Tabletop", "Screen",
		"Monitor", "Electronics", "Computer", "Label", "Hardware", "Computer Hardware", "Tile", "Lighting", "File", 
		"Webpage", "Phone", "Laptop", "Pc", "Flooring", "Tablet Computer", "Building", "Bowl", "Console", "Diagram",
		"Camera", "Video Camera", "Architecture", "Face", "LCD Screen", "Photo Booth", "Art", "Finger");
	
	private static $plastic = array("Bottle", "Water Bottle", "Mineral Water", "Cup", "Drink", "Coffee Cup");
	
	private static $aluminium = array("Can", "Aluminium", "Tin", "Coke", "Soda", "Drink", "Coke", "Canned Goods");
	
	private static $paper = array("Paper", "Poster", "Diagram", "White Board", "Brochure", "Menu", "Origami", "Cardboard", "Carton");
	
	/*
	 * The database connection
	 */
	private $db_connection;
	
	public function __construct() {
		$this->db_connection = Database::getInstance()->getConnection();
	}
	
	
	public function getExpectedBin($objectArray) {
		// Default bin
		$result = array( "PLASTIC" => 0, "UNSORTED" => 0.5, "PAPER" => 0, "ALUMINIUM" => 0);
		
		// Remove useless object
		$objectArray = array_filter( $objectArray, function($obj) {
			if ( in_array($obj["Name"], self::$uselessLabel) ) {
				return false;
			} else {
				return true;
			}
		});
		
		//Retrieve material
		foreach($objectArray as $obj) {
			$name = $obj["Name"];
			$conf = $obj["Confidence"];
			if (in_array($name, self::$plastic)) {
				$result["PLASTIC"] += $conf;
			} 
			if (in_array($name, self::$aluminium)) {
				$result["ALUMINIUM"] += $conf;
			}
			if (in_array($name, self::$paper)) {
				$result["PAPER"] += $conf;
			}
		}
		
		//Sort result
		uasort($result, function($a, $b) {
			if ( $a == $b ) {
				return 0;
			}
			return ($a > $b) ? -1 : 1;
		});
		
		//~ var_dump($result);
		
		//~ echo "<p>".key($result)."</p>";
		//~ foreach () {
			
		//~ }
		
		return key($result);
		
	}
	
	/*
	 * For each material we have the probability to be a certain 
	 */
	private function getPredictions($objectArray) {
		$materials = array();
		// Query - Get the probability of an object to be of a certain material
		$sql = "SELECT i.real_bin, SUM(l.accuracy), COUNT(i.real_bin)
				FROM image_labels AS l 
				JOIN bin_images as i ON i.image_id = l.image_id
				WHERE l.image_label = ? AND NOT i.real_bin = 'TODO'
				GROUP BY i.real_bin";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("s", $name);
		
		// Iterate over all objects
		foreach($objectArray as $obj) {
			// Extract the object
			$name = $obj['Name'];
			$accuracy = round($obj['Confidence'], 2);
			// Retrieve old data
			$stmt->execute();
			$stmt->store_result();
			$stmt->bind_result(
				$type, 
				$sum, 
				$count
			);
			$stmt->fetch();
			$sum = $accuracy * $sum;
			
			//If I don't know the type - Put in unsorted
			if ( empty(trim($type)) ) {
				$type = "UNSORTED";
			}
			
			//~ echo "\n<p>Trovato: $type [$sum - $count volte]</p>";
			
			// Add key to the array if not exists
			if ( ! array_key_exists($type, $materials) ) {
				//~ echo "\n<p>Aggiungo $type all'array</p>";
				$materials[$type] = array("sum" => 0, "count" => 0, "avg" => 0);
			}
			// Add data to the array
			$materials[$type]["sum"] += $sum;
			$materials[$type]["count"] += $count;
		}
		
		// Calculate AVG
		foreach ($materials as $type => $material) {
			// Only if there is more than 1 object
			if ($material["count"] > 1) {
				$materials[$type]["avg"] = $material["sum"] / $material["count"];
			}
		}
		
		
		//~ echo "<p>Array disordinato: ";
		//~ print_r($materials);
		//~ echo "</p>";
		
		//Order by AVG
		uasort($materials, function($a, $b) {
			if ( $a["avg"] == $b["avg"] ) {
				return 0;
			}
			return ($a["avg"] < $b["avg"]) ? -1 : 1;
		});
		
		//~ echo "\n<p>Array ordinato: ";
		//~ print_r($materials);
		//~ echo "</p>";
		
		return $materials;
	}
	
}
?> 
