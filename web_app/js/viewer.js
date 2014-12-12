var annoGrpTransformFunc;
var IIPServer="";
var slideCnt = 0;
var curSlide = "";
var curDataset = "";

var viewer = null;
var imgHelper = null, osdCanvas = null, viewerHook = null;
var overlayHidden = false, selectMode = false, segDisplayOn = false;;
var olDiv = null;
var lastScaleFactor = 0;
var pyramids, trainingSets;
var clickCount = 0;					

// The following only needed for active sessions
var uid = null, classifier = "", negClass = "", posClass = "";			





//
//	Initialization
//	
//		Get a list of available slides from the database
//		Populate the selection and classifier dropdowns
//		load the first slide
//		Register event handlers
//
$(function() {
	
	// Create the slide zoomer, update slide count etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "image_zoomer", prefixUrl: "images/", animationTime: 0.1});
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
	});



	viewer.addHandler('close', function(event) {
		statusObj.haveImage(false);
		
        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
        osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
	});

	
	viewer.addHandler('animation-finish', function(event) {

		if( segDisplayOn ) {
		
			if( statusObj.scaleFactor() > 0.5 ) {
				$('.overlaySvg').css('visibility', 'visible');
				updateSeg();
			} else {
				$('.overlaySvg').css('visibility', 'hidden');
			}
		}
	});

	// get slide host info
	//
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			uid = data['uid'];
			classifier = data['className'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			IIPServer = data['IIPServer'];
			curDataset = data['dataset'];
			
			// Don't display the legend until a classifier is selected
			$('#legend').hide();

			if( uid === null ) {
				// No active session, don;t allow navigation to select & visualize
				$('#nav_select').hide();
				$('#nav_visualize').hide();
			} else {
				// Active session, dataset selection not allowed
				document.getElementById('dataset_sel').disabled = true
			}
			// Slide list and classifier list will also be updated by this call
			updateDatasetList();
		}
	});

	
	// Set the update handlers for the selectors
	$("#slide_sel").change(updateSlide);
	$("#dataset_sel").change(updateDataset);
	$("#classifier_sel").change(updateClassifier);
});






//
//	Get the url for the slide pyramid and set the viewer to display it
//
//
function updatePyramid() {

	slide = "";

	if( pyramids[$('#slide_sel').prop('selectedIndex')] === null ) {	
	
		// Slides that don't have their path in the database use the digital slide
		// archive. Eventually all slides will be retrived from the local image server, allowing
		// this code to be removed.
		//
		var dataviewUrl="http://cancer.digitalslidearchive.net/local_php/get_slide_list_from_db_groupid_not_needed.php"
		var slideUrl = dataviewUrl+'?slide_name_filter=' + curSlide;
		console.log("Slide data: " + slideUrl);
		
		$.ajax({ 
			type: 	"GET",
			url: 	slideUrl,
			dataType:	"xml",
			success: function(xml) {

				pyramid = $(xml).find("slide_url").text();
				// Slides from node15 have /cgi-bin/iipsrv.fcgi? as part of their path
				// we need to remove it.
				// This will all go away when all slides are migrated to the new server
				var pos = pyramid.indexOf('?');
				pyramid = pyramid.substring(pos + 1);

				console.log("Loading: " + IIPServer + pyramid);
				viewer.open(IIPServer + pyramid);
			}, 
			error: function() {
				alert("Unable to get slide information");	
			}
		});
	} else {
		// Zoomer needs '.dzi' appended to the end of the filename
		pyramid = "DeepZoom="+pyramids[$('#slide_sel').prop('selectedIndex')]+".dzi";
		console.log("Loading: " + IIPServer + pyramid);
		viewer.open(IIPServer + pyramid);
	}
}




//
//	Updates the dataset selector
//
function updateDatasetList() {
	var	datasetSel = $("#dataset_sel");

	// Get a list of datasets
	$.ajax({
		url: "db/getdatasets.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			for( var item in data ) {
				datasetSel.append(new Option(data[item], data[item]));
			}

			if( curDataset === null ) {
				curDataset = data[0];		// Use first dataset initially
			} else {
				datasetSel.val(curDataset);
			}
									
			// Need to update the slide list since we set the default slide
			updateSlideList();
			
			// Classifier list needs the current dataset
			updateClassifierList();
		}
	});
}





//
//	Updates the list of available slides for the current dataset
//
function updateSlideList() {
	var slideSel = $("#slide_sel");
	var slideCntTxt = $("#count_patient");

	console.log("Getting slides for: "+curDataset);
	
	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: curDataset },
		dataType: "json",
		success: function(data) {

			pyramids = data['paths'];
			curSlide = String(data['slides'][0]);		// Start with the first slide in the list
			slideCnt = Object.keys(data['slides']).length;;
			slideCntTxt.text(slideCnt);

			slideSel.empty();
			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data['slides'] ) {			
				slideSel.append(new Option(data['slides'][item], data['slides'][item]));
			}

			// Get the slide pyrimaid and display	
			updatePyramid();
		}
	});
}





