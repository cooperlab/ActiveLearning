<?php

	require 'hostspecs.php';
	require '../db/logging.php';
	
	session_start();
	
	$sel_data =  array( "command" => "select", 
	  			 	     "uid" => $_SESSION['uid'],
	  			 	     "iteration" => $_SESSION['iteration']);

	$sel_data = json_encode($sel_data);

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
	
	socket_write($socket, $sel_data, strlen($sel_data));
	$response = socket_read($socket, (100 * 1024));
	socket_close($socket);

	// Now get the max X & Y from the database for the slide of the samples
	//
	$dbConn = guestConnect();
	$boundaryConn = boundaryConnect();
	$response = json_decode($response, true);
		
	
	for($i = 0, $len = count($response['samples']); $i < $len; ++$i) {
	
		$response['samples'][$i]['centX'] = round($response['samples'][$i]['centX'], 1);
		$response['samples'][$i]['centY'] = round($response['samples'][$i]['centY'], 1);
		
		
		// get slide dimensions for the sample
		$sql = 'SELECT x_size, y_size, pyramid_path, scale FROM slides WHERE name="'.$response['samples'][$i]['slide'].'"';
		if( $result = mysqli_query($dbConn, $sql) ) {
			$row = mysqli_fetch_row($result);
			
			$response['samples'][$i]['maxX'] = intval($row[0]);
			$response['samples'][$i]['maxY'] = intval($row[1]);
			$response['samples'][$i]['path'] = $row[2];
			$response['samples'][$i]['scale'] = intval($row[3]);
			
			mysqli_free_result($result);
		} 
		
		// Get database id for the sample
		$sql = 'SELECT id, boundary FROM boundaries WHERE slide="'.$response['samples'][$i]['slide'].'"';
		$sql = $sql.' AND centroid_x='.$response['samples'][$i]['centX'].' and centroid_y='.$response['samples'][$i]['centY'];

		if( $result = mysqli_query($boundaryConn, $sql) ) {
			$array = mysqli_fetch_row($result);
			
			$response['samples'][$i]['id'] = intval($array[0]);
			$response['samples'][$i]['boundary'] = $array[1];
			mysqli_free_result($result);
		} 		
	}
	$_SESSION['iteration'] = $response['iteration'];
	
	mysqli_close($dbConn);
	mysqli_close($boundaryConn);
	$response = json_encode($response);
	
	echo $response;
?>
