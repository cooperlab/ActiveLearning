<?php

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



	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}

	$sql = 'SELECT boundary, id from boundaries where slide="'.$slide.'" AND centroid_x BETWEEN '.$left.' AND '.$right.' AND centroid_y BETWEEN '.$top.' AND '.$bottom.' LIMIT 15000';
 

	if( $result = mysqli_query($dbConn, $sql) ) {

		$jsonData = array();
		while( $array = mysqli_fetch_row($result) ) {
			$jsonData[] = $array;
		}
		
		mysqli_free_result($result);

		echo json_encode($jsonData);
	}

	mysqli_close($dbConn);

?>

