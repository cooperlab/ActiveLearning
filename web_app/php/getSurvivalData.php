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
	require '../db/logging.php';
	/* 	Retrieve a list of datasets from the data base.
		Return as a json object
	*/
	$slides = $_POST['slideSet'];
  //$posfloat = $slides['scores'][0]['posNum']/$slides['scores'][0]['totalNum'];
	//$posPernt = round($posfloat * 100 );
	$list = array();
	foreach ($slides['scores'] as $v) {
		$posfloat = $v['posNum']/$v['totalNum'];
		$posPernt = round($posfloat * 100 );
		$slidelist = array();
		array_push($slidelist, $v['slide']);
		array_push($slidelist, $posPernt);
		array_push($list, $slidelist);
	}
		$fpath = '../trainingsets/tmp/tmp.csv';
		$fp = fopen($fpath, 'w');
		foreach ($list as $fields) {
		    fputcsv($fp, $fields);
		}
		fclose($fp);
		// Execute the python script with the JSON data
		$results = shell_exec('python ../python/test.py '.$fpath);
	  // Decode the result
	  $results = json_decode($results);
		//write_log("INFO","json results fro python code: ".$results['test']);
		$clinicalData = array("scores" => $results );
		echo json_encode($clinicalData);
?>
