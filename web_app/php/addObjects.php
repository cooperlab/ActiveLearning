<?php

	require 'hostspecs.php';
	session_start();
	
	$selectedObjs = array( "command" => "pickerAdd", 
	  			 	    "uid" => $_SESSION['uid'],
	  			 	    "samples" => $_POST['samples']);
	 
	$selectedObjs = json_encode($selectedObjs, JSON_NUMERIC_CHECK);
	
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
	
	socket_write($socket, $selectedObjs, strlen($selectedObjs));
	$response = socket_read($socket, 1024);
	socket_close($socket);
	
	echo $response;

?>
