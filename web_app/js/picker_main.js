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
	//
	$.ajax({
		url: "../php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			uid = data['uid'];
			classifier = data['className'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			curDataset = data['dataset'];
			IIPServer = data['IIPServer'];

/*
			if( uid === null ) {
				$('#nav_select').hide();
				$('#nav_visualize').hide();
			} else {
				// There's an active session, disable the "start session" 
				// form.
				//
				$('#beginSession').attr('disabled', 'true');
				$('#trainset').attr('disabled', 'true');
				$('#datasetSel').attr('disabled', 'true');
				$('#posClass').attr('disabled', 'true');
				$('#negClass').attr('disabled', 'true');
				
				// TODO - Populate the text fields with the session values.
				// This way we can see the criteria for the
				// current session
			}
*/
		}
	});


	// Populate Dataset dropdown
	//
	$.ajax({
		url: "../db/getdatasets.php",
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


