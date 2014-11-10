<?php

	require 'hostspecs.php';
	require '../db/logging.php';
	session_start();
	
	// Send finalize command to al server to save the training set and
	// get the pertinent info to save in the database
	//
	$end_data =  array( "command" => "finalize", 
			 	   "uid" => $_SESSION['uid']);
	$end_data = json_encode($end_data);
			 	   
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
	
	socket_write($socket, $end_data, strlen($end_data));	
	$response = socket_read($socket, 8192);
	socket_close($socket);

	$response = json_decode($response, true);


	$dbConn = guestConnect();
	// Get dataset ID
	if( $result = mysqli_query($dbConn, 'SELECT id from datasets where name="'.$_SESSION['dataset'].'"') ) {

		$array = mysqli_fetch_row($result);
		$datasetId = $array[0];
		mysqli_free_result($result);
	}
	
	
	// Add classifier to database
	//
	$sql = 'INSERT INTO training_sets (name, type, dataset_id, iterations, filename)';
	$sql = $sql.' VALUES("'.$_SESSION['classifier'].'", "binary", '.$datasetId;
	$sql = $sql.', '.$response['iterations'].', "'.$response['filename'].'")';
	
	$status = mysqli_query($dbConn, $sql);
	$trainingSetId = $dbConn->insert_id;
	if( $status == FALSE ) {
		log_error("Unable to insert classifier into database ".mysqli_connect_error());
		exit();
	}
	
	// Add classes to the database
	//		!!!! Assuming binary for now !!!!
	//
	$sql = 'INSERT INTO classes (name, training_set_id, color, label)';
	$sql = $sql.'VALUES("'.$_SESSION['posClass'].'",'.$trainingSetId.', "green", 1)';
	$status = mysqli_query($dbConn, $sql);
	$posId = $dbConn->insert_id;
	if( $status == FALSE ) {
		log_error("Unable to insert pos class into database ".mysqli_error($dbConn));
		exit();
	}

	$sql = 'INSERT INTO classes (name, training_set_id, color, label)';
	$sql = $sql.'VALUES("'.$_SESSION['negClass'].'",'.$trainingSetId.', "red", -1)';
	$status = mysqli_query($dbConn, $sql);
	$negId = $dbConn->insert_id;
	if( $status == FALSE ) {
		log_error("Unable to insert neg class into database ".mysqli_error($dbConn) );
		exit();
	}

	// Add samples to the database
	//
	for($i = 0, $len = count($response['samples']); $i < $len; ++$i) {

		if( $response['samples'][$i]['label'] === 1 ) {
			$classId = $posId;
		} else {
			$classId = $negId;
		}

		$sql = 'INSERT INTO training_objs (training_set_id, cell_id, iteration, class_id)';
		$sql = $sql.'VALUES('.$trainingSetId.','.$response['samples'][$i]['id'].', '.$response['samples'][$i]['iteration'].','.$classId.')';
		mysqli_query($dbConn, $sql);
	}
	mysqli_close($dbConn);
	
	write_log("INFO", "Session ".$_SESSION['classifier']." finished, Training set saved to: ".$response['filename']);
	
	// TODO - Add a download of the training set file.
	echo "PASS";
	
	// Cleanup session variables
	//
	session_destroy();	
?>
