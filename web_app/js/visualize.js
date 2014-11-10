var negClass = "";
var posClass = "";
var uid = "";
var IIP_Server = "";
var SlidePath = "";
var curDataset;

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
			curDataset = data['dataset'];
			IIP_Server = data['iipServer'];
			SlidePath = data['slidePath'];
					
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

			console.log("Vis data length: " + data.length);
			var thumbtag;
			
			// Data is returned ordered most certain to most uncertain for
			// the negative class followed by the positive class most uncertain 
			// to most certain
			// 			
			var row = 1, col = 1;
			for(var i in data) {
				slide = data[i]['slide'];
				centX = (data[i]['centX'] - 40) / data[i]['maxX'];
				centY = (data[i]['centY'] - 40) / data[i]['maxY'];
				sizeX = 80.0 / data[i]['maxX'];
				sizeY = 80.0 / data[i]['maxY'];
				loc = centX+","+centY+","+sizeX+","+sizeY;
				
				thumbNail = IIP_Server+SlidePath+slide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
				
				if( curDataset === "Single slide test" ) {
					thumbNail = IIP_Server+SlidePath+slide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
				} else {
					thumbNail = IIP_Server+"FIF=/bigdata3/mnalisn_scratch/active_learning_slides/SOX2-pyramids/sox2test.tif"+SlideLocPre+loc+SlideLocSuffix;						
				}
					
				thumbTag = "#row_"+(parseInt(row))+"_"+(parseInt(col));
				$(thumbTag).attr("src", thumbNail);
				
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

				if( overlay != null ) {
					x = nuclei[obj]['centX'] - 40;
					y = nuclei[obj]['centY'] - 40;
		
					ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
					ele.setAttribute('points', nuclei[obj]['boundary']);
					ele.setAttribute('id', 'boundary');
					ele.setAttribute('stroke', 'yellow');
					ele.setAttribute('fill', 'none');
					ele.setAttribute('visibility', 'visible');
					overlay.appendChild(ele);	
				}
				scale = 1.0; //img.clientWidth / 80.0; 
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

