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

	//require 'connect.php';
	require 'logging.php';		// Also includes connect.php

	/* 	Retrieve a list of datasets from the data base.
		Return as a json object
	*/

	$application = $_POST['application'];
	write_log("INFO","Application: ".$application);
	$dbConn = guestConnect();
	$sql = "";

	if( $application == "nuclei" ) {
		$sql = "SELECT name,features_file from datasets
			 where not (features_file like '%spfeatures%' OR features_file like '%pofeatures%') order by name";
	}
	elseif( $application == "region" ) {
		$sql = "SELECT name,features_file from datasets
			where features_file like '%spfeatures%' order by name";
	 }
	 elseif( $application == "cell" ) {
		 $sql = "SELECT name,features_file from datasets
	 		where features_file like '%pofeatures%' order by name";
	 }else{
		 log_error("application selection failed:  ");
	 }

	 if( $result = mysqli_query($dbConn, $sql) ) {

		 	$jsonData = array();
			while( $array = mysqli_fetch_row($result) ) {
	 			$obj = array();

	 			$obj[] = $array[0];
	 			$obj[] = $array[1];

	 			$jsonData[] = $obj;
	 		}
 			mysqli_free_result($result);

 			echo json_encode($jsonData);
  }

	mysqli_close($dbConn);
?>
