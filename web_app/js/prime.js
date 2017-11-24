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
var annoGrpTransformFunc;
var IIPServer = "";



//var SlideSuffix = ".svs-tile.dzi.tif";
var SlideSuffix = ".svs.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";

var uid = "";
var classifier = "";
var negClass = "";
var posClass = "";
var viewer = null;
var imgHelper = null, osdCanvas = null, viewerHook = null;
var curSlide = "", curDataset = "";

var displaySeg = false, selectNuc = false;
var lastScaleFactor = 0;
var clickCount = 0;

var	selectedJSON = [];
var pyramids;

var boundsLeft = 0, boundsRight = 0, boundsTop = 0, boundsBottom = 0;

// 8 boxes used for a deleting function performed when the user double-clicking a box
var	boxes = ["box_1", "box_2", "box_3", "box_4", "box_5", "box_6","box_7", "box_8"];
var application = "";
var scale = 0.5;
var superpixel_size = 0;
//
//	Initialization
//
//
$(function() {

	application = $_GET("application");

	document.getElementById("prime").setAttribute("href","prime.html?application="+application);

	// Setup the grid slider relative to the window width
	var width = 0;
	$('#overflow .slider div').each(function() {
		width += $(this).outerWidth(true);
	});
	$('#overflow .slider').css('width', width + "px");


	// Create the slide zoomer, add event handlers, etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "slideZoom", prefixUrl: "images/", animationTime: 0.1});
	imgHelper = viewer.activateImagingHelper({onImageViewChanged: onImageViewChanged});
	viewerHook = viewer.addViewerInputHook({ hooks: [
					{tracker: 'viewer', handler: 'clickHandler', hookHandler: onMouseClick}
			]});

	annoGrpTransformFunc = ko.computed(function() {
										return 'translate(' + svgOverlayVM.annoGrpTranslateX() +
										', ' + svgOverlayVM.annoGrpTranslateY() +
										') scale(' + svgOverlayVM.annoGrpScale() + ')';
									}, this);


	//
	// Image handlers
	//
	viewer.addHandler('open', function(event) {
		osdCanvas = $(viewer.canvas);
		statusObj.haveImage(true);

		osdCanvas.on('mouseenter.osdimaginghelper', onMouseEnter);
		osdCanvas.on('mousemove.osdimaginghelper', onMouseMove);
		osdCanvas.on('mouseleave.osdimaginghelper', onMouseLeave);

		statusObj.imgWidth(imgHelper.imgWidth);
		statusObj.imgHeight(imgHelper.imgHeight);
		statusObj.imgAspectRatio(imgHelper.imgAspectRatio);
		statusObj.scaleFactor(imgHelper.getZoomFactor());

	});


	viewer.addHandler('close', function(event) {
		osdCanvas = $(viewer.canvas);
		statusObj.haveImage(false);

        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
		osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
		posSel = negSel = totalSel = 0;
	});


	viewer.addHandler('animation-finish', function(event) {

		if( displaySeg ) {
			if (application == "region") {
				scale = 0.05;
			}
			if( statusObj.scaleFactor() > scale ) {
				$('.overlaySvg').css('visibility', 'visible');
				var centerX = statusObj.dataportLeft() +
							  ((statusObj.dataportRight() - statusObj.dataportLeft()) / 2);
				var centerY = statusObj.dataportTop() +
							  ((statusObj.dataportBottom() - statusObj.dataportTop()) / 2);

				if( centerX < boundsLeft || centerX > boundsRight ||
					centerY < boundsTop || centerY > boundsBottom ) {

					updateSeg();
				}

			} else {
				$('.overlaySvg').css('visibility', 'hidden');
			}
		}
	});

	// Assign click handlers to each of the thumbnail divs
	// also used when the user double-clicking the thumbnail box
	boxes.forEach(function(entry) {

		var	box = document.getElementById(entry);
		var	clickCount = 0;

		box.addEventListener('click', function() {
			clickCount++;
			if( clickCount === 1 ) {
				singleClickTimer = setTimeout(function() {
					clickCount = 0;
				}, 200);
			} else if( clickCount === 2 ) {
				clearTimeout(singleClickTimer);
				clickCount = 0;
				// if the box are doubleclicked, call deleteSample with entry number
				deleteSample(entry);
			}
		}, false);
	});


	// get session vars and load the first slide
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {

			uid = data['uid'];
			classifier = data['className'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			curDataset = data['dataset'];
			IIPServer = data['IIPServer'];

			if( uid == null ) {
				window.alert("No session active");
				window.history.back();
			} else {
				updateSlideList();
				$('#posLabel').text(posClass);
				$('#negLabel').text(negClass);
			}
		}
	});

	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			for( var item in data ) {
				if (curDataset == data[item][0]) {
					superpixel_size = data[item][2];
				}
			}
		}
	});

	// Set the update handler for the slide selector
	$("#slideSel").change(updateSlide);

	posSel = 0;
	negSel = 0;
	clickCount = 0;

});





