<?php
	require 'hostspecs.php';
	session_start();
	
	
	$submit_data =  array( "command" => "submit", 
	  			 	       "uid" => $_SESSION['uid'],
	  			 	       "samples" => $_POST['samples'],
	  			 	       "iteration" => $_POST['iteration'] );
	  			 	       
	$submit_data = json_encode($submit_data, JSON_NUMERIC_CHECK);
	
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
	
	socket_write($socket, $submit_data, strlen($submit_data));
	$response = socket_read($socket, 10);
	socket_close($socket);
	
	$response = strlen($submit_data);
	echo json_encode($response);
?>
