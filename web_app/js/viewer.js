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

var debugMode = 1;

//
// TODO - Remove modes from the viewer
//

var viewMode = 'view';		// Default to regular viewer mode
							// Viewer modes are:
							//	'view'  - Normal view (display boundaries, select slides)
							//  'prime' - Prime mode  (select cells for classifier)
							//  'cell'  - cell info   (Single slide, display boundaries)s
						

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
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "image_zoomer", prefixUrl: "images/"});
	imgHelper = viewer.activateImagingHelper({onImageViewChanged: onImageViewChanged});
	viewerHook = viewer.addViewerInputHook({ hooks: [
						{tracker: 'viewer', handler: 'pressHandler', hookHandler: onMouseDown},
						{tracker: 'viewer', handler: 'scrollHandler', hookHandler: onMouseScroll},
						{tracker: 'viewer', handler: 'clickHandler', hookHandler: onMouseClick},
						{tracker: 'viewer', handler: 'releaseHandler', hookHandler: onMouseUp}
				 ]});


	var newMode = $_GET('mode');
	console.log("New mode: " + newMode);
	if( newMode != null ) {
		viewMode = newMode;
	} 
	
	if( viewMode == 'cell' ) {
		// TODO!!!! ad a check for GET variables 'slide', 'cx' and 'cy'. Theses specify a particlular
		// nuclei to zoom to. 
	}

	$(viewer.canvas).css('background', 'black');
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

		updateImageInfo();

        // Create a div that encompasses the entire IMAGE area for the overlay. The overlay
		// has a 1:1 correlation with the image.
		//
        olDiv = document.createElement('div');
        $(olDiv).attr("id", "ovrSVG");
        
        // Load a blank placeholder SVG
		$(olDiv).load('images/blank.svg');

        var olRect = new OpenSeadragon.Rect(imgHelper.physicalToLogicalX(imgHelper.dataToPhysicalX(0)),
                                        imgHelper.physicalToLogicalY(imgHelper.dataToPhysicalY(0)),
                                        imgHelper.physicalToLogicalX(imgHelper.dataToPhysicalX(statusObj.imgWidth())),
                                        imgHelper.physicalToLogicalY(imgHelper.dataToPhysicalY(statusObj.imgHeight())));

        viewer.drawer.addOverlay({
                element:    olDiv,
                location:   olRect,
                placement:  OpenSeadragon.OverlayPlacement.TOP_LEFT
        });
	});



	viewer.addHandler('close', function(event) {
		statusObj.haveImage(false);
		
		viewer.drawer.clearOverlays();
        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
        osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
	});
	
	if( viewMode == 'view' || viewMode == 'prime' ) { 

		// Slide list will also be updated by this call
		updateDatasetList();

		// Set the update handler for the slide selector
		slideSel.change(updateSlide);
		// Set the update handler ffor the dataset selector
		datasetSel.change(updateDataset);
	}
});






//
//	Get the url for the slide pyramid and set the viewer to display it
//
//
function updatePyramid() {

	var slideUrl = dataviewUrl+'?slide_name_filter=' + curSlide;
	slide = "";
	
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

	console.log("Current dataset: " + curDataset);
	
	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: curDataset },
		dataType: "json",
		success: function(data) {

			curSlide = data[0];		// Start with the first slide in the list
			slideCnt = Object.keys(data).length;;
			slideCntTxt.text(slideCnt);

			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data ) {			
				slideSel.append(new Option(data[item], data[item]));
			}

			// Get the slide pyrimaid and display	
			updatePyramid();
		}
	});
}






//
//	A new slide has been selected from the drop-down menu, update the 
// 	slide zoomer.
//
//
function updateSlide() {

}





//
//
//
//
function updateDataset() {


}






