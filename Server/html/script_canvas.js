var canvas = document.getElementById('viewport'),
context = canvas.getContext('2d');

var map_image = 'https://i.imgur.com/pnqkqlY.png';
var bin_image = 'https://i.imgur.com/bkwpRrm.png';
var heavy_bin = 'https://i.imgur.com/5DZ9pUe.png';

//Color for different status
var full = "#ff4d4d";
var half = "#ffff4d";
var empty = "#4dff4d";

var dim_x_bin = 24;
var dim_y_bin = 24;

//Request the data of the bin to /data.php
var xmlHttp = new XMLHttpRequest();
xmlHttp.open( "GET", "/data.php", false ); // false for synchronous request
xmlHttp.send( null );
var data = xmlHttp.responseText;
var info = jQuery.parseJSON(data);
//Log data obtained
console.log(info);

/*Data retrieved from the DB
 * 0 -> Color of bin
 * 1 -> Does the bin is heavier than normally
 * 2 -> x coordinate
 * 3 -> y coordinate
 * 4 -> callback link
*/

create_map();
create_buttons();


function create_map() {
	base_image = new Image();
	base_image.src = map_image;
	base_image.onload = function(){
		context.imageSmoothingEnabled = false;
		context.drawImage(base_image, 0, 0, base_image.width, base_image.height);
		
		var arrayLength = info.length;
		for (var i = 0; i < arrayLength; i++) {
			color = full;
			if (info[i][0] == "half") {
				color = half;
			} else if (info[i][0] == "empty") {
				color = empty;
			} else if (info[i][0] == "full") {
				color = full;
			}
			draw_bin(color, info[i][1], info[i][2], info[i][3]);
		}
	}
}

function create_buttons() {
	var arrayLength = info.length;
	elem = document.getElementById('all_but');
	for (var i = 0; i < arrayLength; i++) {
		//~ console.log("Pulsante: "+i);
		draw_bin(info[i][0], info[i][1], info[i][2], info[i][3]);
		but = "<input type='button' class='bin_button' onclick=\"location.href='http://10.79.1.176:1880/bin/"+ info[i][4] +"';\" style=\"left:"+ (info[i][2]-dim_x_bin/4+2) +"px; top:"+ (info[i][3]-2) +"px;\" />";
		elem.innerHTML = elem.innerHTML + but;
	}
}

function draw_bin(status, heavy, x, y) {
	//Bin size
	
	
	//Start painting
	context.beginPath();
	//Create background
	context.rect(x, y, dim_x_bin/3*2, dim_y_bin-4);
	//Fill with the selected color
	context.fillStyle = status;
	context.fill();
		
	//Change image for heavy bins
	if (heavy !== true) {
		//Not a weight over the mean
		imageObj = new Image();
		imageObj.src = bin_image;
		imageObj.onload = function() {
			context.drawImage(imageObj, x-dim_x_bin/6*2+2, y-dim_x_bin/6*2+2, dim_x_bin+4, dim_y_bin+4);
		};
	} else {
		//Weight over the mean - Probably organic waste
		imageHeavyObj = new Image();
		imageHeavyObj.src = heavy_bin;
		imageHeavyObj.onload = function() {
			context.drawImage(imageHeavyObj, x-dim_x_bin/6*2+4, y-dim_x_bin/6*2+4, dim_x_bin, dim_y_bin+4);
		};
	}
	
}