function updateSlideList() {
	var slideSel = $("#slideSel");
	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: curDataset },
		dataType: "json",
		success: function(data) {

			pyramids = data['paths'];
			curSlide = String(data['slides'][0]);		// Start with the first slide in the list

			slideSel.empty();
			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data['slides'] ) {
				slideSel.append(new Option(data['slides'][item], data['slides'][item]));
			}

			// Get the slide pyrimaid and display
			updateSlideView();
		}
	});
}







function updateSlideView() {


	// Zoomer needs '.dzi' appended to the end of the file
	pyramid = "DeepZoom="+pyramids[$('#slideSel').prop('selectedIndex')]+".dzi";
	viewer.open(IIPServer + pyramid);
}





//
//	A new slide has been selected from the drop-down menu, update the
// 	slide zoomer.
//
//
function updateSlide() {

	curSlide = $('#slideSel').val();
	updateSlideView();
}





//
//	Update annotation and viewport information when the view changes
//  due to panning or zooming.
//
//
function onImageViewChanged(event) {
	var boundsRect = viewer.viewport.getBounds(true);

	// Update viewport information. dataportXXX is the view port coordinates
	// using pixel locations. ie. if dataPortLeft is  0 the left edge of the
	// image is aligned with the left edge of the viewport.
	//
	statusObj.viewportX(boundsRect.x);
	statusObj.viewportY(boundsRect.y);
	statusObj.viewportW(boundsRect.width);
	statusObj.viewportH(boundsRect.height);
	statusObj.dataportLeft(imgHelper.physicalToDataX(imgHelper.logicalToPhysicalX(boundsRect.x)));
	statusObj.dataportTop(imgHelper.physicalToDataY(imgHelper.logicalToPhysicalY(boundsRect.y)) * imgHelper.imgAspectRatio);
	statusObj.dataportRight(imgHelper.physicalToDataX(imgHelper.logicalToPhysicalX(boundsRect.x + boundsRect.width)));
	statusObj.dataportBottom(imgHelper.physicalToDataY(imgHelper.logicalToPhysicalY(boundsRect.y + boundsRect.height))* imgHelper.imgAspectRatio);
	statusObj.scaleFactor(imgHelper.getZoomFactor());

	var p = imgHelper.logicalToPhysicalPoint(new OpenSeadragon.Point(0, 0));

	svgOverlayVM.annoGrpTranslateX(p.x);
	svgOverlayVM.annoGrpTranslateY(p.y);
	svgOverlayVM.annoGrpScale(statusObj.scaleFactor());

	var annoGrp = document.getElementById('annoGrp');
	annoGrp.setAttribute("transform", annoGrpTransformFunc());
}





