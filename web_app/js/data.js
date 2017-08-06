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
var application = "";
var projectDirMain = "";
var projectDir ="";
var array_mean = "";

$(function() {

	application = $_GET('application');
	projectDirMain = "/fastdata/features/";

	$("#applicationSel").val(application);
	$("#applicationSeldel").val(application);

	document.getElementById("index").setAttribute("href","index.html");
	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
	document.getElementById("nav_data").setAttribute("href","data.html?application="+application);
	document.getElementById("nav_validation").setAttribute("href","validation.html?application="+application);

	$.ajax({
		type: "POST",
		url: "php/data_getprojects.php",
		data: { projectDirMain: projectDirMain },
		dataType: "json",
		success: function(data) {
			// set first featurename
			for (var i = 0; i < data['projectDir'].length; i++) {
				$("#projectSel").append(new Option(data['projectDir'][i], data['projectDir'][i]));
			}
			updateFeature();
		}
	});

	$("#projectSel").change(updateFeature);
	$("#featureSel").change(updateFeatureInfo);
	$("#pyramidSel").change(updatePyramidTable);

	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			for( var item in data ) {
				$("#deleteDatasetSel").append(new Option(data[item][0], data[item][0]));
			}
		}
	});

	//$("#datasetSel").change(updateFeatures);
	$('#progDiag').modal('hide');
	$('#_form').submit(function() {
  		$('#progDiag').modal('show');
	});


});



function updateChart() {

	$('#chart').empty();

	var w = 300,
	h = 300;

	var colorscale = d3.scale.category10();

	//Legend titles
	//var LegendOptions = [];
	//LegendOptions.push(array_slides[0]);

	var d = [];
	for (var i = 0; i < 1; i++) {
	 	 d[i] = [];
	}

	//for (var i = 0; i < array_slides.length; i++) {
 for (var j = 0; j < array_mean.length; j++) {
	 d[0].push({
			 axis: j.toString(),
			 value: array_mean[j]
		 });
 }
	//}

	//Options for the Radar chart, other than default
	var mycfg = {
	 w: w,
	 h: h,
	 maxValue: -1,
	 levels: 6,
	 ExtraWidthX: 300
	}

	RadarChart.draw("#chart", d, mycfg);
}



