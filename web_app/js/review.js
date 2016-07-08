//
//	Copyright (c) 2014-2015, Emory University
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
var uid = "";
var classifier = "";
var negClass = "";
var posClass = "";
var IIPServer = "";
var SlideSuffix = ".svs-tile.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";

var viewer = null, imgHelper = null, osdCanvas = null;
var olDiv = null;

var lastScaleFactor = 0;
var	sampleDataJson = "";

var curDataset;
var curBox = -1;
var curX = 0, curY = 0;

var boundaryOn = true;
var segDisplayOn = true;

var igrArray = new Array();

var slideLists = [];
var cellIndex = [];

//
//	Review
//
//		Get a list of the selected cells from the database
//

$(function() {


	// get slide host info
	//
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {

			uid = data['uid'];
			IIPServer = data['IIPServer'];
			posClass = data['posClass'];
			negClass = data['negClass'];

			if( uid === null ) {
				window.alert("No session active");
				window.history.back();
			}
			else{
				// will be used when the class names are requried.
				//$("#posHeader1").text("Positive class : "+ posClass);
				//$("#negHeader1").text("Negative class : "+ negClass);
			}
		}
	});


// Create the slide zoomer, update slide count etc...
// We will load the tile pyramid after the slide list is loaded
//
viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "slideZoom", prefixUrl: "images/", animationTime: 0.1});
imgHelper = viewer.activateImagingHelper({onImageViewChanged: onImageViewChanged});

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

	// Zoom and pan to selected nuclei
	homeToNuclei();
});



viewer.addHandler('close', function(event) {
	osdCanvas = $(viewer.canvas);
	statusObj.haveImage(false);

	//viewer.drawer.clearOverlays();
			osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
			osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
			osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

	osdCanvas = null;
	statusObj.curSlide("");

});


	viewer.addHandler('animation-finish', function(event) {

		var annoGrp = document.getElementById('annoGrp');
		var sampGrp = document.getElementById('sample');

		if( sampGrp != null ) {
			sampGrp.parentNode.removeChild(sampGrp);
		}

		if( annoGrp != null ) {
			sampGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
			sampGrp.setAttribute('id', 'sample');
			annoGrp.appendChild(sampGrp);

			ele = document.createElementNS("http://www.w3.org/2000/svg", "rect");

			ele.setAttribute('x', curX - 50);
			ele.setAttribute('y', curY - 50);
			ele.setAttribute('width', 100);
			ele.setAttribute('height', 100);
			ele.setAttribute('stroke', 'yellow');
			ele.setAttribute('fill', 'none');
			ele.setAttribute('stroke-width', 4);
			ele.setAttribute('id', 'boundBox');

			sampGrp.appendChild(ele);


			ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
			ele.setAttribute('points', sampleDataJson['review'][curBox]['boundary']);
			ele.setAttribute('id', 'boundary');
			ele.setAttribute('stroke', 'yellow');
			ele.setAttribute('fill', 'none');
			if( boundaryOn ) {
				ele.setAttribute('visibility', 'visible');
				// Make sure toggle button refects the correct action and is enabled
				$('#toggleBtn').val("Hide segmentation");
				$('#toggleBtn').removeAttr('disabled');
			} else {
				ele.setAttribute('visibility', 'hidden');
			}
			sampGrp.appendChild(ele);

			$('.overlaySvg').css('visibility', 'visible');
		}
	});

	if( uid === null ) {
		window.alert("No session active");
		window.history.back();
	} else {
		genReview();
	}
});

// Do review
// the selected samples are retrieved by calling reviewSamples.php
//
function genReview() {
  $.ajax({
    url: "php/reviewSamples.php",
    data: "",
    dataType: "json",
    success: function(data) {

      sampleDataJson = data;

			// Clear the slide viewer if there's something showing
			//
			if( statusObj.curSlide() != "" ) {

				viewer.close();
				statusObj.curSlide("");
			};

			// sort by slide name
			sampleDataJson['review'].sort(function(a, b) {
			  var nameA = a.slide.toUpperCase();
			  var nameB = b.slide.toUpperCase();
			  if (nameA < nameB) {
			    return -1;
			  }
			  if (nameA > nameB) {
			    return 1;
			  }
			  // names must be equal
			  return 0;
			});

			// 1'st line information
			slidesInfo(sampleDataJson['review']);
			// 2'nd line information
			displaySlidesamples(sampleDataJson['review']);
			// default view
			thumbSingleClick(0);
    },
    error: function() {
      console.log("Review failed");
    }
  });
}

