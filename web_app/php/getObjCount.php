<?php

	require 'hostspecs.php'; // declares $host
	session_start();
	
	$statusCmd = array( "command" => "pickerCnt", 
	  			 	    "uid" => $_SESSION['uid']);
	 
	$statusCmd = json_encode($statusCmd, JSON_NUMERIC_CHECK);
	
	$addr = gethostbyname($host);
	set_time_limit(0);
	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		echo "socket_create failed:  ". socket_strerror(socket_last_error()) . "<br>";
	}
	
	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		echo "socket_connect failed: ".socket_strerror(socket_last_error()) . "<br>";
	}
	
	socket_write($socket, $statusCmd, strlen($statusCmd));
	$response = socket_read($socket, 1024);
	socket_close($socket);
	
	echo $response;

?>
