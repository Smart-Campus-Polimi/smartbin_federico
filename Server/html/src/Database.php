<?php
Class Database {
	
	private $connection;
	
	/*
	 * The private static variable that hold the instance of the class
	 */
	private static $instance = null;
	
	/*
	 * The private builder of the class
	 */
	private function __construct() {
		// Load db credential
        $secret = parse_ini_file("/var/www/secret.ini",true);
        // Create the conneection
        $this->connection = mysqli_connect(
			"localhost", 
			$secret["db"]["username"], 
			$secret["db"]["password"], 
			$secret["db"]["db_name"]
		);

		//~ var_dump($secret);
		
		//Error if DB is not available
		if ( ! $this->connection ) {
			throw new Exception("Cannot connect to DB");
		}
		
    }
    
    /*
     * Returns the created DB connection
     */
    public function getConnection() {
		return $this->connection;
	}
    
	public static function getInstance() {
		if (self::$instance == null) {   
			$c = __CLASS__;
			self::$instance = new $c;
		}
		
		return self::$instance;
	}
    
}
?> 
