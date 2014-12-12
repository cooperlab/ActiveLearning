<?php

	require 'hostspecs.php';
	session_start();
	
	
	$sessionInfo = array();
	
	if( isset($_SESSION['uid']) ) {
		$sessionInfo['uid'] = $_SESSION['uid'];
		$sessionInfo['classname'] = $_SESSION['classifier'];
		$sessionInfo['posClass'] = $_SESSION['posClass'];
		$sessionInfo['negClass'] = $_SESSION['negClass'];
		$sessionInfo['dataset'] = $_SESSION['dataset'];	
	} else {
		$sessionInfo['uid'] = null;
		$sessionInfo['dataset'] = null;
	}

	$sessionInfo['alServer'] = $host;
	$sessionInfo['alServerPort'] = $port;
	$sessionInfo['IIPServer'] = $IIPServer;
	
	echo json_encode($sessionInfo);
?>
