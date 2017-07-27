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
var IIPServer="";
var curDataset = "";
var slideSet = null;
var slideReq = null;
var uid = null;
var application = "";

//
//	Initialization
//
//		Get a list of available slides from the database
//		Populate the selection and classifier dropdowns
//		load the first slide
//		Register event handlers
//
$(function() {

	application = $_GET("application");

	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("nav_select").setAttribute("href","grid.html?application="+application);
	document.getElementById("nav_review").setAttribute("href","review.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_heatmaps").setAttribute("href","heatmaps.html?application="+application);

	// get slide host info
	//
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {

			uid = data['uid'];
			IIPServer = data['IIPServer'];
			curDataset = data['dataset'];

			if( uid === null ) {
				window.alert("No session active");
				window.history.back();
			} else {
				genHeatmaps();
			}
		}
	});

});





function genHeatmaps() {

	// Display the progress dialog...
	$('#progDiag').modal('show');

	$.ajax({
		type: "POST",
		url: "php/genAllHeatmaps.php",
		data: { dataset: curDataset,
				uid: uid,
				application : application,
			  },
		dataType: "json",
		success: function(data) {

			slideSet = data;

			for( var item in slideSet['scores'] ) {
				createRow(Number(item) + 1, Number(item));
			}

			// Hide progress dialog
			$('#progDiag').modal('hide');

		},
		failure: function() {
			console.log("genAllHeatmaps failed");
		}
	});
}





function createRow(rowNo, index) {

	var container = document.getElementById('heatmaps');
	var	row, col, ele, svg, anchor, anchorRef, ele2;
	var slide = String(slideSet['scores'][index]['slide']);			// Slide name may be a number


	row = document.createElement("div");
	row.setAttribute('id', 'row'+rowNo);
	row.setAttribute('class', 'row');

	anchorRef = "viewer.html?application="+application+"&slide="+ slide;

	// 1st column - Uncertainty heatmap
	col = document.createElement("div");
	col.setAttribute('id', 'box_'+rowNo+'_1');
	col.setAttribute('class', 'col-sm-4 col-md-4 col-lg-4');
	anchor = document.createElement("a");
	anchor.setAttribute('href', anchorRef);
	svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
	svg.setAttributeNS(null, "id", 'svg'+rowNo+'_1');
	anchor.appendChild(svg);
	col.appendChild(anchor);
	row.appendChild(col);

	// 2nd column - Class count heatmap
	col = document.createElement("div");
	col.setAttribute('id', 'box_'+rowNo+'_2');
	col.setAttribute('class', 'col-sm-4 col-md-4 col-lg-4');
	anchor = document.createElement("a");
	anchor.setAttribute('href', anchorRef);
	svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
	svg.setAttributeNS(null, "id", 'svg'+rowNo+'_2');
	anchor.appendChild(svg);
	col.appendChild(anchor);
	row.appendChild(col);

	// 3rd column - stats
	col = document.createElement("div");
	col.setAttribute('id', 'box_'+rowNo+'_3');
	col.setAttribute('class', 'col-sm-4 col-md-4 col-lg-4');
	ele = document.createElement("p");
	ele2 = document.createElement("strong");
	ele2.innerHTML = slide;
	ele.appendChild(ele2);
	col.appendChild(ele);
	ele = document.createElement("p");
	ele.setAttribute('id', 'stats_'+rowNo);
	ele.innerHTML = "Median uncertainty: " + slideSet['scores'][index]['uncertMedian'].toFixed(2) +
					"</br>Max Average Uncertainty: " + slideSet['scores'][index]['uncertMax'].toFixed(2) +
					"</br>Max class density: " + slideSet['scores'][index]['classMax'].toFixed(2);

	col.appendChild(ele);
	row.appendChild(col);

	container.appendChild(row);

	// Add a horizontal line between slides
	ele = document.createElement("div");
	ele.setAttribute("style", "width: 100%; height: 2px; background: #777777; overflow: hidden;");
	container.appendChild(ele);

	genSVG(Number(slideSet['scores'][index]['width']), Number(slideSet['scores'][index]['height']), rowNo, index);

}




function genSVG(width, height, rowNo, index) {

	var	img1 = document.getElementById('box_1_1'),
		scale, svgWidth, svgHeight;
	var xlinkns = "http://www.w3.org/1999/xlink", imageRef, heatRef;
	var path = String(slideSet['paths'][slideSet['scores'][index]['index']]),
		slide = String(slideSet['scores'][index]['slide']);

	if( width >= height ) {
		svgWidth = img1.offsetWidth - 30;
		scale = svgWidth / width;
		svgHeight = height * scale;
	} else {
		svgHeight = 300;	// Limit heigth to 300
		scale = svgHeight / height;
		svgWidth = width * scale;
	}

	imageRef = IIPServer + "FIF=" + path + "&WID=600&CVT=jpeg";

	// Uncertainty heatmap
	heatRef = "heatmaps/" + uid + "/" + slide + ".jpg";

	theSvg = document.getElementById('svg'+rowNo+'_1');
	theSvg.setAttribute('width', svgWidth);
	theSvg.setAttribute('height', svgHeight);

	ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
	ele.setAttributeNS(null, "x", 0);
	ele.setAttributeNS(null, "y", 0);
	ele.setAttributeNS(null, "width", svgWidth);
	ele.setAttributeNS(null, "height", svgHeight);
	ele.setAttributeNS(xlinkns, "xlink:href", imageRef);

	theSvg.appendChild(ele);

	ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
	ele.setAttributeNS(null, "x", 0);
	ele.setAttributeNS(null, "y", 0);
	ele.setAttributeNS(null, "width", svgWidth);
	ele.setAttributeNS(null, "height", svgHeight);
	ele.setAttributeNS(null, 'opacity', 0.25);
	ele.setAttributeNS(xlinkns, "xlink:href", heatRef);

	theSvg.appendChild(ele);

	// Class count heatmap
	heatRef = "heatmaps/" + uid + "/" + slide + "_class.jpg";

	theSvg = document.getElementById('svg'+rowNo+'_2');
	theSvg.setAttribute('width', svgWidth);
	theSvg.setAttribute('height', svgHeight);

	ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
	ele.setAttributeNS(null, "x", 0);
	ele.setAttributeNS(null, "y", 0);
	ele.setAttributeNS(null, "width", svgWidth);
	ele.setAttributeNS(null, "height", svgHeight);
	ele.setAttributeNS(xlinkns, "xlink:href", imageRef);

	theSvg.appendChild(ele);

	ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
	ele.setAttributeNS(null, "x", 0);
	ele.setAttributeNS(null, "y", 0);
	ele.setAttributeNS(null, "width", svgWidth);
	ele.setAttributeNS(null, "height", svgHeight);
	ele.setAttributeNS(null, 'opacity', 0.25);
	ele.setAttributeNS(xlinkns, "xlink:href", heatRef);

	theSvg.appendChild(ele);

}

//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}
