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

			console.log("UID: "+uid);

			if( uid == "" ) {
				$('#nav_grid').hide();
			} else {
				// There's an active session, disable the "start session" 
				// form.
				//
				$('#beginSession').attr('disabled', 'true');
				// TODO - Populate the text fields with the session values
				// and diabple them
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
