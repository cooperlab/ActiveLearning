<?php

	require '../db/logging.php';
		
	// Make sure initSession.php was referenced from the main
	// page by checking if 'submit' is empty
	//
	if( empty($_POST['submit']) ) {
		echo "Form was not submitted <br>";
		exit;
	}
	
	// TODO - Add an alert to indicate what was missing. Should
	// be on the main page
	
	
	// Each of the text fields must also be filled in
	//
	if( empty($_POST["classifiername"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}
	
	if( empty($_POST["posName"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}

	if( empty($_POST["negName"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}
	
	// Generate a unique id to track this session in the server
	//
	$UID = uniqid("", true);
	
	// Get the dataset file from the database
	//
	$dbConn = guestConnect();
	$sql = 'SELECT features_file FROM datasets WHERE name="'.$_POST["dataset"].'"';

	if( $result = mysqli_query($dbConn, $sql) ) {

		$featureFile = mysqli_fetch_row($result);			
		mysqli_free_result($result);
	}
	mysqli_close($dbConn);

	
	// Send init command to AL server
	//	
	$init_data =  array( "command" => "init", 
				   "name" => $_POST["classifiername"],
			 	   "dataset" => $_POST["dataset"],
			 	   "features" => $featureFile[0],
			 	   "uid" => $UID);

	$init_data = json_encode($init_data);

	require 'hostspecs.php';
	//
	//	$host and $port are defined in hostspecs.php
	//			
	$addr = gethostbyname($host);
	set_time_limit(0);
	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		echo "socket_create failed:  ". socket_strerror(socket_last_error()) . "<br>";
	}
	
	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		echo "socket_connect failed: ".socket_strerror(socket_last_error()) . "<br>";
	}
	
	socket_write($socket, $init_data, strlen($init_data));	
	$response = socket_read($socket, 10);
	socket_close($socket);
	
	if( strcmp($response, "PASS") == 0 ) { 
		write_log("INFO", "Session '".$_POST["classifiername"]."' started");
		
		session_start();
		$_SESSION['uid'] = $UID;
		$_SESSION['posClass'] = $_POST["posName"];
		$_SESSION['negClass'] = $_POST["negName"];
		$_SESSION['classifier'] = $_POST["classifiername"];
		$_SESSION['iteration'] = 0;
		$_SESSION['dataset'] = $_POST["dataset"];
		header("Location: ../prime.html");
	} else {
	
		echo "Unable to init session<br>";
	}
?>
