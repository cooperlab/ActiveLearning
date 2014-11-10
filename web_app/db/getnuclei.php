<?php
	require 'logging.php';		// Also includes connect.php
	require '../php/hostspecs.php';



	/* 	Retrieve a list of nuclei boundaries within the given rectangle
		Return as a json object
	*/

	/* 
		Get the bounding box and slide name passed by the ajax call
	*/
	$left = intval($_POST['left']);
	$right = intval($_POST['right']);
	$top = intval($_POST['top']);
	$bottom = intval($_POST['bottom']);
	$slide = $_POST['slide'];

	$dataSet = $_POST['dataset'];
	$trainSet = $_POST['trainset'];
	
/*	write_log("INFO", "Classifying ".$dataSet.", ".$trainSet);
	if( $dataSet === "sox2singleTile.h5" ) {
		
	
		$cmd =  array( "command" => "apply", 
				 	   "dataset" => "sox2singleTile.h5",
				 	   "trainingset" => $trainSet
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
		$classification = socket_read($socket, 8192);
		socket_close($socket);
	
		$classification = json_decode($classification, true);
		write_log("INFO", "Classification Done");
	}
*/

	$dbConn = guestConnect();
	$sql = 'SELECT boundary, id from boundaries where slide="'.$slide.'" AND centroid_x BETWEEN '.$left.' AND '.$right.' AND centroid_y BETWEEN '.$top.' AND '.$bottom.' LIMIT 15000';
  	
	if( $result = mysqli_query($dbConn, $sql) ) {

		$jsonData = array();
		while( $array = mysqli_fetch_row($result) ) {
			$array[] = "";
			$jsonData[] = $array;
		}
		
		mysqli_free_result($result);

	}
	mysqli_close($dbConn);
	
	echo json_encode($jsonData);

?>

