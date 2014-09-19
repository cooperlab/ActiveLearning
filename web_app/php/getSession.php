<?php

	session_start();
	
	$sessionInfo = array();
	
	if( isset($_SESSION['uid']) ) {
		$sessionInfo['uid'] = $_SESSION['uid'];
		$sessionInfo['classname'] = $_SESSION['classifier'];
		$sessionInfo['posClass'] = $_SESSION['posClass'];
		$sessionInfo['negClass'] = $_SESSION['negClass'];
		$sessionInfo['alServer'] = $_SESSION['al_server'];
		$sessionInfo['alServerPort'] = $_SESSION['al_server_port'];
		$sessionInfo['dataset'] = $_SESSION['dataset'];		
	}
	
	echo json_encode($sessionInfo);
?>
