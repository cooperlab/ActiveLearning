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
var uid = "";
var featurename = "";
var application = "";

$(function() {

	application = $_GET('application');
	$("#applicationSel").val(application);

	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
	document.getElementById("nav_data").setAttribute("href","data.html?application="+application);

	var	boundarySel = $("#boundarySel"),
		pyramidSel = $("#pyramidSel"), slideSel = $("#slideSel"),
		slideInfo = $('#slideInfo'), deleteDatasetSel = $("#deleteDatasetSel");

		$('#progDiag').modal('hide');
		$('#_form').submit(function() {
    		$('#progDiag').modal('show');
		});

	$.ajax({
		type: "POST",
		url: "php/data_getdatasetsfromdir.php",
		data: "",
		dataType: "json",
		success: function(data) {
			// set first featurename
			for (var i = 0; i < data['dirNames'].length; i++) {
				$("#datasetSel").append(new Option(data['dirNames'][i], data['dirNames'][i]));
			}
			//updateFeatures();
		}
	});

	$.ajax({
		type: "POST",
		url: "php/data_getboundariesfromdir.php",
		data: "",
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['dirNames'].length; i++) {
				boundarySel.append(new Option(data['dirNames'][i], data['dirNames'][i]));
			}
			//updateSlideList();
		}
	});

	$.ajax({
		type: "POST",
		url: "php/data_getpyramidinfofromdir.php",
		data: "",
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['dirNames'].length; i++) {
				pyramidSel.append(new Option(data['dirNames'][i], data['dirNames'][i]));
			}
			//updateSlideList();
		}
	});

	$.ajax({
		type: "POST",
		url: "php/data_getslideinfofromdir.php",
		data: "",
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['dirNames'].length; i++) {
				slideSel.append(new Option(data['dirNames'][i], data['dirNames'][i]));
			}
			//updateSlideList();
		}
	});
	// Need to montior changes for the map score select controls. Slide image
	//	size is dependant on these.
	//
	//$("#datasetSel").change(updateDataset);
	//$("#boundarySel").change(updateBoundaries);
	$('input[type="radio"]').on('change', function(){
    if($('#slideInfo').is(':checked')) {
        $('#pyramidSel').show();
    } else {
        $('#pyramidSel').hide();
    }
	})


	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			for( var item in data ) {
				deleteDatasetSel.append(new Option(data[item][0], data[item][0]));
			}
		}
	});

	$("#datasetSel").change(updateFeatures);

});


//
//	Updates the list of available slides for the current dataset
//
function updateFeatures() {

	featurename = datasetSel.options[datasetSel.selectedIndex].label;
	//document.getElementById('featureName').innerHTML = featurename;

	// Get the information for the current dataset
	// $.ajax({
	// 	type: "POST",
	// 	url: "php/data_getfeatures.php",
	// 	data: { featurename: featurename },
	// 	dataType: "json",
	// 	success: function(data) {
	//
  //     document.getElementById('featureInfo').innerHTML = data['features'];
	//
	// 	}
	// });
}

//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}
