<?php
require_once "Database.php";

Class Smartbin {
	
	/*
	 * The Database connection
	 */
	private $db_connection;
	
	/*
	 * Create a Smartbin object with a valid DB Connection
	 */
	public function __construct() {
        $this->db_connection = Database::getInstance()->getConnection();
    }
	
	/*
	 * Add a bin to the list and return the ID
	 */
	public function addBin($json) {
		$id = -1;
		$sql = "INSERT INTO list (bin_description) 
				VALUES (?)";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("s", $json["name"]);
		if ($stmt->execute()) {
			//Get the id only if the insertion was successful
			$id = $this->db_connection->insert_id;
		}
		$stmt->close();
		//Return the newly auto inserted id
		return $id;
	}
	
	/*
	 * Add the data of a certain bin
	 */
	public function addBinData($json) {
		if ( !is_array($json) ) {
			//throw new Exception("Invalid data sent");
		}
		$sql = "INSERT INTO bin_data (bin_id, size, fill_level, weight) 
				VALUES (?, ?, ?, ?)";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("iiii", $json["id"], $json["size"], $json["fill"], $json["weight"]);
		$stmt->execute();
		$stmt->close();
	}
	
	/*
	 * OLD - NOT WORKING
	 * Gives the last status of the bin
	 */
	public function getLastBinStatus($id) {
		$data = array();
		$sql = "SELECT h.height, h.total_height, weight, time
				FROM bin_height AS h
				JOIN bin_weight AS w ON  
				WHERE bin_id = ? AND time = (
					SELECT MAX(time)
					FROM bin_data
                    WHERE bin_id = ?
				)";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("ii", $id, $id);
		$stmt->execute();
		$stmt->store_result();
		$stmt->bind_result(
			$data['size'], 
			$data['fill_level'], 
			$data['weight'], 
			$data['time']
		);
		$stmt->fetch();
		$stmt->close();		
		return $data;
	}
	
	/*
	 * Retrieve the last height info
	 */
	public function getLastHeight($id) {
		$sql = "SELECT h.height, h.total_height, l.description, h.time 
				FROM bin_height AS h 
				JOIN bin_list AS l ON l.bin_id = h.bin_id 
				WHERE l.bin_id = ? ORDER BY h.time DESC 
				LIMIT 1"; 
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("i", $id);
		$stmt->execute();
		$stmt->store_result();
		$stmt->bind_result(
			$data['height'], 
			$data['total_height'], 
			$data['description'], 
			$data['time']
		);
		$stmt->fetch();
		$stmt->close();		
		return $data;
	}
	
	/*
	 * Retrieve the last weight info
	 */
	public function getLastWeight($id) {
		$sql = "SELECT w.weight, l.description, w.time 
				FROM bin_weight AS w 
				JOIN bin_list AS l ON w.bin_id = l.bin_id 
				WHERE w.bin_id = ? ORDER BY w.time DESC 
				LIMIT 1";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("i", $id);
		$stmt->execute();
		$stmt->store_result();
		$stmt->bind_result(
			$data['weight'], 
			$data['description'], 
			$data['time']
		);
		$stmt->fetch();
		$stmt->close();		
		return $data;
	}
	
	/*
	 * Return the Average height (not related to bin height)
	 */
	public function getAvgHeight($id) {
		$sql = "SELECT AVG(height) 
				FROM bin_height 
				WHERE bin_id = ?
				GROUP BY bin_id 
				LIMIT 1";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("i", $id);
		$stmt->execute();
		$stmt->store_result();
		$stmt->bind_result(
			$avg_height
		);
		$stmt->fetch();
		$stmt->close();		
		return $avg_height;
	}
	
	/*
	 * Return the Average weight
	 */
	public function getAvgWeight($id) {
		$sql = "SELECT AVG(weight) 
				FROM bin_weight 
				WHERE bin_id = ?
				GROUP BY bin_id 
				LIMIT 1";
		$stmt = $this->db_connection->prepare($sql);
		$stmt->bind_param("i", $id);
		$stmt->execute();
		$stmt->store_result();
		$stmt->bind_result(
			$avg_weight
		);
		$stmt->fetch();
		$stmt->close();		
		return $avg_weight;
	}
		
	/*
	 * Return the fill status of the bin in percentage
	 */
	public function getFillStatus($id) {
		$height_data = $this->getLastHeight($id);
		$ratio = 0;
		if ( ! empty($height_data["total_height"]) ) {
			$ratio = $height_data["height"]/$height_data["total_height"];
		}
		return $ratio;
	}
	
	/*
	 * Return the information about abnormal weight of bin
	 * The bin should be heavier than 5% wrt the average related to his history and his fill status
	 */
	public function heavierThanExpected($id, $tolerance =  0.05) {
		
		//Minimum weight = 20g -> A little more than the empty sacket
		$min_weight = 20;
		
		//Get weight
		$weight = ($this->getLastWeight($id))["weight"];
		
		//Get avg weight
		$expected_weight = $this->getExpectedWeight($id);
		$expected_weight = max($min_weight, $expected_weight);
		//~ echo "[$id] Real: $weight - Avg: $expected_weight\n";
		
		//Compare using the tolerance level given
		return $weight > ($expected_weight * (1 + $tolerance));
	}
	
	/*
	 * Obtain the average weight related to the fill level (height)
	 * (AVG(H)*AVG(W))/REAL(H) = EXPECTED(W)
	 */
	private function getExpectedWeight($id) {
		$data = $this->getLastHeight($id);
		$avg_h = $this->getAvgHeight($id);
		$avg_w = $this->getAvgWeight($id);
		if (! empty ($data["total_height"]) && ! empty($real_h) ) {
			$real_h = $data["height"]/$data["total_height"];
			$avg_h = $avg_h / $data["total_height"];
			return ($avg_h*$avg_w)/$real_h;
		} else {
			return 0;
		}
	}
}
?> 
