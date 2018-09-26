<?php
require_once "src/Smartbin.php";

class PlotMap {
	
	/*
	 * The array of data
	 */
	private $data;
	
	/*
	 * DB connection
	 */
	private $db_connection;
	
	/*
	 * Smartbin object
	 */
	private $smartbin;
	
	/*
	 * Create an object that can plot maps
	 */
	public function __construct($useFake = true) {
		//Create DB connection
		$this->db_connection = Database::getInstance()->getConnection();
		
		//Create Smartbin object
		$this->smartbin = new Smartbin();
		
		//Instatiate data array
		$this->data = array();
		if ($useFake) {
			$this->data = array(
				["full", false, 168, 297, "123"],
				["full", false, 389, 320, "123"],
				["full", true, 423, 320, "123"],
				["half", false, 500, 315, "123"],
				["empty", false, 536, 315, "123"],
				["empty", true, 642, 315, "123"],

				//Bin x3
				["half", false, 267, 50, "123"],
				["empty", true, 284, 50, "123"],
				["half", false, 301, 50, "123"]
			);
		}
	}
	
	/*
	 * Get data of all the bins in the building
	 */
	public function getData($building) {
		$res = array();
		$sql = "SELECT xpos, ypos, bin_id 
				FROM bin_list
				WHERE building = ? AND type = 'fill_status'";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("s", $building);
		$stmt->execute();
		$stmt->store_result();
		$result = array();
		$stmt->bind_result(
			$x, 
			$y, 
			$id
		);
		while ($stmt->fetch()) {
			//Elaborate data			
			$status = $this->elaborateHeight($id);
			$tooHeavy = $this->smartbin->heavierThanExpected($id, 0.05);
			$res[] = array($status, $tooHeavy, $x, $y, $id);
		}
			
		$stmt->close();		
		
		//Join the existing data
		$this->data = array_merge($this->data, $res);

		return $this->data;
	}
	
	/*
	 * Return the status of the bin
	 */
	private function elaborateHeight($id) {
		//Get ratio representing the fill status
		$ratio = $this->smartbin->getFillStatus($id);
		
		//Execute comparison
		if ($ratio < 0.33) {
			return "empty";
		} else if ($ratio < 0.66) {
			return "half";
		} else {
			return "full";
		}
	}

}
?>
