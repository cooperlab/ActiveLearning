var negClass = "";
var posClass = "";
var uid = "";
var IIP_Server = "";
var SlidePath = "";
var curDataset;

var SlideSuffix = ".svs-tile.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";





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
		data: { strata: 4,
				groups: 4 
			  },
		success: function(data) {

			console.log("Vis data length: " + data.length);
			var thumbtag;
			
			// Data is returned most certainto most uncertain for
			// each class, we need to rearange the negative class
			// for the display
			
			var row = 1, col = 1;
			for(var i in data) {
				slide = data[i]['slide'];
				centX = (data[i]['centX'] - 50) / data[i]['maxX'];
				centY = (data[i]['centY'] - 50) / data[i]['maxY'];
				sizeX = 100.0 / data[i]['maxX'];
				sizeY = 100.0 / data[i]['maxY'];
				loc = centX+","+centY+","+sizeX+","+sizeY;
				
				thumbNail = IIP_Server+SlidePath+slide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
				
				if( curDataset === "Single slide test" ) {
					thumbNail = IIP_Server+SlidePath+slide+SlideSuffix+SlideLocPre+loc+SlideLocSuffix;
				} else {
					thumbNail = IIP_Server+"FIF=/bigdata3/mnalisn_scratch/active_learning_slides/SOX2-pyramids/sox2test.tif"+SlideLocPre+loc+SlideLocSuffix;						
				}
	
				console.log("Grid thumbnail: "+thumbNail);
				
				thumbTag = "#row_"+(parseInt(row))+"_"+(parseInt(col));
				$(thumbTag).attr("src", thumbNail);
				
				col = col + 1;
				if( col > 8 ) {
					col = 1;
					row = row + 1;
				}
			}
		}
	});
}

