<?php

	/* 	Retrieve the pyramid path for the specified slide
		Return as a json object
	*/
	$slide = $_POST['slide'];
	
	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}
	
	/* 
		May want to change this if the joins cause a slowdown
	*/	
	$sql = 'SELECT pyramid_path FROM slides WHERE name="'.$slide.'"';
	if( $result = mysqli_query($dbConn, $sql) ) {

		$path = array();
		while( $array = mysqli_fetch_row($result) ) {
			$path[] = $array[0];
		}
		
		mysqli_free_result($result);

	}
	mysqli_close($dbConn);

	echo json_encode($path);
?>

