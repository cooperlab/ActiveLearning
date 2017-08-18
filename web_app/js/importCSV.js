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

var eventSel = $("#censoringdata");
var timeSel = $("#timesdata");
var groupSel = $("#groupdata");
var data = [];
var var_name = [];
var chosen_var = [];
var chosen_field;
var json_for_send = ''
var csv_Json;
var IIPServer="";
var curDataset = "";
var slideSet = null;
var slideReq = null;
var uid = null;
var application = "";

$(function() {

    application = $_GET("application");

    document.getElementById("home").setAttribute("href","index_home.html?application="+application);
    document.getElementById("nav_select").setAttribute("href","grid.html?application="+application);
    document.getElementById("nav_review").setAttribute("href","review.html?application="+application);
    document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
    document.getElementById("nav_heatmaps").setAttribute("href","heatmaps.html?application="+application);
    document.getElementById("nav_survival").setAttribute("href","survival.html?application="+application);

    // get slide host info
  	//
  	$.ajax({
  		url: "php/getSession.php",
  		data: "",
  		dataType: "json",
  		success: function(data) {

  			uid = data['uid'];
  			curDataset = data['dataset'];

  			if( uid === null ) {
  				window.alert("No session active");
  				window.history.back();
  			}
  			else{
  				getObjectCnt();
  			}
  		}
  	});

    if (window.File && window.FileReader && window.FileList && window.Blob) {
      $('#files').bind('change', handleFileSelect);
    }
});


function getSurvivalData() {

	// Get the information for the current dataset
	$.ajax({
		type: "POST",
		url: "php/getSurvivalData.php",
		data: { slideSet: slideSet,
			test: curDataset},
		dataType: "json",
		success: function(data) {
				var result = data['scores'];
		}
	});
}

function getObjectCnt() {

	// Display the progress dialog...
	$('#progDiag').modal('show');

	$.ajax({
		type: "POST",
		url: "php/getcounts.php",
		data: { dataset: curDataset,
				uid: uid
			  },
		dataType: "json",
		success: function(data) {

			slideSet = data;

			for( var i in slideSet['scores'] ) {
				var slide = String(slideSet['scores'][i]['slide']);
				var posNum = slideSet['scores'][i]['posNum'];
				var totalNum = slideSet['scores'][i]['totalNum'];
			}

			getSurvivalData();

			// Hide progress dialog
			$('#progDiag').modal('hide');

		},
		failure: function() {
			console.log("getCounts failed");
		}
	});
}


function handleFileSelect(evt) {
  var files = evt.target.files; // FileList object
  var file = files[0];
  var reader = new FileReader();
  reader.readAsText(file);
  reader.onload = function(event) {
      var csv = event.target.result;
      csv_Json = csvJSON(csv);
      console.log(csv_Json);
      $.ajax({
        type: 'POST',
        url: 'python/tableContent.php',
        data: {'uni_coeff': csv_Json},
        success: function(msg) {
           console.log(msg);
           var filename = "python/uni_output.csv"
           updateTable(filename);
        }
      });

      data = $.csv.toObjects(csv);
      console.log(data);

      var obj1 = data[0];
      var_name = Object.keys(obj1);
      var var_len = var_name.length;
      var ul = document.getElementById("list");
      for (var i = 0; i< var_len; i++) {
        var li = document.createElement("li");
        li.className = "list-group-item";
        if (var_name[i] == "time" || var_name[i] == "event") {
            continue;
        }
        li.appendChild(document.createTextNode(var_name[i]));
        ul.appendChild(li);
      }
      for (var i = 0; i < data.length;i++) {
          if (isNaN(data[i].event) ||isNaN(data[i].time)) {
              continue;
          }
          eventSel.append(data[i].event).append("\n");
          timeSel.append(data[i].time).append("\n");
          groupSel.append("Whole Dataset").append("\n");
      }
    }
// getInputDataAndDrawKM();
}

