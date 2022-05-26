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

#include <ESPAsyncWebServer.h>
#include "flashz.hpp"

#ifndef ESP_IMAGE_HEADER_MAGIC
#define ESP_IMAGE_HEADER_MAGIC  0xE9
#endif
#define GZ_HEADER               0x1F
#define ZLIB_HEADER             0x78

static const char PGmimehtml[] = "text/html; charset=utf-8";
static const char PGmimetxt[]  = "text/plain";


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