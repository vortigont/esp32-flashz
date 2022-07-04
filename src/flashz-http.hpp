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
a set of helper functions to handle compressed OTA updates via HTTP

build_time flags could be used to include only required functions,
this may introduce additional library dependency, i.e. AsyncWebServer

FZ_HTTP_ASYNC    - build with AsyncWebServer support handlers
*/

#pragma once

#ifdef FZ_HTTP_ASYNC
#include <ESPAsyncWebServer.h>
#endif  // #ifdef FZ_HTTP_ASYNC

#define REBOOT_TIMEOUT  5000

static const char PGmimehtml[] = "text/html; charset=utf-8";
static const char PGmimetxt[]  = "text/plain";

enum class fz_http_err_t:int {
    write_err = -6,
    bad_start = -5,
    bad_stream = -4,
    bad_size = -3,
    httpcode_err = -2,
    bad_param = -1,
    ok = 0
};

#ifdef FZ_HTTP_ASYNC
/**
 * @brief register OTA URL within AsyncServer
 * - provides a simple file upload form for FW/FW file images, could be raw *.bin or *.zz zlib compressed files
 * - register file upload call-back that hanles infate and OTA flash
 * 
 * @param srv - AsyncWebServer object
 * @param url - i.e. "/upload"
 */
void fz_async_register_ota(AsyncWebServer &srv, const char* url);

/**
 * @brief register file upload call-back that hanled infate and OTA flash
 * handles HTTP_POST only, webform handler should be registered elsewhere
 * 
 * @param srv - AsyncWebServer object
 * @param url - i.e. "/upload"
 */
void fz_async_register_upload(AsyncWebServer &srv, const char* url);

/**
 * @brief HTTP_POST OTA upload handler
 * file upload call-back that hanles zlib infate and OTA flash
 * 
 * @param request 
 * @param filename 
 * @param index 
 * @param data 
 * @param len 
 */
void fz_async_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
#endif  // #ifdef FZ_HTTP_ASYNC

/**
 * @brief fetch (possibly compressed) image file via http and flash
 * compressed image format is autodetected
 * 
 * @param url - source URL for firmware file (https is not supported)
 * @param imgtype - image file type U_FLASH (0 - default) or U_SPIFFS
 * @param autoreboot - reboot MCU after specified ms if flash was successfull, disable autoreboot if set to '0'
 * @return fz_http_err_t - returns error code
 */
fz_http_err_t fz_http_client(const char* url, int imgtype = 0, unsigned autoreboot = REBOOT_TIMEOUT);
