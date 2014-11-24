<?php

	require 'connect.php';

	/* 	Retrieve a single nuclei that has the closest centroid to the specified point
		Return as a json object
	*/

	/* 
		Get the bounding box centroid and slide name passed by the ajax call
	*/
	$cellX = intval($_POST['cellX']);
	$cellY = intval($_POST['cellY']);
	$slide = $_POST['slide'];


	$boxLeft = $cellX - 10;
	$boxRight = $cellX + 10;
 	$boxTop = $cellY - 10;
	$boxBottom = $cellY + 10;

	$dbConn = guestConnect();

	$sql = 'SELECT boundary, id, centroid_x, centroid_y, '.
		   '(pow(centroid_x -'.$cellX.',2) + pow(centroid_y -'.$cellY.',2)) AS dist '.
		   'FROM boundaries '.
		   'WHERE slide="'.$slide.'" AND centroid_x BETWEEN '.$boxLeft.' AND '.$boxRight.
		   ' AND centroid_y BETWEEN '.$boxTop.' AND '.$boxBottom.
		   ' ORDER BY dist LIMIT 1';

	if( $result = mysqli_query($dbConn, $sql) ) {

		$jsonData = mysqli_fetch_row($result);	
		mysqli_free_result($result);
	}		
	
	$sql = 'SELECT x_size, y_size, scale FROM slides WHERE name="'.$slide.'"';
	if( $result = mysqli_query($dbConn, $sql) ) {
		$sizes = mysqli_fetch_row($result);
		mysqli_free_result($result);
	}	
	mysqli_close($dbConn);

	array_push($jsonData, $sizes[0], $sizes[1], $sizes[2]);
	 
	echo json_encode($jsonData);

?>

