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

#include "flashz-async.hpp"
#include <Ticker.h>

#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

// ESP32 log tag
static const char *TAG __attribute__((unused)) = "FZ-ASYNC";

#define REBOOT_TIMEOUT  5000

static const char PGotaform[]  = R"===(
<form method="post" action="#" enctype='multipart/form-data'>
<input type="radio" id="imgtype1" name="img" value="fw" checked>
<label for="imgtype1">Firmware</label>
<input type="radio" id="imgtype2" name="img" value="fs">
<label for="imgtype2">FileSystem</label>
<input type='file' accept='.bin, .zz' name='file'>
<input type='submit' value='Upload'></form>
)===";
static const char PGimg[]  = "img";


void fz_async_register_ota(AsyncWebServer &srv, const char* url){

    srv.on(url, HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, PGmimehtml, PGotaform); });
    fz_async_register_upload(srv, url);
}

void fz_async_register_upload(AsyncWebServer &srv, const char* url){
    srv.on(url, HTTP_POST,
        [](AsyncWebServerRequest *request){
            if (FlashZ::getInstance().hasError()) {
                request->send(503, PGmimetxt, "Update FAILED");
            } else {
                Ticker *selfreboot = new Ticker;
                selfreboot->attach_ms(REBOOT_TIMEOUT, [](){ ESP.restart(); });
                request->send(200, PGmimetxt, "OTA complete, autoreboot in 5 sec...");
            }
        },
        fz_async_handler
    );
}

void fz_async_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){

    // first chunk of body data
    if (!index) {
        bool mode_z = data[0] == ZLIB_HEADER ? true : false;    // check if we have a compressed image

        int type;

        if (request->hasParam(PGimg, true)){
            // image type is specified in the form data
            type = request->getParam(PGimg, true)->value() == "fs" ? U_SPIFFS : U_FLASH;
        } else{
            // no image type specified, try to autodetect
            if (data[0] == ESP_IMAGE_HEADER_MAGIC || mode_z)        // can't detect what is insize zlib, so assume it's a fw image (won't owerwrite chip's FS)
                type = U_FLASH;
            else
                type = U_SPIFFS;

//          return request->send(400, PGmimetxt, F("Not an FW image or img type is unknown"));
        }

        // can rely on upload's size only if img is uncompressed
        size_t size = (data[0] == ESP_IMAGE_HEADER_MAGIC) ? request->contentLength() : UPDATE_SIZE_UNKNOWN;

        ESP_LOGD(TAG, "Updating %s, input size:%u, mode_z:%u, magic: %02X", (type == U_FLASH)? "Firmware" : "Filesystem", request->contentLength(), mode_z, data[0]);

        if (!(mode_z ? FlashZ::getInstance().beginz(size, type) : FlashZ::getInstance().begin(size, type))){
            return request->send(503, PGmimetxt, FlashZ::getInstance().errorString());
        }
    }

    // file content data
    if (len) {
        if(FlashZ::getInstance().writez(data, len, final) != len){
            ESP_LOGW(TAG, "OTA failed in progress: %s", FlashZ::getInstance().errorString());
            request->send(503, PGmimetxt, FlashZ::getInstance().errorString());
            return FlashZ::getInstance().abortz();
        }
    }

    // last chunk of input
    if (final) {
        if(FlashZ::getInstance().endz()){
            ESP_LOGI(TAG, "Update Success: %u bytes", index+len);
            //request->send(200, PGmimetxt, "OTA complete");
        } else {
            ESP_LOGW(TAG, "OTA failed to complete");
            //request->send(503, PGmimetxt, FlashZ::getInstance().errorString());
        }
    }
}