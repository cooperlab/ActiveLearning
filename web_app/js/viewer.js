var annoGrpTransformFunc;
var dataviewUrl="http://cancer.digitalslidearchive.net/local_php/get_slide_list_from_db_groupid_not_needed.php"
var slideHost="http://node15.cci.emory.edu/";
var slideCnt = 0;
var curSlide = "";
var curDataset = "";
var viewer = null;
var imgHelper = null, osdCanvas = null, viewerHook = null;
var overlayHidden = false, selectMode = false, segDisplayOn = false;;
var olDiv = null;
var lastScaleFactor = 0;
var pyramids;

var debugMode = 1;

	var clickCount = 0;					

//
//	Initialization
//	
//		Get a list of available slides from the database
//		Populate the selection dropdown
//		load the first slide
//		Register event handlers
//
$(function() {
	var slideSel = $("#slide_sel");
	var	datasetSel = $("#dataset_sel");
	
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
	updateInterface();
	
	
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

	// Slide list will also be updated by this call
	updateDatasetList();
	
	// Classifier list needs the current dataset so update 
	// dataset list first
	updateClassifierList();
	
	// Set the update handler for the slide selector
	slideSel.change(updateSlide);
	// Set the update handler ffor the dataset selector
	datasetSel.change(updateDataset);

});






//
//	Get the url for the slide pyramid and set the viewer to display it
//
//
function updatePyramid() {

	var slideUrl = dataviewUrl+'?slide_name_filter=' + curSlide;
	slide = "";

	if( pyramids[$('#slide_sel').prop('selectedIndex')] === null ) {	
	
		$.ajax({ 
			type: 	"GET",
			url: 	slideUrl,
			dataType:	"xml",
			success: function(xml) {
		
				pyramid = $(xml).find("slide_url").text();
				console.log("Loading: " + pyramid);
				viewer.open(slideHost + pyramid);
			}, 
			error: function() {
				alert("Unable to get slide information");	
			}
		});
	} else {
	
		pyramid = "cgi-bin/iipsrv.fcgi?DeepZoom="+pyramids[$('#slide_sel').prop('selectedIndex')];
		console.log("Loading: " + pyramid);
		viewer.open(slideHost + pyramid);
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
			
			curDataset = data[0];		// Use first dataset initially
				
			for( var item in data ) {
				datasetSel.append(new Option(data[item], data[item]));
			}
			
			// Need to update the slide list since we set the default slide
			updateSlideList();
		}
	});
}





//
//	Updates the list of available slides for the current dataset
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

	// First selection should be none
	classSel.append(new Option('----------------', 'none'));

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
		
		console.log("Current Slide: "+curSlide);
	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { slide: 	curSlide,
					left:	statusObj.dataportLeft(),
					right:	statusObj.dataportRight(),
					top:	statusObj.dataportTop(),
					bottom:	statusObj.dataportBottom(),
					dataset: curDataset,
					trainset: "sox2TileDemo.h5"
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
						ele.setAttribute('stroke', 'aqua');
						ele.setAttribute('fill', 'none');
						
						segGrp.appendChild(ele);
					}
        		}
    	});
	} 
}





//
// 	Display the appropriate interface componenets
// 	based on the view mode. The components default to 
// 	hidden.
//
//
function updateInterface() {

	var comp = document.getElementById('DebugData');
	if( debugMode == 1 ) {
		$('#DebugData').children().show();
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


