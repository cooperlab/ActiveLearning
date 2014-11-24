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
	}

	$sessionInfo['alServer'] = $host;
	$sessionInfo['alServerPort'] = $port;
	$sessionInfo['IIPServer'] = $IIPServer;
	
	
//	$sessionInfo['slideHost'] = $SlideHost;
//	$sessionInfo['slidePath'] = $SlidePath;
	
	echo json_encode($sessionInfo);
?>
