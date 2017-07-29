<?php

//
//	Copyright (c) 2014-2017, Emory University
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

	require '../db/accounts.php';
	require 'hostspecs.php';

	$deleteDatasetSel = $_POST['deleteDatasetSel'];
	$application=$_POST['application'];

	/************	Start existing dataset and slide name check ************/
	// dataset name check

	$dbConn = mysqli_connect("localhost", $guestAccount, $guestPass, "nuclei");

	if( !$dbConn ) {
		echo("<p>Unable to connect to the database server</p>" . mysqli_connect_error() );
		exit;
	}

	$sql = 'DELETE FROM datasets WHERE name="'.$deleteDatasetSel.'"';

	if( $result = mysqli_query($dbConn, $sql) ) {
		mysqli_free_result($result);
	}
	else{
		echo "<script type='text/javascript'>window.alert('Data deletion cannot be processed !! ');
		window.location.href = '../data.html?application=".$application."';</script>";
		exit;
	}

	mysqli_close($dbConn);

	echo "<script type='text/javascript'>window.alert('$deleteDatasetSel is removed !! ');
	window.location.href = '../index_home.html?application=".$application."';</script>";
	exit;

?>
