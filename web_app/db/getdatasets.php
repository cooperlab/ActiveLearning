<?php

	/* 	Retrieve a list of datasets from the data base.
		Return as a json object
	*/

	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}

	if( $result = mysqli_query($dbConn, "SELECT name from datasets") ) {

		$jsonData = array();
		while( $array = mysqli_fetch_row($result) ) {
			$jsonData[] = $array[0];
		}
		
		mysqli_free_result($result);

		echo json_encode($jsonData);
	}

	mysqli_close($dbConn);
?>
