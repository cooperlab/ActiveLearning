var uid = "";
var classifier = "";
var negClass = "";
var posClass = "";




//
//	Initialization
//
//
$(function() {

	var	datasetSel = $("#datasetSel");

	// get session vars
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			if( data.length > 0 ) {
				uid = data[0];
				classifier = data[1];
				posClass = data[2];
				negClass = data[3];
			}
			
			if( uid == "" ) {
				$('#nav_grid').hide();
			}
		}
	});



	$.ajax({
		url: "db/getdatasets.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			curDataset = data[0];		// Use first dataset initially
				
			for( var item in data ) {
				datasetSel.append(new Option(data[item], data[item]));
			}
		}
	});

});
