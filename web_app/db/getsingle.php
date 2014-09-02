<?php

	/* 	Retrieve a single nuclei that has the closest centroid to the specified point
		Return as a json object
	*/

	/* 
		Get the bounding box and slide name passed by the ajax call
	*/
	$cellX = intval($_POST['cellX']);
	$cellY = intval($_POST['cellY']);
	$slide = $_POST['slide'];


	$boxLeft = $cellX - 10;
	$boxRight = $cellX + 10;
 	$boxTop = $cellY - 10;
	$boxBottom = $cellY + 10;

	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}

	$sql = 'SELECT boundary, id, (pow(centroid_x -'.$cellX.',2) + pow(centroid_y -'.$cellY.',2)) AS dist '.
		   'FROM boundaries '.
		   'WHERE slide="'.$slide.'" AND centroid_x BETWEEN '.$boxLeft.' AND '.$boxRight.
		   ' AND centroid_y BETWEEN '.$boxTop.' AND '.$boxBottom.
		   ' ORDER BY dist LIMIT 1';

	if( $result = mysqli_query($dbConn, $sql) ) {

//		$jsonData = array();
//		while( $array = mysqli_fetch_row($result) ) {
//			$jsonData[] = $array;
//		}
		$jsonData = mysqli_fetch_row($result);	
		mysqli_free_result($result);
		echo json_encode($jsonData);
	}
	mysqli_close($dbConn);

?>