//
//	Retreive the boundaries for nuclei within the viewport bounds.
//	TODO - Look into expanding the nuclei request to a 'viewport' width
//			boundary around the view port. Since we are now using the
//			'animation-finish' event to trigger the request, it may be
//			possible to retreive that many boundaries in a sufficient
//			amount of time
//
function updateSeg() {

	if (application == "region") {
		scale = 0.05;
	}

	if( statusObj.scaleFactor() > scale ) {

		var left, right, top, bottom, width, height;

		// Grab nuclei a viewport width surrounding the current viewport
		//
		width = statusObj.dataportRight() - statusObj.dataportLeft();
		height = statusObj.dataportBottom() - statusObj.dataportTop();

		left = (statusObj.dataportLeft() - width > 0) ?	statusObj.dataportLeft() - width : 0;
		right = statusObj.dataportRight() + width;
		top = (statusObj.dataportTop() - height > 0) ?	statusObj.dataportTop() - height : 0;
		bottom = statusObj.dataportBottom() + height;

	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { slide: 	curSlide,
					trainset: "none",
					dataset: "none",
					left:	left,
					right:	right,
					top:	top,
					bottom:	bottom,
					application: application,
			},

			success: function(data) {

					var ele;
					var segGrp = document.getElementById('segGrp');
					var annoGrp = document.getElementById('anno');

					// Save current viewport location
					boundsLeft = statusObj.dataportLeft();
					boundsRight = statusObj.dataportRight();
					boundsTop = statusObj.dataportTop();
					boundsBottom = statusObj.dataportBottom();

					// If group exists, delete it
					if( segGrp != null ) {
						segGrp.parentNode.removeChild(segGrp);
					}

					// Create segment group
          segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
          segGrp.setAttribute('id', 'segGrp');
          annoGrp.appendChild(segGrp);

					if (application == "cell"){
						for( cell in data ) {
							ele = document.createElementNS("http://www.w3.org/2000/svg", "circle");

							ele.setAttribute('cx', data[cell][1]);
							ele.setAttribute('cy', data[cell][2]);
							ele.setAttribute('r', 2);
							ele.setAttribute('id', 'N' + data[cell][0]);
							ele.setAttribute('stroke', 'aqua');
							ele.setAttribute('fill', 'aqua');

							segGrp.appendChild(ele);
						}

					} else{
						for( cell in data ) {
							ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

							ele.setAttribute('points', data[cell][0]);
							ele.setAttribute('id', 'N' + data[cell][1]);
							ele.setAttribute('stroke', 'aqua');
							if (application == "region") {
								ele.setAttribute('stroke-width', 4);
								ele.setAttribute("stroke-dasharray", "5,5");
							}
							ele.setAttribute('fill', 'none');

							segGrp.appendChild(ele);
						}

					}

					if( selectedJSON.length > 0 ) {
						for( i = 0; i < selectedJSON.length; i++ ) {
							var bound = document.getElementById("N"+selectedJSON[i]['id']);
							if( bound != null ) {
								if (application == "region") {
									bound.setAttribute('fill', 'yellow');
									bound.setAttribute("fill-opacity", "0.2");
								}
								else{
									bound.setAttribute('stroke', 'yellow');
								}
							}
						}
					}

	  		}
    	});
	}
}

//
// Check if a sample selected is duplicated or not
// Parameters
// centroid position (X,Y) of the selected sample
// Return
// true: if duplicated
// false: if not duplicated
//
function duplicateCheck(x,y) {

	var centX = x;
	var centY = y;

	for( i = 0; i < selectedJSON.length; i++ ) {
			if ((selectedJSON[i]['centX'] == centX) && (selectedJSON[i]['centY'] == centY))
			return true;
	}

	return false;
}

