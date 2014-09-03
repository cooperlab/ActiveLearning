<?php

	session_start();
	
	// Get list of samples from al server
	//
	$end_data =  array( "command" => "finalize", 
			 	   "uid" => $_SESSION['uid']);
	$end_data = json_encode($end_data);
			 	   
	$port = $_SESSION['al_server_port'];
	$addr = gethostbyname($_SESSION['al_server']);
	set_time_limit(0);
	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		echo "socket_create failed:  ". socket_strerror(socket_last_error()) . "<br>";
	}
	
	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		echo "socket_connect failed: ".socket_strerror(socket_last_error()) . "<br>";
	}
	
	socket_write($socket, $end_data, strlen($end_data));	
	$response = socket_read($socket, 8192);
	socket_close($socket);

	$response = json_decode($response, true);


	// Add classifier to database
	//
	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}
	
	// Get dataset ID
	if( $result = mysqli_query($dbConn, 'SELECT id from datasets where name="'.$_SESSION['dataset'].'"') ) {

		$array = mysqli_fetch_row($result);
		$datasetId = $array[0];
		
		mysqli_free_result($result);
	}

	$sql = 'INSERT INTO training_sets (name, type, dataset_id, iterations)';
	$sql = $sql.' VALUES("'.$_SESSION['dataset'].'", "binary", '.$datasetId.', '.$response['iterations'].')';
	mysqli_query($dbConn, $sql);
	
	mysqli_close($dbConn);
	
	
	// Add classes to the database
	//
	// Add samples to the database
	//

	// Cleanup session variables
	//
	session_destroy();	
?>