// Gets slides information
// Parameter: array-like
// the selected sample information in array
// Return
// displays the information of the selected cells
//
function slidesInfo(sampleArray) {

	var totalNumofSlides = 0;
	var totalNumofCells = sampleArray.length;
	var totalNumofPositive = 0;
	var totalNumfofNegative = 0;
	var array_slide = [];

	for( sample in sampleArray ) {
		if (sampleArray[sample]['label'] === 1){
			totalNumofPositive = totalNumofPositive + 1;
		}
		else if (sampleArray[sample]['label'] === -1){
			totalNumfofNegative = totalNumfofNegative + 1;
		}
		array_slide.push(sampleArray[sample]['slide']);
	}

	totalNumofSlides = counts(array_slide);

	for( var i=0; i<totalNumofSlides; i++) {
		cellIndex.push([]);
	}

	var	container, row, linebreak;
	container = document.getElementById('posHeader1');
	row = document.createElement("div");
	row.setAttribute('class','row');
	row.innerHTML = "<font size=3> <b> Total slides : </b> " + totalNumofSlides + " <br>" +
		"<b> Total cells : </b> " + totalNumofCells + " <br>" +
		"<b> Total positive cells : </b> " + totalNumofPositive + " <br>" +
		"<b> Total negative cells : </b> " + totalNumfofNegative;
	container.appendChild(row);

}

// Gets object counts
// Parameter: array-like
// an object information
// Return: int
// number of objects
//
function counts(array){
	var counts = {};
	var totalNum = 0;

	array.forEach(function(element) {
	  counts[element] = (counts[element] || 0) + 1;
	});

	totalNum = Object.keys(counts).length;

	return totalNum;
}


// Displays samples for each slide
// Parameter: array-like
// the selected sample information in array
// Return
// display the selected samples for each slide
//
function displaySlidesamples(sampleArray){

	var slideObject = [];
	var slideName = "";
	var slideNum = 0;
	var isFirst = true;
	slideLists = [];

	// splits slides
	for( sample in sampleArray ) {
		if (isFirst){
			slideObject = [];
			slideName = sampleArray[sample]['slide'];
			slideObject.push(sampleArray[sample]);
			isFirst = false;
		}
		else{
			if( slideName != sampleArray[sample]['slide']){
				slideLists.push(slideObject);
				slideObject = [];
				slideName = sampleArray[sample]['slide'];
				slideObject.push(sampleArray[sample]);
				slideNum = slideNum + 1;
				cursampleIndex = 0;
			}
			else{
				slideObject.push(sampleArray[sample]);
			}
		}
		cellIndex[slideNum].push(sample);
	}

	if(slideObject.length > 0){
		slideLists.push(slideObject);
	}

	var	reviewSel = $("#reviewSel");
	reviewSel.append(new Option("All", "All"));
	for( var i=0; i < slideLists.length; i++ ) {
		reviewSel.append(new Option(slideLists[i][0]['slide'], slideLists[i][0]['slide']));
	}
	// display cells for each slide
	for( var i=0; i < slideLists.length; i++ ) {
		displayOneslide(slideLists[i], i, cellIndex[i]);
	}
}

// Runs when selecting a slide and displays the selected cells in the slide
// Parameter: None
// Return: None
//
function doreviewSel(){
	var slideName = document.getElementById("reviewSel").value;
	clearPosNeg();
	if (slideName == "All"){
		// display all
		for( var i=0; i < slideLists.length; i++ ) {
			displayOneslide(slideLists[i], i, cellIndex[i]);
		}
	}
	else{
		// display the select slide
		for( var i=0; i < slideLists.length; i++ ) {
			if (slideName == slideLists[i][0]['slide']){
				displayOneslide(slideLists[i], i, cellIndex[i]);
			}
		}
	}
}

