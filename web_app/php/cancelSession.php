<?php
	require 'hostspecs.php';	// $host & $port defined here
	require '../db/logging.php';
	
	session_start();
	
	$end_data =  array( "command" => "end", 
			 	   "uid" => $_SESSION['uid']);
	$end_data = json_encode($end_data);
			 	   
	$addr = gethostbyname($host);
	set_time_limit(0);
	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		log_error("socket_create failed:  ". socket_strerror(socket_last_error()));
		exit();
	}
	
	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		log_error("socket_connect failed: ".socket_strerror(socket_last_error()));
		exit();
	}
	
	socket_write($socket, $end_data, strlen($end_data));	
	$response = socket_read($socket, 10);
	socket_close($socket);

	write_log("INFO", "Session '".$_SESSION['classifier']."' canceled");

	// Cleanup session variables
	//
	session_destroy();	
?>
