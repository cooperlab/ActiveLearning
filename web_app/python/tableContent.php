<?php
		if(isset($_POST['uni_coeff'])) {
			$csv_data = $_POST['uni_coeff'];
			file_put_contents("uni_coeff.json", $csv_data);
			$uni_result = shell_exec('python multiCoeff.py uni_coeff.json uni');
		}else if(isset($_POST['multi_coeff'])){
			$json = $_POST['multi_coeff'];
			file_put_contents("multi_coeff.json", $json);
			$multi_result = shell_exec('python multiCoeff.py multi_coeff.json multi');
			$result = json_decode($multi_result);
			$test = array("multi" => $result);
      echo json_encode($test);
		}else if(isset($_POST['logrank'])){
			$log_data = $_POST['logrank'];
			file_put_contents("logrank.json", $log_data);
			$logrank_result = shell_exec('python getlogrank.py logrank.json');
			$rst = json_decode($logrank_result);
			$dt = array("lg" => $rst);
			echo json_encode($dt);
		}
?>