// Clears current positive and negative divs
// Parameter: None
// Return: None
//
function clearPosNeg(){
	document.getElementById('pos').innerHTML = "";
	document.getElementById('neg').innerHTML = "";
}

// Displays samples for one slide
// Parameter: array-like, int
// one slide and the selected samples for the slide
// Return
// display the selected samples for the slide
//
function displayOneslide(sampleArray, slideNum, sampleIndex){
	// gets a slide information
	slideInfo(sampleArray[0], sampleArray.length);

	var isPos = 1;
	var posNewline = false;
	var negNewline = false;
	var poscolNum = 0;
	var negcolNum = 0;
	var posRow = 0;
	var negRow = 0;
	var isFirstLine = true;
	var isNewLine = false;
	var lastsample = 0;

	for( sample in sampleArray ) {
		if( sampleArray[sample]['label'] === 1 ) {
			isPos = 1;
			posNewline = false;
			var remainder = (poscolNum % 4);
			if(remainder === 0) {
				posNewline = true;
				posRow = posRow + 1;
			}
			// displays postive samples
			addPos(sampleIndex[sample], slideNum, posNewline, sample, posRow, sampleArray[sample]);

			var labelTag = "#label_"+isPos+'_'+sample+'_'+slideNum;
			var label = $('#box_'+isPos+'_'+sample+'_'+slideNum).children(".classLabel");

			$(labelTag).text(posClass);
			label.addClass("posLabel");

			poscolNum = poscolNum + 1

		} else if( sampleArray[sample]['label'] === -1 ) {
			isPos = 0;
			negNewline = false;
			var remainder = (negcolNum % 4);
			if(remainder === 0) {
				negNewline = true;
				negRow = negRow + 1;
			}
			// displays negative samples
			addNeg(sampleIndex[sample], slideNum, negNewline, sample, negRow, sampleArray[sample]);

			var labelTag = "#label_"+isPos+'_'+sample+'_'+slideNum;
			var label = $('#box_'+isPos+'_'+sample+'_'+slideNum).children(".classLabel");

			$(labelTag).text(negClass);
			label.addClass("negLabel");

			negcolNum = negcolNum + 1
		} else {
			//igrArray.push(sampleArray);
		}
		lastsample = lastsample + 1;
	}
	// check if there's no pos or neg box
	var diff = poscolNum - negcolNum;
	if (diff > 0){
		box = 'neg';
		isPos = 0;
		for (var k=0; k<diff; k++){
				negNewline = false;
				var remainder = (negcolNum % 4);
				if(remainder === 0) {
					negNewline = true;
					negRow = negRow + 1;
				}
				emptybox(box, slideNum, negNewline, negRow, lastsample, isPos);
				negcolNum = negcolNum + 1;
				lastsample = lastsample + 1;
		}
	}
	else if (diff < 0){
		box = 'pos';
		isPos = 1;
		for (var k=0; k<Math.abs(diff); k++){
				posNewline = false;
				var remainder = (poscolNum % 4);
				if(remainder === 0) {
					posNewline = true;
					posRow = posRow + 1;
				}
				emptybox(box, slideNum, posNewline, posRow, lastsample, isPos);
				poscolNum = poscolNum + 1;
				lastsample = lastsample + 1;
		}
	}
	else{
		// requried for the ignored samples if needed.
	}

}

// Gets a slide information
// Parameter: array-like, int
// number of samples and the selected sample information in array
// Return
// displays the information of the selected cells
//
function slideInfo(sampleSlide, cellnum) {

	var	container, row, linebreak;
	container = document.getElementById('pos');
	row = document.createElement("div");
	row.setAttribute('class','row');
	row.innerHTML = "<hr> <b> Slide name : </b> " + sampleSlide['slide'] + " <br/>";
	container.appendChild(row);

	container = document.getElementById('neg');
	row = document.createElement("div");
	row.setAttribute('class','row');
	row.innerHTML = "<hr> <b> Numer of the selected cells :</b> " + cellnum + " <br/>";
	container.appendChild(row);

}

