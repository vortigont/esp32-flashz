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

#pragma once

#ifdef FZ_WITH_ASYNCSRV
#include <ESPAsyncWebServer.h>
#define FZ_NO_WEBSRV
#else
#include <WebServer.h>
#endif  // #ifdef FZ_WITH_ASYNCSRV
#include <Ticker.h>

#define FZ_REBOOT_TIMEOUT  5000

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



/**
 * @brief FlashZ HTTP helper class
 * implements http uploading/downloading for (compressed) firmware/fs images
 * on the fly decompression and flashing
 * 
 */
class FlashZhttp {
    struct callback_arg_t {
        int type;
        String url;
    };

    unsigned rst_timeout = FZ_REBOOT_TIMEOUT;
    callback_arg_t *cb;

    Ticker *t;

    static void fz_http_trigger(FlashZhttp *fz);

public:
    unsigned autoreboot(unsigned t);
    unsigned autoreboot(){ return rst_timeout; };

#ifdef FZ_WITH_ASYNCSRV
    /**
     * @brief register OTA URL within AsyncServer, handles HTTP GET requests
     * - provides a simple file upload form for FW/FS file images, could be raw *.bin or *.zz zlib compressed files
     * - register file upload call-back that hanles infate and OTA flash
     * 
     * @param srv - AsyncWebServer object
     * @param url - i.e. "/upload"
     */
    void provide_ota_form(AsyncWebServer *srv, const char* url);

    /**
     * @brief register file upload call-back that hanles upload, infate and OTA flash
     * handles HTTP_POST request
     * form data is parsed to find out type of upload, compressed/uncompressed format
     * or triggers http_client call to download image from remote URL
     * 
     * @param srv - AsyncWebServer object
     * @param url - i.e. "/upload"
     */
    void handle_ota_form(AsyncWebServer *srv, const char* url);

    /**
     * @brief callback for file upload data
     * it decompresses file chunk (if needed) and writes data to flash
     * MCU will autoreboot on success if autoreboot() is > 0 ms
     * 
     * @param request 
     * @param filename 
     * @param index 
     * @param data 
     * @param len 
     */
    void file_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
#endif // #ifdef FZ_WITH_ASYNC

#ifndef FZ_NO_WEBSRV
    /**
     * @brief register OTA URL within AsyncServer, handles HTTP GET requests
     * - provides a simple file upload form for FW/FS file images, could be raw *.bin or *.zz zlib compressed files
     * - register file upload call-back that hanles infate and OTA flash
     * 
     * @param srv - AsyncWebServer object
     * @param url - i.e. "/upload"
     */
    void provide_ota_form(WebServer *srv, const char* url);

    /**
     * @brief register file upload call-back that hanles upload, infate and OTA flash
     * handles HTTP_POST request
     * form data is parsed to find out type of upload, compressed/uncompressed format
     * or triggers http_client call to download image from remote URL
     * 
     * @param srv - AsyncWebServer object
     * @param url - i.e. "/upload"
     */
    void handle_ota_form(WebServer *server, const char* url);

    /**
     * @brief callback for file upload data
     * it decompresses file chunk (if needed) and writes data to flash
     * MCU will autoreboot on success if autoreboot() is > 0 ms
     * 
     * @param request 
     * @param filename 
     * @param index 
     * @param data 
     * @param len 
     */
    void file_upload(WebServer *server);
#endif // #ifndef FZ_NO_WEBSRV

    /**
     * @brief fetch (possibly compressed) image file via http and flash
     * compressed image format is autodetected
     * 
     * @param url - source URL for firmware file (https is not supported)
     * @param imgtype - image file type U_FLASH (0 - default) or U_SPIFFS
     * @return fz_http_err_t - returns error code
     */
    fz_http_err_t http_get(const char* url, int imgtype = 0);
    fz_http_err_t http_get();

};