function updateTable() {

	$('#pyramidTable').empty();

	var column_names = ["Slide name","Width","Height","Scale"];
	var clicks = {slides: 0, width: 0, height: 0, scale: 0};

	var table = d3.select("#pyramidTable").append("table");
	table.append("thead").append("tr");

	var headers = table.select("tr").selectAll("th")
			.data(column_names)
		.enter()
			.append("th")
			.text(function(d) { return d; });

	var rows, row_entries, row_entries_no_anchor, row_entries_with_anchor;

	d3.json("csv/data.json", function(data) { // loading data from server

		// draw table body with rows
		table.append("tbody")

		// data bind
		rows = table.select("tbody").selectAll("tr")
			.data(data, function(d){ return d.slides; });

		// enter the rows
		rows.enter()
			.append("tr")

		// enter td's in each row
		row_entries = rows.selectAll("td")
				.data(function(d) {
					var arr = [];
					for (var k in d) {
						if (d.hasOwnProperty(k)) {
					arr.push(d[k]);
						}
					}
					return [arr[0],arr[1],arr[2],arr[4]];
				})
			.enter()
				.append("td")

		// draw row entries with no anchor
		row_entries_no_anchor = row_entries.filter(function(d) {
			return (/https?:\/\//.test(d) == false)
		})
		row_entries_no_anchor.text(function(d) { return d; })

		/**  sort functionality **/
		headers
			.on("click", function(d) {
				if (d == "Slide name") {
					clicks.slides++;
					// even number of clicks
					if (clicks.slides % 2 == 0) {
						// sort ascending: alphabetically
						rows.sort(function(a,b) {
							if (a.slides.toUpperCase() < b.slides.toUpperCase()) {
								return -1;
							} else if (a.slides.toUpperCase() > b.slides.toUpperCase()) {
								return 1;
							} else {
								return 0;
							}
						});
					// odd number of clicks
					} else if (clicks.slides % 2 != 0) {
						// sort descending: alphabetically
						rows.sort(function(a,b) {
							if (a.slides.toUpperCase() < b.slides.toUpperCase()) {
								return 1;
							} else if (a.slides.toUpperCase() > b.slides.toUpperCase()) {
								return -1;
							} else {
								return 0;
							}
						});
					}
				}
				if (d == "Width") {
				clicks.width++;
					// even number of clicks
					if (clicks.width % 2 == 0) {
						// sort ascending: numerically
						rows.sort(function(a,b) {
							if (+a.width < +b.width) {
								return -1;
							} else if (+a.width > +b.width) {
								return 1;
							} else {
								return 0;
							}
						});
					// odd number of clicks
					} else if (clicks.width % 2 != 0) {
						// sort descending: numerically
						rows.sort(function(a,b) {
							if (+a.width < +b.width) {
								return 1;
							} else if (+a.width > +b.width) {
								return -1;
							} else {
								return 0;
							}
						});
					}
				}
				if (d == "Height") {
					clicks.height++;
						// even number of clicks
						if (clicks.height % 2 == 0) {
							// sort ascending: numerically
							rows.sort(function(a,b) {
								if (+a.height < +b.height) {
									return -1;
								} else if (+a.height > +b.height) {
									return 1;
								} else {
									return 0;
								}
							});
						// odd number of clicks
						} else if (clicks.height % 2 != 0) {
							// sort descending: numerically
							rows.sort(function(a,b) {
								if (+a.height < +b.height) {
									return 1;
								} else if (+a.height > +b.height) {
									return -1;
								} else {
									return 0;
								}
							});
						}
				}

				if (d == "Scale") {
				clicks.scale++;
					// even number of clicks
					if (clicks.scale % 2 == 0) {
						// sort ascending: numerically
						rows.sort(function(a,b) {
							if (+a.scale < +b.scale) {
								return -1;
							} else if (+a.scale > +b.scale) {
								return 1;
							} else {
								return 0;
							}
						});
					// odd number of clicks
					} else if (clicks.scale % 2 != 0) {
						// sort descending: numerically
						rows.sort(function(a,b) {
							if (+a.scale < +b.scale) {
								return 1;
							} else if (+a.scale > +b.scale) {
								return -1;
							} else {
								return 0;
							}
						});
					}
				}
			}) // end of click listeners
	});
	d3.select(self.frameElement).style("height", "300px").style("width", "400px");

}



function updatePyramidTable() {

	var csvFile = $("#pyramidSel :selected").text();

	$.ajax({
		type: "POST",
		url: "php/data_csvtojson.php",
		data: { csvFile: csvFile },
		dataType: "json",
		success: function(data) {

			$('#pyramidTable').empty();
			var test = data;
			updateTable();
		}
	});

}



function updatePyramid(){

	projectDir = projectDirMain+projectSel.options[projectSel.selectedIndex].label;
	var pyramidSel = pyramidSel.options[pyramidSel.selectedIndex].label;

	$.ajax({
		type: "POST",
		url: "php/data_getpyramidinfofromdir.php",
		data: { projectDir: projectDir },
		dataType: "json",
		success: function(data) {

			$('#pyramidSel').empty();

			for (var i = 0; i < data['slideInfo'].length; i++) {
				$("#pyramidSel").append(new Option(data['slideInfo'][i], data['slideInfo'][i]));
			}
			if (document.getElementById('pyramidSel').options.length > 0) {
				updatePyramidTable();
			}

		}
	});

}
//
//	Updates the list of feature files located in the project directory
//
function updateFeature() {

	projectDir = projectDirMain+projectSel.options[projectSel.selectedIndex].label;

	$('#pyramidSel').empty();
	$('#featureSel').empty();
	$('#boundarySel').empty();
	$('#chart').empty();
	$('#pyramidTable').empty();


	$.ajax({
		type: "POST",
		url: "php/data_getdatasetsfromdir.php",
		data: { projectDir: projectDir,
						application: application },
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['featureName'].length; i++) {
				$("#featureSel").append(new Option(data['featureName'][i], data['featureName'][i]));
			}
			if (document.getElementById('featureSel').options.length > 0) {
				updateFeatureInfo();
			}
		}
	});

	$.ajax({
		type: "POST",
		url: "php/data_getboundariesfromdir.php",
		data: { projectDir: projectDir },
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['boundaryDir'].length; i++) {
				$("#boundarySel").append(new Option(data['boundaryDir'][i], data['boundaryDir'][i]));
			}
			//updateFeatures();
		}
	});

	$.ajax({
		type: "POST",
		url: "php/data_getpyramidinfofromdir.php",
		data: { projectDir: projectDir },
		dataType: "json",
		success: function(data) {

			for (var i = 0; i < data['slideInfo'].length; i++) {
				$("#pyramidSel").append(new Option(data['slideInfo'][i], data['slideInfo'][i]));
			}
			if (document.getElementById('pyramidSel').options.length > 0) {
				updatePyramidTable();
			}

		}
	});

}


//
//	Updates the list of available slides for the current dataset
//
function updateFeatureInfo() {

	var featurename = featureSel.options[featureSel.selectedIndex].label;
	var delay = 2000;

	$('#progDiag').modal('show');

	// Get the information for the current dataset
	 $.ajax({
	 	type: "POST",
	 	url: "php/data_getfeatures.php",
		data: { projectDir: projectDir,
						featurename: featurename },
	 	dataType: "json",
	 	success: function(data) {
			setTimeout(function() {
			 delaySuccess(data);
			  }, delay);
	 	}
	 });


}

function delaySuccess(data) {

	array_mean = data['mean'];

	updateChart();

	$('#progDiag').modal('hide');

}

//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}