// Displays an empty box for each sample
// Parameter: string, int, bool, int, int, bool
// postive or negative, slide number, new line checker, sample number, pos checker
// Return
// displays an empty boxe
//
function emptybox(box, slideNum, isNewline, rownum, sample, isPos) {

	var	container, row, col, im, img;

	container = document.getElementById(box);

	if (isNewline){
		row = document.createElement("div");
		row.setAttribute('class', 'row');
		row.setAttribute('id', box+'Row'+'_'+rownum+'_'+ slideNum);
	}
	else{
			row = document.getElementById(box+'Row'+'_'+rownum+'_'+ slideNum);
	}

	col = document.createElement("div");
	col.setAttribute('class', 'review_div');
	col.setAttribute('id', 'box_'+isPos+'_'+sample+'_'+slideNum);

	// Make sure overlay is hidden
	$('.overlaySvg').css('visibility', 'hidden');

	row.appendChild(col);
	container.appendChild(row);

	// clean border color
	var bcolor = document.getElementById('box_'+isPos+'_'+sample+'_'+slideNum);
	$(bcolor).css('border-color', 'white');

}

// Displays a positive sample
// Parameter: int, int, bool, int, int, object
// index for all samples, slide number, new line checker, sample number, row number, sample object
// Return
// displays a positive sample
//
function addPos(index, slideNum, isNewline, sample, rowNo, pos) {

	var	container, row, over, col, im, img, label, docFrag;
	var slide, scale, centX, centY, sizeX, sizeY, loc;
	var isPos = 1;

	container = document.getElementById('pos');

	if (isNewline){
			row = document.createElement("div");
			row.setAttribute('id', 'posRow'+'_'+rowNo+'_'+slideNum);
			row.setAttribute('class', 'row');
	}
	else{
			row = document.getElementById('posRow'+'_'+rowNo+'_'+slideNum);
	}

	col = document.createElement("div");
	col.setAttribute('class', 'review_div');
	col.setAttribute('id', 'box_'+isPos+'_'+sample+'_'+slideNum);

	docFrag = document.createDocumentFragment();

	label = document.createElement("div");
	label.setAttribute('class', 'classLabel');
	label.setAttribute('id', 'label_'+isPos+'_'+sample+'_'+slideNum);

	img = document.createElement('div');
	//img.setAttribute('class','col-sm-2 col-md-2 col-lg-2');
	im = document.createElement('img');
	im.setAttribute('id','thumb_'+isPos+'_'+sample+'_'+slideNum);
	im.setAttribute('width',90);
	im.setAttribute('height',90);

	scale = pos['scale'];
	slide = pos['slide'];

	centX = (pos['centX'] - (25.0 * scale)) / pos['maxX'];
	centY = (pos['centY'] - (25.0 * scale)) / pos['maxY'];

	sizeX = (50.0 * scale) / pos['maxX'];
	sizeY = (50.0 * scale) / pos['maxY'];
	loc = centX+","+centY+","+sizeX+","+sizeY;

	thumbNail = IIPServer+"FIF="+pos['path']+SlideLocPre+loc+"&WID=100"+SlideLocSuffix;

	im.setAttribute('src',thumbNail);

	img.appendChild(im);

	docFrag.appendChild(label);
	docFrag.appendChild(img);

	// Make sure overlay is hidden
	$('.overlaySvg').css('visibility', 'hidden');

	col.appendChild(docFrag);
	row.appendChild(col);
	container.appendChild(row);

	// add click
	var	box = document.getElementById('box_'+isPos+'_'+sample+'_'+slideNum);
	var	clickCount = 0;

	box.addEventListener('click', function() {
		clickCount++;
		if( clickCount === 1 ) {
			singleClickTimer = setTimeout(function() {
				clickCount = 0;
				thumbSingleClick(index);
			}, 200);
		}	else if( clickCount === 2 ) {
			clearTimeout(singleClickTimer);
			clickCount = 0;
			thumbDoubleClick(index);
		}
	}, false);
}

