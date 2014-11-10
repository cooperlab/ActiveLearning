<?php

	$nuclei = $_POST['nuclei'];
	
	
	$dbConn = mysqli_connect("localhost", "guest", "", "nuclei");
	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit();
	}

	for($i = 0; $i < count($nuclei); $i++) {
		$sql = 'SELECT boundary  FROM boundaries WHERE slide="'.$nuclei[$i]['slide'].
				'" AND id='.$nuclei[$i]['id'].' LIMIT 1';

		if( $result = mysqli_query($dbConn, $sql) ) {

			$boundary = mysqli_fetch_row($result);	
			$nuclei[$i]['boundary'] = $boundary;

			mysqli_free_result($result);
		}
	}
			
	mysqli_close($dbConn);

	echo json_encode($nuclei);
?>

