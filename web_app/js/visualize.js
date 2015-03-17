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
var negClass = "";
var posClass = "";
var uid = "";
var IIPServer = "";


var SlideSuffix = ".svs-tile.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";

// Objects to display, 'group' row of 2 * 'strata' objects
var	strata = 4, groups = 4;
var segDisplayOn = true;


//
//	Initialization
//
//
$(function() {


	// get session vars
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {
			
			uid = data['uid'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			IIPServer = data['IIPServer'];

			
			if( uid == null ) {			
				window.alert("No session active");
				window.history.back();
			} else {
			
				$('#posHeader1').text(posClass);
				$('#negHeader1').text(negClass);
				
				visualize();
			}
		}
	});

	
});





function visualize() {
	
	$.ajax({
		type: "POST",
		url: "php/getVisualization.php",
		dataType: "json",
		data: { strata: strata,
				groups: groups 
			  },
		success: function(data) {

			var thumbTag;
			
			// Data is returned ordered most certain to most uncertain for
			// the negative class followed by the positive class most uncertain 
			// to most certain
			// 			
			var row = 1, col = 1, scale;
			for(var i in data) {
				slide = data[i]['slide'];
				scale = data[i]['scale'];
				
				centX = (data[i]['centX'] - (40.0 * scale)) / data[i]['maxX'];
				centY = (data[i]['centY'] - (40.0 * scale)) / data[i]['maxY'];
				sizeX = (80.0 * scale) / data[i]['maxX'];
				sizeY = (80.0 * scale) / data[i]['maxY'];
				loc = centX+","+centY+","+sizeX+","+sizeY;
				
				
				if( data[i]['path'] === null ) {
					var SlidePath = "FIF=/bigdata2/PYRAMIDS/KLUSTER/20XTiles_raw/";	
					thumbNail = IIPServer+SlidePath+slide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
				} else {
					thumbNail = IIPServer+"FIF="+data[i]['path']+SlideLocPre+loc+SlideLocSuffix;						
				}
				
				console.log("thumbnail: " + thumbNail);
					
				thumbTag = "#row_"+(parseInt(row))+"_"+(parseInt(col));
				$(thumbTag).attr("src", thumbNail);
				
				if( row === 1 ) {
					thumbTag = "#thumb_"+parseInt(col);
					$(thumbTag).attr("src", thumbNail);
				}
									
				col = col + 1;
				if( col > 2 * strata ) {
					col = 1;
					row = row + 1;
				}
			}
			
			showBoundaries(data);
		}
	});
}







function showBoundaries(nuclei)
{

	$.ajax({
		type: "POST",
		url: "db/getBoundariesForThumbs.php",
		dataType: "json",
		data: { nuclei: nuclei },
		success: function(nuclei) {

			var img = document.getElementById('row_1_1');
			var overlay = document.getElementById('vis1_1');
			var	ele, x, y, row = 1, col = 1, scale;
	
			for( obj in nuclei ) {

				rowTag = "row_"+parseInt(row)+"_"+parseInt(col);
				visTag = "vis"+parseInt(row)+"_"+parseInt(col);
				
				img = document.getElementById(rowTag);
				overlay = document.getElementById(visTag);
				slideScale = nuclei[obj]['scale'];


				if( overlay != null ) {
					x = nuclei[obj]['centX'] - 40.0; //(40.0 * slideScale);
					y = nuclei[obj]['centY'] - 40.0; //(40.0 * slideScale);
					console.log("Centroid  "+nuclei[obj]['centX']+", "+nuclei[obj]['centY']);
					console.log("Translating by  -"+x+", -"+y);

					ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
					ele.setAttribute('points', nuclei[obj]['boundary']);
					ele.setAttribute('id', 'boundary');
					ele.setAttribute('stroke', 'yellow');
					ele.setAttribute('fill', 'none');
					ele.setAttribute('visibility', 'visible');
					overlay.appendChild(ele);	
				}
				
				console.log("Image width: "+img.clientWidth+" image height: "+img.clientHeight);
				console.log("Scale to: "+(img.clientWidth / (50.0 *slideScale)));
				
				scale = 1.0; //(img.clientWidth / (50.0 *slideScale));
				overlay.setAttribute("transform", 'translate(-'+x+',-'+y+') scale('+scale+')');
				
				col = col + 1;
				if( col > 8 ) {
					col = 1;
					row = row + 1;
				}
			}
		}
	});

}






// 
//	Show and hide segmentation by setting the css visibility 
//	attribute to hidden or visible.
//
//
function toggleSeg() {

	var	segBtn = $('#toggleBtn');

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
	}
}