//
//	Update image information for the current slide
//
//
function updateImageInfo() {

	statusObj.imgWidth(imgHelper.imgWidth);
	statusObj.imgHeight(imgHelper.imgHeight);
	statusObj.imgAspectRatio(imgHelper.imgAspectRatio);
	statusObj.scaleFactor(imgHelper.getZoomFactor());
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
	
	if( viewMode == 'cell' ) {
	
	} else if( viewMode == 'prime' ) {
		$('#btn_2').show();	
		$('#SelDataset').hide();
	} else {    // View mode
		$('#btn_2').hide();
		$('#SelDataset').show();
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
// inputHook handlers ---------------------------------------------
//
function onMouseDown(event) {
}



function onMouseUp(event) {
}


function onMouseClick(event) {

	if( selectMode && statusObj.scaleFactor() > 1.0 ) { 
		console.log("Click - x: " + Math.round(statusObj.mouseImgX()) + 
					" y: " + Math.round(statusObj.mouseImgY()) );

		selectCell(Math.round(statusObj.mouseImgX()), Math.round(statusObj.mouseImgY()));	
	}
}


function onMouseScroll(event) {

	//  ???? NOT WORKING ????
	if( selectMode && statusObj.scaleFactor() > 1.0 ) {
		event.stopBubbling = true;
	}

}

//
// ===============================================================
//







//
//	Load the boundaries for the current slide and display
//
//
function viewSegmentation() {

	var btn1 = $("#btn_1"); 
	var	svg = document.getElementsByTagName('svg')[0];

	if( segDisplayOn ) {
		svg.setAttribute('visibility', 'hidden');
		segDisplayOn = false;
		btn1.val("Show Segmentation");
	} else {

		svg.setAttribute('visibility', 'visible');
		segDisplayOn = true;
		btn1.val("Hide Segmentation");
	}
}





//
//	Eneter or exit nuclei selection mode. Allows users to click on a nuclei to select it
//  for priming the learner.
//
//
function setSelectMode() {

	var btn2 = $("#btn_2"), btn1 = $("#btn_1");
	var	svg = document.getElementsByTagName('svg')[0];
	
	if( selectMode ) {

		selectMode = false;	
		btn2.val("Select Nuclei");
		btn2.css('color', 'black');

		btn1.prop('disabled', false);
	} else {

		selectMode = true;
		svg.setAttribute('visibility', 'visible');
        btn2.val("Done");
		btn2.css('color', 'red');

		btn1.prop('disabled', true);
	}
}	



//
//
//
/*
function selectNuclei() {
	$.ajax({
			type: 	"POST",
			url:	"db/getsingle.php",
			dataType: "json",
			data:	{ slide: 	curSlide,
					  cellX:	statusObj.mouseImgX(),
					  cellY:	statusObj.mouseImgy()
			},
           success: function(data) {

                    var ele;
                    var annoGrp = document.getElementsByTagName('g')[0];

                    for( cell in data ) {
                        ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

                        ele.setAttribute('points', data[cell][0]);
                        ele.setAttribute('id', 'N' + data[cell][1]);
                        ele.setAttribute('stroke', 'blue');
                        ele.setAttribute('fill', 'none');

                        annoGrp.appendChild(ele);
                    }
               	}
	});

}

*/

//
//	Retrieves the bounding polygon from the database for the cell closest
//	to cellX, cellY. Creates a new SVG group to contain selected cells if
//  it doesn't exist already. Moves the selected cell from the annotation 
//	group to the select group.
//
function selectCell(cellX, cellY) {

    $.ajax({
	        type:   "POST",
            url:    "db/getsingle.php",
            dataType: "json",
            data:   { slide:    curSlide,
                      cellX:    cellX,
                      cellY:    cellY
            },
            success: function(data) {
					if( data !== null ) {
						var ele;
						var selectGrp = document.getElementById('selGrp');


						// Create selection froup if not already created
						if( selectGrp == null ) {
							var annoGrp = document.getElementById('anno');
							
							selectGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
							selectGrp.setAttribute('id', 'selGrp');
							annoGrp.appendChild(selectGrp);
						}
						
						// Change to find element by id using the cell id and move to select group 
						//	ele.parentNode.removeChild(ele);
						//  ele.setAttribute('stroke', 'yellow');
						//	selectGrp.appendChild(ele);
						//

						ele = document.getElementById('N' + data[1]);
						ele.parentNode.removeChild(ele);
						ele.setAttribute('stroke', 'yellow');
						selectGrp.appendChild(ele);

//						ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

//						ele.setAttribute('id', 'N' + data[1]);
//						ele.setAttribute('points', data[0]);
//						ele.setAttribute('stroke', '#f1d92c');
//						ele.setAttribute('fill', 'none');
//						selectGrp.appendChild(ele);
					}
				}	
    });
}




function updateSeg() {


	if( statusObj.scaleFactor() > 0.5 ) {
	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { slide: 	curSlide,
					left:	statusObj.dataportLeft(),
					right:	statusObj.dataportRight(),
					top:	statusObj.dataportTop(),
					bottom:	statusObj.dataportBottom()
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
						ele.setAttribute('stroke', 'blue');
						ele.setAttribute('fill', 'none');
						
						segGrp.appendChild(ele);
					}
        		}
    	});
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
	
	// TODO - Add an animation-finish event handler to the viewer and do the segmentation update
	//	there. This will make scrolling and zooming the slide much smoother. See prime.js for an
	//	example.
	//
	
	
	// Update the segmentation and select displays only if they are on.	
	if( segDisplayOn ) {
		var annoGrp = document.getElementById('anno');
		
		if( statusObj.scaleFactor() > 0.5 ) {
			if( overlayHidden ) {
				overlayHidden = false;
				annoGrp.setAttribute('visibility', 'visible');
			}
			updateOverlayInfo();
		} else {
			if( overlayHidden == false ) {
				overlayHidden = true;
				annoGrp.setAttribute('visibility', 'hidden');
			}
		}
	}
}






function updateOverlayInfo() {

	// Only update the scale of the svg if it has changed. This speeds up 
	// scrolling through the image.
	//
	if( lastScaleFactor != statusObj.scaleFactor() ) {
		lastScaleFactor = statusObj.scaleFactor();
		var annoGrp = document.getElementById('anno');
		var scale = "scale(" + statusObj.scaleFactor() + ")";
	
		annoGrp.setAttribute("transform", scale);
	}

	updateSeg();
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




// Apply binfding for knockout.js - Let it keep track of the image info
// and mouse positions
//
ko.applyBindings(statusObj);


