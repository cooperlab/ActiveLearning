var annoGrpTransformFunc;
var IIP_Server = "http://node15.cci.emory.edu/cgi-bin/iipsrv.fcgi?";
var SlidePath = "FIF=/bigdata2/PYRAMIDS/KLUSTER/20XTiles_raw/";
var SlideSuffix = ".svs-tile.dzi.tif&";
var dataviewUrl="http://cancer.digitalslidearchive.net/local_php/get_slide_list_from_db_groupid_not_needed.php"
var slideHost="http://node15.cci.emory.edu/";
var SlideLocPre = "RGN=";
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

$(function() {

	width = 0;
	$('#overflow .slider div').each(function() {
		width += $(this).outerWidth(true);
	});
	
	$('#overflow .slider').css('width', width + "px");

	// Create the slide zoomer, add event handlers, etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "slideZoom", prefixUrl: "images/"});
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
		
		viewer.drawer.clearOverlays();
        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
		osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
		curSlide = "";
		posSel = negSel = totalSel = 0;
	});


	viewer.addHandler('animation-finish', function(event) {

		if( displaySeg ) {
		
			if( statusObj.scaleFactor() > 0.5 ) {
				$('.overlaySvg').css('visibility', 'visible');
				updateSeg();
			} else {
				$('.overlaySvg').css('visibility', 'hidden');
			}
		}
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

			curSlide = data[0];		// Start with the first slide in the list

			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data ) {			
				slideSel.append(new Option(data[item], data[item]));
			}

			// Get the slide pyrimaid and display	
			updateSlideView();
		}
	});
}





function updateSlideView() {

	var slideUrl = dataviewUrl+'?slide_name_filter=' + curSlide;
	
	$.ajax({ 
		type: 	"GET",
		url: 	slideUrl,
		dataType:	"xml",
		success: function(xml) {
		
			pyramid = $(xml).find("slide_url").text();
			viewer.open(slideHost + pyramid);			
		}, 
		error: function() {
			alert("Unable to get slide information");
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





function nucleiSelect() {

	if( selectNuc ) {
		$.ajax({
	        type:   "POST",
            url:    "db/getsingle.php",
            dataType: "json",
            data:   { slide:    curSlide,
                      cellX:    Math.round(statusObj.mouseImgX()),
                      cellY:    Math.round(statusObj.mouseImgY())
            },
            success: function(data) {
					if( data !== null ) {
											
						console.log('Selected nuclei: '+data[1]);
						
						if( statusObj.posSel() < 4 ) {
							statusObj.posSel(statusObj.posSel() + 1); 
						} else if( statusObj.negSel() < 4 ) {
							statusObj.negSel(statusObj.negSel() + 1); 			
						} 
				
						var total = statusObj.posSel() + statusObj.negSel();
	
						sample = {};
						
						// Distance from nuclei is element 4
						sample['slide'] = curSlide;
						sample['id'] = data[1];
						sample['centX'] = data[2];
						sample['centY'] = data[3];
						sample['boundary'] = data[0];
						sample['maxX'] = data[5];
						sample['maxY'] = data[6];
						
						if( total <= 4 ) {
							sample['label'] = 1;
						} else {
							sample['label'] = -1;
						}
												
						selectedJSON.push(sample);
						
						var box = "#box_" + total, thumbTag = "#thumb_" + total,
							labelTag = "#label_" + total, loc;

						console.log("centX: " + sample['centX'] + ", maxX: " + sample['maxX']);
						
						$(box).show();
						centX = (sample['centX'] - 25) / sample['maxX'];
						centY = (sample['centY'] - 25) / sample['maxY'];
						sizeX = 50.0 / sample['maxX'];
						sizeY = 50.0 / sample['maxY'];
						loc = centX+","+centY+","+sizeX+","+sizeY;
				
						var thumbNail = IIP_Server+SlidePath+curSlide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
						$(thumbTag).attr("src", thumbNail);

						if( sample['label'] === 1 ) {
							$(labelTag).text(posClass);
							$(labelTag).css('background', '#00DD00');
						} else {
							$(labelTag).text(negClass);				
							$(labelTag).css('background', '#DD0000');
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

	clickCount++;
	if( clickCount === 1 ) {
		// If no click within 200ms, treat it as a single click
		singleClickTimer = setTimeout(function() {
					// Single click
					clickCount = 0;
				}, 200);
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
		// Submit to active learning server
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






function cancelSession() {
	console.log("Canceling");
	
	$.ajax({
		url: "php/cancelSession.php",
		data: "",
		success: function() {
			window.location = "index.html";
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

