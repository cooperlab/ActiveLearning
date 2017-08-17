
<!DOCTYPE html>
<html lang="en">
	 <head>
				<meta content="text/html;charset=utf-8" http-equiv="Content-Type">
				<meta content="utf-8" http-equiv="encoding">
      </head>
    <body>

<?php
		// require '../db/logging.php';
		if(isset($_POST['uni_coeff'])) {
			$csv_data = $_POST['uni_coeff'];
			file_put_contents("uni_coeff.json", $csv_data);
			$uni_result = shell_exec('python multiCoeff.py uni_coeff.json uni');
			$resultData = json_decode($uni_result, true);
			var_dump($resultData);

		}else if(isset($_POST['multi_coeff'])){
			$json = $_POST['multi_coeff'];
			file_put_contents("multi_coeff.json", $json);
			$multi_result = shell_exec('python multiCoeff.py multi_coeff.json multi');
			$resultData = json_decode($multi_result, true);
			var_dump($resultData);
		}
?>
    </body>
</html>
