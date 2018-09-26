<?php
require_once "aws/aws-autoloader.php";

Class S3Client {
	
	private $bucket;
	
	private $s3Client;
	    
    	public function __construct() {
		// Load credentials
		$secret = parse_ini_file("../secret.ini",true);
		$credentials = new Aws\Credentials\Credentials(
			$secret['aws']['key'],
			$secret['aws']['secret']
		);
		$this->bucket = $secret['aws']['bucket'];
		
		// Create client options
		$options = [
			'region'            => 'eu-west-1',
			'version'           => '2006-03-01',
			'signature_version' => 'v4',
			'credentials' => $credentials
		];
		
		// Instantiate an Amazon S3 client.
		$this->s3Client = new Aws\S3\S3Client($options);
    }
    
    private function getListObjectBucket() {
		$result = $this->s3Client->listObjects([
			'Bucket' => $this->bucket
		]);
		
		//TODO Parse the result of the bucket
		
		return $result;
	} 

	/* 
	 * Upload the image: $image to the bucket and gives it the name: $fileName
	 */
	private function uploadImageToBucket($image, $fileName) {
		$result = $this->s3Client->putObject([
			'Body' => $image,
			'Bucket' => $this->bucket,
			'Key' => $fileName,
			'ACL'    => 'public-read',
			'ContentType' => 'image/jpeg',
		]);
		
		return $result;
	}
	
	/* 
	 * Removes the image named: $fileName from the bucket
	 */
	private function removeImageFromBucket($fileName) {
		$result = $this->s3Client->deleteObject([
			'Bucket' => $this->bucket,
			'Key' => $fileName,
		]);
		
		return true;
	}

}
?> 
