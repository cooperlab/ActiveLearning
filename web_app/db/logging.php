<?php

function write_log($type, $message)
{
	$dbConn = mysqli_connect("localhost", "logger", "BMIIsGoodz!", "nuclei");
	if( !$dbConn ) {
		return array( status => false, message => 'Unable to connect to database');
	}

	if( $message == '' ) {
		return array( status => false, message => 'Empty message not valid');
	}
	
	$remoteAddr = $_SERVER['REMOTE_ADDR'];
	if( $remoteAddr == '' ) {
		$remoteAddr = "UNKNOWN";
	} else {
		$remoteAddr = gethostbyaddr($remoteAddr);
		if( $remoteAddr == '' ) {
			$remoteAddr = $_SERVER['REMOTE_ADDR'];
		}
	}
		

	$message = mysqli_real_escape_string($dbConn, $message);
	$remoteAddr = mysqli_real_escape_string($dbConn, $remoteAddr);
	$type = mysqli_real_escape_string($dbConn, $type);
	 
	$sql = "INSERT INTO logs (remote_addr, type, message) VALUES('$remoteAddr','$type','$message')";	
	$status = mysqli_query($dbConn, $sql);

	if( !$status ) {
		return array( status => false, message => "Unable to add to log");
	}
	
	mysqli_close($dbConn);
	return array( status => true);
}






function log_error($message) {
	write_log("ERROR", $message);
}



?>
