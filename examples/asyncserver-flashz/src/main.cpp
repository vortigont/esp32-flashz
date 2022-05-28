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

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "flashz-async.hpp"


#define BAUD_RATE       115200  // serial port baud rate (for debug)

const char* ssid = "MySSID";
const char* password = "MyPassword";
const char* ota_url = "/update";

AsyncWebServer server(80);      // AsyncWebServer instance

// MAIN Setup
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.print("Starting flashz test");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\nConnected to: %s, IP-address: %s", ssid, WiFi.localIP().toString().c_str());
  Serial.printf("\nNavigate to: http://%s%s, and upload raw or zlib-compressed firmware/fs image", WiFi.localIP().toString().c_str(), ota_url);

  /*
   register FlashZ OTA handler. It provides simple html upload form for GET handler
   and file upload handler for firmware flashing
  */
  fz_async_register_ota(server, ota_url);

  /*
    If you want to handle GET requests for OTA url on your own, than check
    for two other functions in flashz-async.hpp
  */

  // try to mount LittleFS and serve all static files from root /
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