<?php
require_once "aws/aws-autoloader.php";

Class Rekognition {
	
	private $rekognitionClient;
		
	public function __construct() {
		// Create credentials
		$secret = parse_ini_file("../secret.ini",true);
		$credentials = new Aws\Credentials\Credentials(
			$secret['aws']['key'],
			$secret['aws']['secret']
		);
		
		// Create client options
		$options = [
			'region'            => 'eu-west-1',
			'version'           => '2016-06-27',
			'signature_version' => 'v4',
			'credentials' => $credentials
		];
        // Instantiate an Amazon Rekognition client. -> Image Analysis
        $this->rekognitionClient = new Aws\Rekognition\RekognitionClient($options);
    }

	/* 
	 * Detect all labels from the images
	 * $image must be a string of byte -> Use: file_get_contents($url|$path)
	 */
	public function detectImageLabel($image) {
		//Send request to get Labels
		$result = $this->rekognitionClient->detectLabels([
			'Image' => [ 
				'Bytes' => $image,
			],
		]);
		$labels = $result->get("Labels");		
		return $labels;
	}
	
}
?> 
