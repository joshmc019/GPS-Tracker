var canvas = document.getElementById('Canvas');
var context = canvas.getContext("2d");

// Map sprite
var mapSprite = new Image();
mapSprite.src = "map.png";

var Marker = function () {
    this.Sprite = new Image();
    this.Sprite.src = "marker0.png"
    this.Width = 16;
    this.Height = 16;
    this.XPos = 0;
    this.YPos = 0;
}

var Markers = new Array();

var main = function () {
    draw();
};

var draw = function () {
    // Clear Canvas
    context.fillStyle = "#000";
    context.fillRect(0, 0, canvas.width, canvas.height);

    // Draw map
    context.drawImage(mapSprite, 0, 0, 551, 659);
    
    // Draw markers
    for (var i = 0; i < Markers.length; i++) {
        var tempMarker = Markers[i];

        if (i == Markers.length - 1) {
          tempMarker.Sprite.src = "marker1.png";
        } else {
          tempMarker.Sprite.src = "marker0.png";
        }

        // Draw marker
        context.drawImage(tempMarker.Sprite, tempMarker.XPos, tempMarker.YPos, tempMarker.Width, tempMarker.Height);
    }
};

setInterval(main, (1000 / 2)); // Refresh 2 times a second

const refreshInterval = 3000; // Refresh data points every 3 seconds

var Location = function () {
    this.latitude = 0;
    this.longitude = 0;
}

// Constants for the GPS Coordinates data
var Locations = new Array();
const Locations_MAX_LEN = 16;

const lat_min = 33.3675;
const lat_max = 33.3725;
const lat_range = lat_max - lat_min;
const lon_min = -86.85;
const lon_max = -86.855;
const lon_range = (-1) * (lon_max - lon_min);

// HTTP GET request for the current location data
setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        console.log(this.responseText);

        var s = "Coords: " + this.responseText;
        document.getElementById("coords").innerHTML = s;

        const coords = this.responseText.split(',');

        console.log(coords[0]);
        console.log(coords[1]);

        var loc = new Location();

        var llat = parseFloat(coords[0]);
        var llon = parseFloat(coords[1]);

        loc.latitude = llat;
        loc.longitude = llon;

        var mark = new Marker();
        
        var markX = canvas.width + ((llon - lon_min) / lon_range) * canvas.width;
        var markY = canvas.height - ((llat - lat_min) / lat_range) * canvas.height;

        mark.XPos = markX;
        mark.YPos = markY;

        if (Locations.length >= Locations_MAX_LEN) {
          Locations.shift();
          Markers.shift();
        }

        Locations.push(loc);
        Markers.push(mark);

        let i = 0;
        while (i < Locations.length) {
            console.log(Locations[i]);
            console.log(Markers[i]);
            i++;
        }
      }
    };
    xhttp.open("GET", "/location", true);
    xhttp.send();
  }, refreshInterval);

  // HTTP GET request for the current altitude data
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        console.log(this.responseText);

        var s = "Alt: " + this.responseText + " (m)";
        document.getElementById("text-alt").innerHTML = s;

        var a = parseFloat(this.responseText);
        updateAltGauge(a);
      }
    };
    xhttp.open("GET", "/altitude", true);
    xhttp.send();
  }, refreshInterval);

  // HTTP GET request for the current velocity data
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        console.log(this.responseText);

        var s = "Speed: " + this.responseText + " (mph)";
        document.getElementById("text-velo").innerHTML = s;

        var v = parseFloat(this.responseText);
        updateVeloGauge(v);
      }
    };
    xhttp.open("GET", "/velocity", true);
    xhttp.send();
  }, refreshInterval);

// Constants for the altitude data
const alt_lowerBound = 0.0;
const alt_upperBound = 300.0;
const alt_range = alt_upperBound - alt_lowerBound;
const alt_mult = 100.0;

// Update the position of the needle on the altitude gauge
function updateAltGauge(alt) {
  const needle = document.getElementById('needle-alt');
  const angle = (alt / alt_range) * 180 - 90;
  needle.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;
}

// Constants for the velocity data
const velo_lowerBound = 0.0;
const velo_upperBound = 50.0;
const velo_range = velo_upperBound - velo_lowerBound;
const velo_mult = 100.0;

// Update the position of the needle on the velocity gauge
function updateVeloGauge(velo) {
  const needle = document.getElementById('needle-velo');
  const angle = (velo / velo_range) * 180 - 90;
  needle.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;
}