//
// Delete a sample selected from thumbnail box
// Parameters
// box id ex) box_1, box_2, ...
// Call undoBoundColors() and displayThumbNail()
//
function deleteSample(box) {
	var index = boxes.indexOf(box);

	for( i =0; i < selectedJSON.length; i++ ) {
			// remove sample
			var thumbTag = "#thumb_"+(i+1);
			$(thumbTag).attr("src", "");
			// remove box
			var boxDiv = "#box_"+(i+1);
			$(boxDiv).hide();
	}

	if( selectedJSON[index]['label'] == 1 ) {
		statusObj.posSel(statusObj.posSel() - 1);
	} else if( selectedJSON[index]['label'] == -1 ) {
		statusObj.negSel(statusObj.negSel() - 1);
	}
	// call undoBoundColors
	undoBoundColors(selectedJSON[index]['id']);

	selectedJSON.splice(index,1);

	displayThumbNail();

	if( statusObj.posSel()  > 3 ) {
		$('#instruct').text("Selecting "+negClass+" samples");
	}
	else {
		$('#instruct').text("Selecting "+posClass+" samples");
	}
}

//
// Display thubmnails
// When the user removed a box, new thumbnail should be displayed again.
//
function displayThumbNail(){

	// get current path from the select slide
	var currentPath = pyramids[$('#slideSel').prop('selectedIndex')];
	var SlidePathPre = "";
	var pyramidPath = "";
  var scale_cen = 25;
	var scale_size = 50.0;

	if (application == "region") {
		if (superpixel_size == "16") {
			scale_cen = 36;
			scale_size = 64.0;
		}
		else {
			scale_cen = 64;
			scale_size = 128.0;
		}
	}

	for( i =0; i < selectedJSON.length; i++ ) {
	 		var box = "#box_" + (i + 1), thumbTag = "#thumb_" + (i + 1),
					labelTag = "#label_" + (i + 1), loc, label;


		centX = (selectedJSON[i]['centX'] - (scale_cen * selectedJSON[i]['scale'])) / selectedJSON[i]['maxX'];
		centY = (selectedJSON[i]['centY'] - (scale_cen * selectedJSON[i]['scale'])) / selectedJSON[i]['maxY'];
		sizeX = (scale_size * selectedJSON[i]['scale']) / selectedJSON[i]['maxX'];
		sizeY = (scale_size * selectedJSON[i]['scale']) / selectedJSON[i]['maxY'];

		loc = centX+","+centY+","+sizeX+","+sizeY;

		// set pyramids path
		SlidePathPre = currentPath.substring(0, currentPath.lastIndexOf("/") + 1);
		pyramidPath = SlidePathPre+selectedJSON[i]['slide']+SlideSuffix;

		var thumbNail = IIPServer+"FIF="+pyramidPath
								+SlideLocPre+loc+"&WID=100"+SlideLocSuffix;

		$(thumbTag).attr("src", thumbNail);

		label = $(box).children(".classLabel")
		$(box).show();


		if( selectedJSON[i]['label'] == 1 ) {
			$(labelTag).text(posClass);
			label.removeClass("negLabel").addClass("posLabel");
		} else if ( selectedJSON[i]['label'] == -1 ){
			$(labelTag).text(negClass);
			label.removeClass("posLabel").addClass("negLabel");
		}
	}
}

//
// Update colors when a sample is selected
// Parameters
// selectedJSON id
//
function updateBoundColors(currentID) {

	for( i = 0; i < selectedJSON.length; i++ ) {

		var bound = document.getElementById("N"+selectedJSON[i]['id']);

		if( bound != null ) {
			if (selectedJSON[i]['id'] == currentID) {
				if (application == "region") {
					bound.setAttribute('fill', 'yellow');
					bound.setAttribute("fill-opacity", "0.2");
				}
				else{
					bound.setAttribute('stroke', 'yellow');
				}
			}
		}
	}
}

//
// Undo colors when a sample is deleted
// Parameters
// selectedJSON id
//
function undoBoundColors(currentID) {

	for( i = 0; i < selectedJSON.length; i++ ) {

		var bound = document.getElementById("N"+selectedJSON[i]['id']);

		if( bound != null ) {
			if (selectedJSON[i]['id'] == currentID){
				if (application == "region") {
					bound.setAttribute('fill', 'none');
				}
				else{
					bound.setAttribute('stroke', 'aqua');
				}
			}
		}
	}
}