//
//	Updates the classifier selector
//
function updateClassifierList() {
	var classSel = $("#classifier_sel");

	classSel.empty();
	
	// First selection should be none
	classSel.append(new Option('----------------', 'none'));
	console.log("UID: "+uid);
	console.log("Current dataset: "+curDataset);
	
	if( uid === null ) {
		$.ajax({
			type: "POST",
			url: "db/getTrainingSets.php",
			data: { dataset: curDataset },
			dataType: "json",
			success: function(data) {
			
				trainingSets = data;
				for( var item in data['trainingSets'] ) {			
					classSel.append(new Option(data['trainingSets'][item], data['trainingSets'][item]));
				}
			}
		});
		
	} else {
		classSel.append(new Option('Current', 'current'));		
	}
}




//
//	A new slide has been selected from the drop-down menu, update the 
// 	slide zoomer.
//
//
function updateSlide() {
	curSlide = $('#slide_sel').val();
	updatePyramid();
}





//
//
//
//
function updateDataset() {

	curDataset = $('#dataset_sel').val();
	updateSlideList();
	updateClassifierList();
}



//
//	Update boundaries, if visible, to the appropriate colors based on
//	the selected classifier.
//
//
function updateClassifier() {

	var class_sel = document.getElementById('classifier_sel'),
		classifier = class_sel.options[class_sel.selectedIndex].value;

	console.log("Classifier changed to: "+classifier);
	
	if( class_sel.selectedIndex != 0 ) {
							
		box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:deeppink;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
		document.getElementById('negLegend').innerHTML = box + " " + negClass;
		box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:lime;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
		document.getElementById('posLegend').innerHTML = box + " " + posClass;
		
		$('#legend').show();
	} else {
	
		$('#legend').hide();
	}
	
	if( overlayHidden === false ) {
	
		updateSeg();
	}
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

	if( statusObj.scaleFactor() > 0.5 ) {
	
		var left, right, top, bottom, width, height;

		// Grab nuclei a viewport width surrounding the current viewport
		//	+++ FIX ME !!!! +++
		width = statusObj.dataportRight() - statusObj.dataportLeft();
		height = statusObj.dataportBottom() - statusObj.dataportTop();
		
		left = (statusObj.dataportLeft() - width > 0) ?	statusObj.dataportLeft() - width : 0;
		right = statusObj.dataportRight() + width;
		top = (statusObj.dataportTop() - height > 0) ?	statusObj.dataportTop() - height : 0;
		bottom = statusObj.dataportBottom() + height;
		
		var class_sel = document.getElementById('classifier_sel'),
			classifier = class_sel.options[class_sel.selectedIndex].value;
		console.log("Current classifier: "+classifier);
		console.log("Current Slide: "+curSlide);
		
	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { uid:	uid,
					slide: 	curSlide,
					left:	statusObj.dataportLeft(),
					right:	statusObj.dataportRight(),
					top:	statusObj.dataportTop(),
					bottom:	statusObj.dataportBottom(),
					dataset: curDataset,
					trainset: classifier
			},
		
			success: function(data) {
					
					var ele;
					var segGrp = document.getElementById('segGrp');
					var annoGrp = document.getElementById('anno');

					// If group exists, delete it
					if( segGrp != null ) {
						segGrp.parentNode.removeChild(segGrp);
					}

					// Create segment group
                    segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
                    segGrp.setAttribute('id', 'segGrp');
                    annoGrp.appendChild(segGrp);


					for( cell in data ) {
						ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
						
						ele.setAttribute('points', data[cell][0]);
						ele.setAttribute('id', 'N' + data[cell][1]);
						ele.setAttribute('stroke', data[cell][2]);
						ele.setAttribute('fill', 'none');
						
						segGrp.appendChild(ele);
					}
        		}
    	});
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
// =======================  Button Handlers ===================================
//



//
//	Load the boundaries for the current slide and display
//
//
function viewSegmentation() {

	var	segBtn = $('#btn_1');

	if( segDisplayOn ) {
		// Currently displaying segmentation, hide it
		segBtn.val("Show Segmentation");
		$('.overlaySvg').css('visibility', 'hidden');
		segDisplayOn = false;
	} else {
		// Segmentation not currently displayed, show it
		segBtn.val("Hide Segmentation");
		$('.overlaySvg').css('visibility', 'visible');
		segDisplayOn = true;
		
		updateSeg();
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
	dataportBottom: ko.observable(0)
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