// Displays a negative sample
// Parameter: int, int, bool, int, int, object
// index for all samples, slide number, new line checker, sample number, row number, sample object
// Return
// displays a negitive sample
//
function addNeg(index, slideNum, isNewline, sample, rowNo, neg) {

	var	container, row, col, im, img, label, docFrag;
	var slide, scale, centX, centY, sizeX, sizeY, loc;
	var isPos = 0;

	container = document.getElementById('neg');

	if (isNewline){
			row = document.createElement("div");
			row.setAttribute('id', 'negRow'+'_'+rowNo+'_'+slideNum);
			row.setAttribute('class', 'row');
	}
	else{
			row = document.getElementById('negRow'+'_'+rowNo+'_'+slideNum);
	}


	col = document.createElement("div");
	col.setAttribute('class', 'review_div');
	col.setAttribute('id', 'box_'+isPos+'_'+sample+'_'+slideNum);

	docFrag = document.createDocumentFragment();

	label = document.createElement("div");
	label.setAttribute('class', 'classLabel');
	label.setAttribute('id', 'label_'+isPos+'_'+sample+'_'+slideNum);

	img = document.createElement('div');
	//img.setAttribute('class','col-sm-2 col-md-2 col-lg-2');
	im = document.createElement('img');
	im.setAttribute('id','thumb_'+isPos+'_'+sample+'_'+slideNum);
	im.setAttribute('width',90);
	im.setAttribute('height',90);

	scale = neg['scale'];
	slide = neg['slide'];

	centX = (neg['centX'] - (25.0 * scale)) / neg['maxX'];
	centY = (neg['centY'] - (25.0 * scale)) / neg['maxY'];

	sizeX = (50.0 * scale) / neg['maxX'];
	sizeY = (50.0 * scale) / neg['maxY'];
	loc = centX+","+centY+","+sizeX+","+sizeY;

	thumbNail = IIPServer+"FIF="+neg['path']+SlideLocPre+loc+"&WID=100"+SlideLocSuffix;

	im.setAttribute('src',thumbNail);

	img.appendChild(im);

	docFrag.appendChild(label);
	docFrag.appendChild(img);

	// Make sure overlay is hidden
	$('.overlaySvg').css('visibility', 'hidden');

	col.appendChild(docFrag);
	row.appendChild(col);
	container.appendChild(row);

	// add click
	var	box = document.getElementById('box_'+isPos+'_'+sample+'_'+slideNum);
	var	clickCount = 0;

	box.addEventListener('click', function() {
		clickCount++;
		if( clickCount === 1 ) {
			singleClickTimer = setTimeout(function() {
				clickCount = 0;
				thumbSingleClick(index);
			}, 200);
		} else if( clickCount === 2 ) {
			clearTimeout(singleClickTimer);
			clickCount = 0;
			thumbDoubleClick(index);
		}
	}, false);
}

// ThumbNamil one click function
//
function thumbSingleClick(index) {
	// Load the appropriate slide in the viewer
	var newSlide = sampleDataJson['review'][index]['slide'];
	curX = Math.round(sampleDataJson['review'][index]['centX']);
	curY = Math.round(sampleDataJson['review'][index]['centY']);

	if( statusObj.curSlide() == "" ) {
		statusObj.curSlide(newSlide);
		statusObj.currentX(curX);
		statusObj.currentY(curY);
		updateSlideView();
	}
	else if( statusObj.curSlide() != newSlide ) {
			viewer.close();
			statusObj.curSlide(newSlide);
			statusObj.currentX(curX);
			statusObj.currentY(curY);
			updateSlideView();
	}
	else {
		// On same slide,, no need to load it again
		statusObj.currentX(curX);
		statusObj.currentY(curY);
		homeToNuclei();
	}
	curBox = index;
	boundaryOn = true;
};

//
//	A double click in the thumbnail box toggles the current classification
//	of the object.
//
//
function thumbDoubleClick(index) {

	var label = sampleDataJson['review'][index]['label'];

	// Toggle through the 3 states, pos, neg and ignore
	//
	if( label === 1 ) {
		sampleDataJson['review'][index]['label'] = -1;
	} else if( label === -1 ) {
		sampleDataJson['review'][index]['label'] = 1;
	} else {
		sampleDataJson['review'][index]['label'] = 0;
	}
	doreviewSel();
	updateLabels();
};


//	Updates labels in al server
//
function updateLabels() {

	// No need to send boundaries to the server
	for( i = 0; i < sampleDataJson['review'].length; i++ ) {
		sampleDataJson['review'][i]['boundary'] = "";
	}

	$.ajax({
		type: "POST",
		url: "php/saveReview.php",
		dataType: "json",
		data: sampleDataJson,
		success: function() {

			// Get a new set of samples
			//updateSamples();
		}
	});
}

