#include <Arduino.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <TinyGPS++.h>

#include "SD_funcs.h"

// Pin definitions of the HC-12 module
#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;

// Pin definitions for SPI - used to connect to SD card
#define SPI_CLK 18
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_CS 5

SPIClass spi(VSPI);
#define SD_SPEED 80000000

// Function signatures
String printLocation();
String printAltitude();
String printVelocity();
void displayGPSInfo();

// Network information for the local access point
const char* ssid = "ESP32-AP";
const char* password = "pass123456";

AsyncWebServer server(80);

// Variables to store the data received from the GPS module
float latitude;
float longitude;
float altitude;
float velocity;

void setup() {
  Serial.begin(115200);

  // Start communication with the HC-12 module
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Start the local Access Point
  WiFi.softAP(ssid, password);

  // Get the IP address of the Access Point
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Enable the server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/index.html", "text/html");
  });

  // Response to HTTP GET requests
  server.on("/location", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", printLocation().c_str());
  });

  server.on("/altitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", printAltitude().c_str());
  });

  server.on("/velocity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", printVelocity().c_str());
  });

  // Allows for CSS and JavaScript and other assets to be loaded from the SD card
  server.serveStatic("/", SD, "/");
  
  server.begin();

  // Initialize the SPI pins for communication with the SD card
  spi.begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_CS);
  // SPI.setDataMode(SPI_MODE0);

  // Initialize the SD card
  SD.begin(SPI_CS, spi, SD_SPEED);
  SD_printType(SD);
  SD_printSize(SD);
  SD_printRootDir(SD);
  
}

void loop() {

  // If GPS module is sending data, decipher it with the TinyGPS+ library then save data into the variables
  while (Serial2.available()) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        altitude = gps.altitude.meters();
        velocity = gps.speed.mph();
      } else {
        latitude = 0;
        longitude = 0;
        altitude = 0;
        velocity = 0;
      }
      displayGPSInfo();
    }
  }

  // The following is used to randomly generate expected data points (used for testing purposes)
  float rng_lat;
  float rng_lon;

  const float lat_upperBound = 33.370605;
  const float lat_lowerBound = 33.367662;
  const float lon_upperBound = 86.853339;
  const float lon_lowerBound = 86.851778;
  const float coord_mult = 1000000.0;

  rng_lat = (random(lat_lowerBound * coord_mult, lat_upperBound * coord_mult) / coord_mult);
  rng_lon = (-1.0) * (random(lon_lowerBound * coord_mult, lon_upperBound * coord_mult) / coord_mult);

  float rng_alt;

  const float alt_lowerBound = 200.0;
  const float alt_upperBound = 300.0;
  const float alt_mult = 100.0;

  rng_alt = (random(alt_lowerBound * alt_mult, alt_upperBound * alt_mult) / alt_mult);

  float rng_velo;

  const float velo_lowerBound = 0.0;
  const float velo_upperBound = 50.0;
  const float velo_mult = 100.0;

  rng_velo = (random(velo_lowerBound * velo_mult, velo_upperBound * velo_mult) / velo_mult);

  // latitude = rng_lat;
  // longitude = rng_lon;
  // altitude = rng_alt;
  // velocity = rng_velo;

  String out = printLocation() + "," + printAltitude() + "," + printVelocity() + '\n';
  Serial.println(out);

  // Save the most recent data to the SD card
  SD_appendFile(SD, "/data4.txt", out.c_str());

  // Log data every 5 seconds
  delay(5000);
}

String printLocation() {
  String str = String(latitude, 6) + "," + String(longitude, 6);
  // Serial.print("printLocation(): ");
  // Serial.println(str);
  
  return str;  
}

String printAltitude() {
  // Serial.print("printAltitude(): ");
  // Serial.println(String(altitude));
  
  return String(altitude);
}

String printVelocity() {
  // Serial.print("printVelocity(): ");
  // Serial.println(String(velocity));

  return String(velocity);
}

void displayGPSInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}