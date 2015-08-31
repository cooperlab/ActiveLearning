<?php

//
//	Copyright (c) 2014-2015, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list 
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//
//

	require 'hostspecs.php';	// $host & $port defined here
	require '../db/logging.php';
	session_start();

	
	$visCmd =  array( "command" => "visualize",
					  "strata" => intval($_POST['strata']),
					  "groups" => intval($_POST['groups']),
	  		 	      "uid" => $_SESSION['uid'] );

	$visCmd = json_encode($visCmd);
	
	$addr = gethostbyname($host);
	set_time_limit(0);	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);

	if( $socket === false ) {
		log_error("[GetVis] socket_create failed: ".socket_strerror(socket_last_error()));
		exit();		
	}
	
	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		log_error("[GetVis] socket_connect failed: ".socket_strerror(socket_last_error()));
		exit();
	}
	
	socket_write($socket, $visCmd, strlen($visCmd));
	
	// FIXME! - Should loop until no data is left on the socket
	$response = socket_read($socket, 8192);
	$additional = socket_read($socket, 8192);
	socket_close($socket);
	$response = $response.$additional;
	
	// Now get the max X & Y from the database for the slide of the samples
	//
	$dbConn = guestConnect();
	$response = json_decode($response, true);
	
	for($i = 0, $len = count($response); $i < $len; ++$i) {
	
		$response[$i]['score'] = round($response[$i]['score'], 4);
		$response[$i]['centX'] = round($response[$i]['centX'], 1);
		$response[$i]['centY'] = round($response[$i]['centY'], 1);
		
		// get slide dimensions for the nuclei
		//
		$sql = 'SELECT x_size, y_size, scale, pyramid_path FROM slides WHERE name="'.$response[$i]['slide'].'"';
		if( $result = mysqli_query($dbConn, $sql) ) {
			$row = mysqli_fetch_row($result);
			
			$response[$i]['maxX'] = intval($row[0]);
			$response[$i]['maxY'] = intval($row[1]);
			$response[$i]['scale'] = intval($row[2]);
			$response[$i]['path'] = $row[3];
			
			mysqli_free_result($result);
		} 
		
		// Get database id for the nuclei
		//
		$sql = 'SELECT id FROM boundaries WHERE slide="'.$response[$i]['slide'].'"';
		$sql = $sql.' AND centroid_x='.$response[$i]['centX'].' and centroid_y='.$response[$i]['centY'];

		if( $result = mysqli_query($dbConn, $sql) ) {
			$row = mysqli_fetch_row($result);
			
			$response[$i]['id'] = intval($row[0]);
			mysqli_free_result($result);
		} 		
	}	
	mysqli_close($dbConn);

	$response = json_encode($response);
	
	echo $response;

?>