function updateTable(filename){
  $('#page-wrap').empty();
  d3.csv(filename, function(error, data) {
    if (error) throw error;
    console.log(data);
    var sortAscending = true;
    var table = d3.select('#page-wrap').append('table');
    var titles = d3.keys(data[0]);
    var cl = ['Variable', 'Hazard Ratio', 'lower 0.95', 'upper 0.95', 'p','Hazard Ratio', 'lower 0.95', 'upper 0.95','p'];
    var headers = table.append('thead').append('tr')
                     .selectAll('th')
                     .data(cl).enter()
                     .append('th')
                     .text(function (d) {
                        return d;
                      })
                     .on('click', function (d) {
                       headers.attr('class', 'header');
                       if (sortAscending) {
                         rows.sort(function(a, b) { return b[d] < a[d]; });
                         sortAscending = false;
                         this.className = 'aes';
                       } else {
                       rows.sort(function(a, b) { return b[d] > a[d]; });
                       sortAscending = true;
                       this.className = 'des';
                       }

                     });

    var rows = table.append('tbody').selectAll('tr')
                 .data(data).enter()
                 .append('tr');
    rows.selectAll('td')
      .data(function (d) {
        return titles.map(function (k) {
          return { 'value': d[k], 'name': k};
        });
      }).enter()
      .append('td')
      .attr('data-th', function (d) {
        return d.name;
      })
      .text(function (d) {
        return d.value;
      });
  });
}


function csvJSON(csv){
  var lines=csv.split("\n");
  var result = [];
  var headers=lines[0].split(",");
  for(var i=1;i<lines.length;i++){
	  var obj = {};
	  var currentline=lines[i].split(",");
	  for(var j=0;j<headers.length;j++){
		  obj[headers[j]] = currentline[j];
	  }
	  result.push(obj);
  }
  //return result; //JavaScript object
  return JSON.stringify(result); //JSON
}
function getFields(input, field) {
  var output = [];
  for (var i=0; i < input.length ; ++i){
    output.push(input[i][field]);
  }
  return output;
}

$(document).on('click', ' .list-group-item', function(e) {
   // add active class at list-item by click
   $(this).parent().find('.list-group-item').removeClass('active');
   $(this).addClass('active');
   // make add(>) button disabled untill selecting one
   var source = $(this).parents('.pick-list').find('.source').children('.active');
     if(source.length > 0) {
         $(this).attr('disabled',false);
         $('.add').attr('disabled',false);
     } else {
         $(this).attr('disabled',true);
         $('.add').attr('disabled',true);
     };
         // make remove(<) button disabled untill selecting one
   var des = $(this).parents('.pick-list').find('.destination').children('.active');
     if(des.length > 0) {
         $(this).attr('disabled',false);
         $('.remove').attr('disabled',false);
     } else {
         $(this).attr('disabled',true);
         $('.remove').attr('disabled',true);
     };
  });
$('.add').click(function(){
    var item = $(this).parents('.pick-list').find('.source').children('.active');
    if(item.length > 0) {
        $('.source .list-group-item.active').appendTo('.list-group.destination');
        $('.add').attr('disabled',true);
        $('.destination .list-group-item.active').removeClass('active');
    }
});
$('.remove').click(function(){
    var item = $(this).parents('.pick-list').find('.destination').children('.active');
    // Move item from destination to source when one item is active/selected
    if(item.length > 0) {
        $('.destination .list-group-item.active').appendTo('.source');
        $('.remove').attr('disabled',true);
        $('.source .list-group-item.active').removeClass('active');
    }
});
$('#submit').click(function(){
    chosen_field = [];
    $('.destination .list-group-item').each(function(){
       chosen_field.push($(this).text());
     });
//     console.log(chosen_field);
     var json_join = new Array();
     var time_obj = new Object();
     var event_obj = new Object();
     var time_dt = getFields(data,"time");
     var event_dt = getFields(data,"event");
     time_obj.key = "time";
     time_obj.value = time_dt;
     event_obj.key = "event";
     event_obj.value = event_dt;
     json_join.push(time_obj);
     json_join.push(event_obj);

     for (var i = 0; i < chosen_field.length; i++) {
         var result = (getFields(data,chosen_field[i]));//array
         var json_single = new Object();
         json_single.key = chosen_field[i];
         json_single.value = result;
         json_join.push(json_single);
    }
    json_for_send = JSON.stringify(json_join);
    $.ajax({
      type: 'POST',
      url: 'python/tableContent.php',
      data: {'multi_coeff': json_for_send},
      success: function(msg) {
        console.log(msg);
        var filename = "python/multi_output.csv"
        updateTable(filename);

      }
    });
    $('#censoringdata').empty();
    $('#timesdata').empty();
    $('#groupdata').empty();

    for (var j = 0; j < chosen_field.length; j++){
        var ch = chosen_field[j];
        for (var i = 0; i < data.length;i++) {
           if (isNaN(data[i].event) ||isNaN(data[i].time)||isNaN(data[i][ch])) {
              continue;
            }
            if(data[i][ch] == 1 ){
              eventSel.append(data[i].event).append("\n");
              timeSel.append(data[i].time).append("\n");
              groupSel.append(ch).append("\n");
            }
        }
    }
    getInputDataAndDrawKM();
});

//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}
