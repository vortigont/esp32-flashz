/*
    ESP32-FlashZ library

    This code implements a library for ESP32-xx family chips and provides an
    ability to upload zlib compressed firmware images during OTA updates.

    It derives from Arduino's UpdaterClass and uses in-ROM miniz decompressor to inflate
    libz compressed data during firmware flashing process

    Copyright (C) Emil Muratov, 2022
    GitHub: https://github.com/vortigont/esp32-flashz

    Lib code based on esptool's implementation https://github.com/espressif/esptool/
    so it inherits it's GPL-2.0 license

 *  This program or library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *  Public License version 2 for more details.
 *
 *  You should have received a copy of the GNU General Public License version 2
 *  along with this library; if not, get one at
 *  https://opensource.org/licenses/GPL-2.0
 */



/*
 An example on how to implement a simple update web page with AsyncWebServer
 and FlashZ lib than can handle both compressed and uncompressed firmware/filesystem OTA updates

 - set you WiFi creds below
 - build and flash the the code
 - run console monitor, it will show an ip address once WiFi is connected
 - open the browser and specified URL  (http://<esp-ip>/update)
 - upload zlib compressed firmware
 - or specify remote URL to download and update the fw image from
 - or use platformio's env's to do automated build/compress/upload fw tests

 NOTE: ESP32-FLashZ lib does not include AsyncWebServer support by default
 due to introduced dependency to external libs.
 To include it use build-time flag FZ_WITH_ASYNCSRV.
 See included platformio.ini file for build environment setup.

*/

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "flashz-http.hpp"


#define BAUD_RATE       115200  // serial port baud rate (for debug)

// Set your WiFi AP credentials here
const char* ssid = "MySSID";
const char* password = "MyPass";
const char* ota_url = "/update";        // OTA form URL

AsyncWebServer server(80);              // AsyncWebServer instance


/*
  FlashZ http helper class, it will handle interaction with AsyncWebServer
  file upload and FlashZ decompressor/writer
*/
FlashZhttp    fz;

// MAIN Setup
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.print("Starting flashz test");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.printf("\nConnected to: %s, IP-address: %s\n", ssid, WiFi.localIP().toString().c_str());
  Serial.printf("\nNavigate to: http://%s%s, and upload raw or zlib-compressed firmware/fs image\n", WiFi.localIP().toString().c_str(), ota_url);

  /*
    Here we register '/update' POST handler

    It handles HTTP POST requests for the form data,
    parses the form, retreives options like firmware type (FW/FS) or
    remote URL downloading and initiates firmware update rpocess.

    This handler is not mandatory, you can use your own fancy handler,
    but you need to implement same POST parsing logic.
  */
  fz.handle_ota_form(&server, ota_url);


  /*
    If you implement you own handlers for the page/form data parsing
    than you need to register file upload handler for the posted data.
    It will decompress uploaded file chunks and write it to SPI flash
    FlashZhttp::file_upload - it has a type of AsyncWebServer callback function
  */

  // try to mount LittleFS and serve all static files from root FS /
  if (LittleFS.begin()){
    server.serveStatic("/", LittleFS, "/")
          .setDefaultFile("index.html");
  }

  server.begin();
}

// MAIN loop
void loop() {
  // just do nothing here
  delay(10);
}