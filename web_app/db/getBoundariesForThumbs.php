<?php

	require 'connect.php';


	$nuclei = $_POST['nuclei'];	
	$dbConn = boundaryConnect();
	
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