function nucleiSelect() {

	if( selectNuc ) {
		$.ajax({
	        type:   "POST",
            url:    "db/getsingle.php",
            dataType: "json",
            data:   { slide:    curSlide,
                      cellX:    Math.round(statusObj.mouseImgX()),
                      cellY:    Math.round(statusObj.mouseImgY()),
											application: application,
            },
            success: function(data) {
					if( data !== null ) {

						var total = statusObj.posSel() + statusObj.negSel();

						if( total < 8 ) {

							sample = {};

							// Distance from nuclei is element 4
							sample['slide'] = curSlide;
							sample['id'] = data[1];
							sample['centX'] = data[2];
							sample['centY'] = data[3];
							sample['boundary'] = data[0];
							sample['maxX'] = data[4];
							sample['maxY'] = data[5];
							sample['scale'] = data[6];

							// check if a sample selected is duplicated or not
							if (!duplicateCheck(sample['centX'], sample['centY'])){

								var cell = document.getElementById("N"+sample['id']);
								var currentID = sample['id'];
								var undo = false;

								if( statusObj.posSel() < 4 ) {
									sample['label'] = 1;
									statusObj.posSel(statusObj.posSel() + 1);
								} else if( statusObj.negSel() < 4 ) {
									sample['label'] = -1;
									statusObj.negSel(statusObj.negSel() + 1);
								}

								total = statusObj.posSel() + statusObj.negSel();
								// if there is no sample selected, update color
								if (selectedJSON.length == 0){
									selectedJSON.push(sample);
									updateBoundColors(currentID);
								}	else {
									if (sample['label'] == 1){
										selectedJSON.splice(statusObj.posSel()-1, 0, sample);
									}	else{
										selectedJSON.splice(total-1, 0, sample);
									}
									// if the color is aqua, then update color
									if( cell.getAttribute('stroke') === "aqua" ) {
										updateBoundColors(currentID);
									}
								}
								// display current samples
								displayThumbNail();

								if( statusObj.posSel()  > 3 ) {
									// make sure instructions are updated
									$('#instruct').text("Selecting "+negClass+" samples");
								}	else {
									$('#instruct').text("Selecting "+posClass+" samples");
								}
							}	else {
							window.alert("Selected sample is duplicted !!");
						}
					}	else {
						window.alert("All samples selected, click prime button to submit");
					}
				}
			}
  	});
	}
}
//
//	+++++++++++    Openseadragon mouse event handlers  ++++++++++++++++
//
//
//	Mouse enter event handler for viewer
//
//
function onMouseEnter(event) {
	statusObj.haveMouse(true);
}


//
// Mouse move event handler for viewer
//
//
function onMouseMove(event) {
	var offset = osdCanvas.offset();

	statusObj.mouseRelX(event.pageX - offset.left);
	statusObj.mouseRelY(event.pageY - offset.top);
	statusObj.mouseImgX(imgHelper.physicalToDataX(statusObj.mouseRelX()));
	statusObj.mouseImgY(imgHelper.physicalToDataY(statusObj.mouseRelY()));
}


//
//	Mouse leave event handler for viewer
//
//
function onMouseLeave(event) {
	statusObj.haveMouse(false);
}