// Update slide view when mouse button is clicked on thumbNail
//
function updateSlideView() {


	$.ajax({
		type: "POST",
		url: "db/getPyramidPath.php",
		dataType: "json",
		data: { slide: statusObj.curSlide() },
		success: function(data) {

				// Zoomer needs '.dzi' appended to the end of the filename
				pyramid = "DeepZoom="+data[0]+".dzi";
				viewer.open(IIPServer + pyramid);
		}
	});
}


function homeToNuclei() {

	// Zoom in all the way
	viewer.viewport.zoomTo(viewer.viewport.getMaxZoom());
	// Move to nucei
	imgHelper.centerAboutLogicalPoint(new OpenSeadragon.Point(imgHelper.dataToLogicalX(curX),
															  imgHelper.dataToLogicalY(curY)));
}


function toggleSegVisibility() {

	var boundary = document.getElementById('boundary');
	if( boundary != null ) {

		if( boundaryOn ) {
			$('#toggleBtn').val("Show segmentation");
			boundary.setAttribute('visibility', 'hidden');
			boundaryOn = false;
		} else {
			$('#toggleBtn').val("Hide segmentation");
			boundary.setAttribute('visibility', 'visible');
			boundaryOn = true;
		}
	}
}

//-----------------------------------------------------------------------------


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






function updateOverlayInfo() {

	// Only update the scale of the svg if it has changed. This speeds up
	// scrolling through the image.
	//
	if( lastScaleFactor != statusObj.scaleFactor() ) {
		lastScaleFactor = statusObj.scaleFactor();
		var annoGrp = document.getElementById('anno');

		if( annoGrp != null ) {
			var scale = "scale(" + statusObj.scaleFactor() + ")";
			annoGrp.setAttribute("transform", scale);
		}
	}
}
//
// ===============	Mouse event handlers for viewer =================
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

	statusObj.mouseX(imgHelper.dataToLogicalX(offset.left));
	statusObj.mouseY(imgHelper.dataToLogicalX(offset.top));
	statusObj.mouseRelX(event.pageX - offset.left);
	statusObj.mouseRelY(event.pageY - offset.top);
	statusObj.mouseImgX(imgHelper.physicalToDataX(statusObj.mouseRelX()));
	statusObj.mouseImgY(imgHelper.physicalToDataY(statusObj.mouseRelY()));
	statusObj.mouseLogX(imgHelper.dataToLogicalX(statusObj.mouseImgX()));
	statusObj.mouseLogY(imgHelper.dataToLogicalY(statusObj.mouseImgY()));
}





//
//	Mouse leave event handler for viewer
//
//
function onMouseLeave(event) {
	statusObj.haveMouse(false);
}





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
	mouseLogX: ko.observable(0),
	mouseLogY: ko.observable(0),
	mouseX: ko.observable(0),
	mouseY: ko.observable(0),
	scaleFactor: ko.observable(0),
	viewportX: ko.observable(0),
	viewportY: ko.observable(0),
	viewportW: ko.observable(0),
	viewportH: ko.observable(0),
	dataportLeft: ko.observable(0),
	dataportTop: ko.observable(0),
	dataportRight: ko.observable(0),
	dataportBottom: ko.observable(0),
	iteration:	ko.observable(0),
	accuracy:	ko.observable(0.0),
	imgReady: ko.observable(false),
	curSlide: ko.observable(""),
	currentX: ko.observable(0),
	currentY: ko.observable(0),
	posSel: ko.observable(0),
	negSel: ko.observable(0),
	totalSel: ko.observable(0),
	testSetCnt: ko.observable(0),
	selectSampleslide: ko.observable(0),
	selectSampleid: ko.observable(0),
	selectSamplecentX: ko.observable(0),
	selectSamplecentY: ko.observable(0),
	selectSampleboundary: ko.observable(0),
	selectSamplemaxX: ko.observable(0),
	selectSamplemaxY: ko.observable(0),
	selectSamplescale: ko.observable(0),
	selectSamplelabel: ko.observable(0),
	selectSampleclickX: ko.observable(0),
	selectSampleclickY: ko.observable(0)

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
