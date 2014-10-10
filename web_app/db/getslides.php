<?php

	/* 	Retrieve a list of segmented slides from the data base.
		Return as a json object
	*/
	$dataset = $_POST['dataset'];
	
	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}
	
	/* 
		May want to change this if the joins cause a slowdown
	*/	
	$sql = 'SELECT s.name, s.pyramid_path FROM slides s JOIN dataset_slides d ON s.id=d.slide_id 
									JOIN datasets t ON d.dataset_id=t.id 
									WHERE t.name="'.$dataset.'"';

	if( $result = mysqli_query($dbConn, $sql) ) {

		$slideNames = array();
		$paths = array();
		while( $array = mysqli_fetch_row($result) ) {
			$slideNames[] = $array[0];
			$paths[] = $array[1];
		}
		
		$slideData = array("slides" => $slideNames, "paths" => $paths);
		mysqli_free_result($result);

	}
	mysqli_close($dbConn);

	echo json_encode($slideData);
?>


