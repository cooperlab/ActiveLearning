<?php

	session_start();
	
	$sessionInfo = array();
	
	if( isset($_SESSION['uid']) ) {
		$sessionInfo[] = $_SESSION['uid'];
		$sessionInfo[] = $_SESSION['classifier'];
		$sessionInfo[] = $_SESSION['posClass'];
		$sessionInfo[] = $_SESSION['negClass'];
		$sessionInfo[] = $_SESSION['al_server'];
		$sessionInfo[] = $_SESSION['al_server_port'];
	}
	
	echo json_encode($sessionInfo);
?>
