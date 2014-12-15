<?php
	require 'logging.php';		// Also includes connect.php
	require '../php/hostspecs.php';

	$colors = array('deeppink', 'lime');
	

	// 	Retrieve a list of nuclei boundaries within the given rectangle
	//	Return as an array of JSON objects, where each object is ordered:
	//
	//		0 - boundary
	//		1 - Id
	//		2 - color

	
	//	Get the bounding box and slide name passed by the ajax call
	//
	$left = intval($_POST['left']);
	$right = intval($_POST['right']);
	$top = intval($_POST['top']);
	$bottom = intval($_POST['bottom']);
	$slide = $_POST['slide'];
	$trainSet = $_POST['trainset'];


	write_log("DEBUG", "trainset: ".$trainSet);
	
	// Get labels for the objects within the viewport
	if( $trainSet != "none" ) {

		$dataSet = $_POST['dataset'];

		write_log("DEBUG", "Applying classifier ".$trainSet." to ".$dataSet);

		// Get the dataset file from the database
		//
		$dbConn = guestConnect();
		$sql = 'SELECT features_file FROM datasets WHERE name="'.$dataSet.'"';

		if( $result = mysqli_query($dbConn, $sql) ) {
	
			$featureFile = mysqli_fetch_row($result);			
			mysqli_free_result($result);
		}
		mysqli_close($dbConn);

		write_log("DEBUG", "Training set filename: ".$featureFile[0]);
		
		// Send command to al server
		$cmd =  array( "command" => "apply", 
					   "uid" => $_POST['uid'],
				 	   "dataset" => $featureFile[0],
				 	   "trainingset" => $trainSet,
				 	   "slide" => $slide,
				 	   "xMin" => $left,
				 	   "xMax" => $right,
				 	   "yMin" => $top,
				 	   "yMax" => $bottom
				 	   );
				 	   
		$cmd = json_encode($cmd);
		
		$addr = gethostbyname($host);
		set_time_limit(0);
	
		$socket = socket_create(AF_INET, SOCK_STREAM, 0);
		if( $socket === false ) {
			log_error("socket_create failed:  ". socket_strerror(socket_last_error()));
		}
	
		$result = socket_connect($socket, $addr, $port);
		if( !$result ) {
			log_error("socket_connect failed: ".socket_strerror(socket_last_error()));
		}
	
		socket_write($socket, $cmd, strlen($cmd));	
		
		// Get classification results, The al server closes the connection
		// after sending data, so it's safe to loop on the result of 
		// socket_read 
		//
		$classification = socket_read($socket, 8192);
		$additional = socket_read($socket, 8192);
		while( $additional != false ) {
			$classification = $classification.$additional;
			$additional = socket_read($socket, 8192);
		}
		socket_close($socket);
		$classification = json_decode($classification, true);
	}


	$dbConn = boundaryConnect();
	$sql = 'SELECT boundary, id, centroid_x, centroid_y from boundaries where slide="'.$slide.'" AND centroid_x BETWEEN '.$left.' AND '.$right.' AND centroid_y BETWEEN '.$top.' AND '.$bottom.' LIMIT 15000';
  	
	if( $result = mysqli_query($dbConn, $sql) ) {

		$jsonData = array();		
		while( $array = mysqli_fetch_row($result) ) {
			$obj = array();
			
			$obj[] = $array[0];
			$obj[] = $array[1];
					
			if( $trainSet != "none" ) {
				$tag = $array[2]."_".$array[3];
				$obj[] = $colors[$classification[$tag]];				
			} else {
				$obj[] = "aqua";
			}
			$jsonData[] = $obj;
		}
		mysqli_free_result($result);
	}
	mysqli_close($dbConn);
	
	echo json_encode($jsonData);

?>

