<?php
  require 'logging.php';		// Also includes connect.php

	$dbConn = guestConnect();

  $data_type = $_POST['dataname'];
  $group_type = $_POST['groupname'];

  $jsonData = array();   //json   :    [  ,  ]


/*
  if field name is age
*/

  if ($data_type === "Age"){
    // if age is greater than avg age
    $sql = "select Censored, survival_time from clinical where age_at_initial > (select AVG(age_at_initial) from clinical)";
    if( $result = mysqli_query($dbConn, $sql) ) {

  		while( $array = mysqli_fetch_row($result) ) {
  			$above_age = array();
  			$above_age[] = $array[0];
  			$above_age[] = $array[1];
        $above_age[] = $group_type[0];

  			$jsonData[] = $above_age;
  		}
  		mysqli_free_result($result);
    }
    // if age is less than equeal than avg age
    $sql = "select Censored, survival_time from clinical where age_at_initial <= (select AVG(age_at_initial) from clinical)";
    if( $result = mysqli_query($dbConn, $sql) ) {

  		while( $array = mysqli_fetch_row($result) ) {
  			$below_age = array();
  			$below_age[] = $array[0];
  			$below_age[] = $array[1];
        $below_age[] = $group_type[1];

  			$jsonData[] = $below_age;
  		}
  		mysqli_free_result($result);
    }
  }


/*
  if field name is gender
*/

  else if ($data_type === "Gender"){

    // if gender is male
    $sql = "SELECT Censored, survival_time from clinical where gender_male = 1";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $above_age = array();
        $above_age[] = $array[0];
        $above_age[] = $array[1];
        $above_age[] = $group_type[0];

        $jsonData[] = $above_age;
      }
      mysqli_free_result($result);
    }
    // if gender is female
    $sql = "SELECT Censored, survival_time from clinical where gender_male = 0";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $below_age = array();
        $below_age[] = $array[0];
        $below_age[] = $array[1];
        $below_age[] = $group_type[1];

        $jsonData[] = $below_age;
      }
      mysqli_free_result($result);
    }
  }

/*
if field name is race
*/
  else if ($data_type === "Race"){

    $sql = "SELECT Censored, survival_time from clinical where race_white = 1";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $above_age = array();
        $above_age[] = $array[0];
        $above_age[] = $array[1];
        $above_age[] = $group_type[0];

        $jsonData[] = $above_age;
      }
      mysqli_free_result($result);
    }
    
    $sql = "SELECT Censored, survival_time from clinical where race_Black_orAfricanAmerican = 1";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $below_age = array();
        $below_age[] = $array[0];
        $below_age[] = $array[1];
        $below_age[] = $group_type[1];

        $jsonData[] = $below_age;
      }
      mysqli_free_result($result);
    }

    $sql = "SELECT Censored, survival_time from clinical where race_Asian = 1";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $below_age = array();
        $below_age[] = $array[0];
        $below_age[] = $array[1];
        $below_age[] = $group_type[2];

        $jsonData[] = $below_age;
      }
      mysqli_free_result($result);
    }

    $sql = "SELECT Censored, survival_time from clinical where race_AmericanIndian_orAlaskanative = 1";
    if( $result = mysqli_query($dbConn, $sql) ) {

      while( $array = mysqli_fetch_row($result) ) {
        $below_age = array();
        $below_age[] = $array[0];
        $below_age[] = $array[1];
        $below_age[] = $group_type[3];

        $jsonData[] = $below_age;
      }
      mysqli_free_result($result);
    }
  }

  mysqli_close($dbConn);

  echo json_encode($jsonData);

?>