//
//	The double click handler doesn't seem to work. So we create
//	our own with a timer.
//
function onMouseClick(event) {

	if (application =="region"){
		event.preventDefaultAction = true;
	}
	clickCount++;
	if( clickCount === 1 ) {
		// If no click within 250ms, treat it as a single click
		singleClickTimer = setTimeout(function() {
					// Single click
					clickCount = 0;
				}, 250);
	} else if( clickCount >= 2 ) {
		// Double click
		clearTimeout(singleClickTimer);
		clickCount = 0;
		nucleiSelect();
	}
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//
//	+++++++++++++	Button handlers +++++++++++++++++++++++++++++++++++++++++++
//

//
//	Set the display flag, update the segmentation button text and
//	set the visibility of the SVG element.
//
function showSegmentation() {

	var	segBtn = $('#segBtn');

	if( displaySeg ) {
		// Currently displaying segmentation, hide it
		segBtn.val("Show Segmentation");
		$('.overlaySvg').css('visibility', 'hidden');
		displaySeg = false;
	} else {
		// Segmentation not currently displayed, show it
		segBtn.val("Hide Segmentation");
		$('.overlaySvg').css('visibility', 'visible');
		displaySeg = true;

		updateSeg();
	}
}




//
//	Pass the selected nuclei to the active learning server and
//	start the "select / label" process.
//
function primeSession() {

	if( statusObj.posSel() != 4 ) {
		window.alert("Need to select 4 "+ posClass+" examples");
	} else if( statusObj.negSel() != 4 ) {
		window.alert("Need to select 4 "+ negClass+" examples");
	} else {

		$('#progDiag').modal('show');

		// No need to send boundaries to the server
		for( i = 0; i < selectedJSON.length; i++ ) {
			selectedJSON[i]['boundary'] = "";
		}

		// Submit to active learning server
		$.ajax({
			type: "POST",
			url: "php/primeSession.php",
			data: {samples: selectedJSON},
			dataType: "json",
			success: function(data) {
				if( data === "PASS" ) {
					window.location = "grid.html?application="+application;
				} else {
					// TODO - Indicate failure
				}
			}
		});
	}
}




//
// Toggles the nuclei selection process. A total of 8 nuclei need to be
//	selected. 4 from each class. They can only be selected if in the
//	'selection' mode.
//
function setSelectMode() {

	var	selBtn = $('#selBtn');
	if( selectNuc ) {
		// Currently selecting nuclei, stop
		selBtn.val("Select Nuclei");
		selBtn.css('color', 'white');
		selectNuc = false;
		$('#instruct').text("");
	} else {
		// Not currently selecting nuclei, start
		selBtn.val("Stop Selecting");
		selBtn.css('color', 'red');
		selectNuc = true;

		if( statusObj.posSel() < 4 ) {
			$('#instruct').text("Selecting "+posClass+" samples");
		} else if( statusObj.negSel() < 4 ) {
			$('#instruct').text("Selecting "+negClass+" samples");
		} else {
			window.alert("All samples selected");
			selectNuc = false;
		}
	}
}



//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}




function cancelSession() {
	$.ajax({
		url: "php/cancelSession.php",
		data: "",
		success: function() {
			window.location = "index_home.html?application="+application;
		}
	});
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//
// Image data we want knockout.js to keep track of
//
var statusObj = {
	haveImage: ko.observable(false),
	haveMouse: ko.observable(false),
	imgAspectRatio: ko.observable(0),
	imgWidth: ko.observable(0),
	imgHeight: ko.observable(0),
	mouseRelX: ko.observable(0),
	mouseRelY: ko.observable(0),
	mouseImgX: ko.observable(0),
	mouseImgY: ko.observable(0),
	scaleFactor: ko.observable(0),
	viewportX: ko.observable(0),
	viewportY: ko.observable(0),
	viewportW: ko.observable(0),
	viewportH: ko.observable(0),
	dataportLeft: ko.observable(0),
	dataportTop: ko.observable(0),
	dataportRight: ko.observable(0),
	dataportBottom: ko.observable(0),
	posSel: ko.observable(0),
	negSel: ko.observable(0)
};


var svgOverlayVM = {
	annoGrpTranslateX:	ko.observable(0.0),
	annoGrpTranslateY:	ko.observable(0.0),
	annoGrpScale: 		ko.observable(1.0),
	annoGrpTransform:	annoGrpTransformFunc
};

var vm = {
	statusObj:	ko.observable(statusObj),
	svgOverlayVM: ko.observable(svgOverlayVM)
};


// Apply binfding for knockout.js - Let it keep track of the image info
// and mouse positions
//
ko.applyBindings(vm);
