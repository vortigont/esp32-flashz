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

#include "flashz-http.hpp"
#include "flashz.hpp"

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define FZ_NOHTTPCLIENT
#warning "esp32-c3 does not support OTA via http-client"
#endif

#ifndef  FZ_NOHTTPCLIENT
#include <HTTPClient.h>
#endif

#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

// ESP32 log tag
static const char *TAG __attribute__((unused)) = "FZ-HTTP";

static const char PGotaform[]  = R"===(
<!DOCTYPE html><html lang='en'>
<head>
    <meta charset='utf-8' />
    <meta name='viewport' content='width=device-width,initial-scale=1'/>
</head><body>
<h2>ESP32 FlashZ OTA form</h2><br>
<form method="post" action="#" enctype='multipart/form-data'>
    <input type="radio" id="imgtype1" name="img" value="fw" checked>
    <label for="imgtype1">Firmware</label>
    <input type="radio" id="imgtype2" name="img" value="fs">
    <label for="imgtype2">FileSystem</label><br>
    <input type='file' accept='.bin, .zz' name="file">
    <button type='submit'>Upload</button>
</form><hr>
<form method="post" action="#" enctype='multipart/form-data'>
    <input type="radio" id="imgtype1" name="img" value="fw" checked>
    <label for="imgtype1">Firmware</label>
    <input type="radio" id="imgtype2" name="img" value="fs">
    <label for="imgtype2">FileSystem</label><br>
    <input type='input' id="url" name='url'>
    <label for="url">Download firmware URL</label>
    <button type='submit'>Update</button>
</form></body></html>
)===";

static const char PGimg[]  = "img";
static const char PGurl[]  = "url";

#ifndef  FZ_NOHTTPCLIENT
void FlashZhttp::_fz_http_trigger(FlashZhttp *fz){
    if (!fz->cb) return;
    fz->_err = fz_http_err_t::inprogress;
    fz->_err = fz->_http_get(fz->cb->url.c_str(), fz->cb->type);
    delete fz->cb;
    fz->cb = nullptr;
}
#endif

#ifdef FZ_WITH_ASYNCSRV
void FlashZhttp::provide_ota_form(AsyncWebServer *srv, const char* url){
    srv->on(url, HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, PGmimehtml, PGotaform); });
}

void FlashZhttp::handle_ota_form(AsyncWebServer *srv, const char* url){
    srv->on(url, HTTP_POST,
        // handle form data
        [this](AsyncWebServerRequest *request){
            // check if a post is for URL fw download form or post-file upload
            if(request->hasParam(PGurl, true)){
#ifdef FZ_NOHTTPCLIENT
                return request->send(500, PGmimetxt, "No HTTP Client support");
#else
                // postpone client-OTA, it can't be run in async call-back
                fetch_async(request->getParam(PGurl, true)->value().c_str(), request->getParam(PGimg, true)->value() == "fs" ? U_SPIFFS : U_FLASH);
                return request->send(200, PGmimetxt, "Attempting OTA from URL in background");
#endif  // FZ_NOHTTPCLIENT
            } else {
                if (FlashZ::getInstance().hasError()) {
                    request->send(503, PGmimetxt, "Update FAILED");
                } else {
                    if (rst_timeout){
                        if (!t)
                            t = new Ticker;

                        t->once_ms(rst_timeout, [](){ ESP.restart(); });
                    }
                    request->send(200, PGmimetxt, "OTA complete, autoreboot in 5 sec...");
                }
            }
        },
        // handle file upload
        [this](AsyncWebServerRequest *r, String f, size_t i, uint8_t *d, size_t l, bool fin){ this->file_upload(r, f, i, d, l, fin); }
    );
}

void FlashZhttp::file_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){

    // first chunk of body data
    if (!index) {
        bool mode_z = (data[0] == ZLIB_HEADER);    // check if we have a compressed image

        int type;

        if (request->hasParam(PGimg, true)){
            // image type is specified in the form data
            type = request->getParam(PGimg, true)->value() == "fs" ? U_SPIFFS : U_FLASH;
        } else{
            // no image type specified, try to autodetect
            if (mode_z || (data[0] == ESP_IMAGE_HEADER_MAGIC))        // can't detect what is insize zlib, so assume it's a fw image (won't owerwrite chip's FS)
                type = U_FLASH;
            else
                type = U_SPIFFS;
//          return request->send(400, PGmimetxt, F("Not an FW image or img type is unknown"));
        }

        // can rely on upload's size only if img is uncompressed
        // request->contentLength() return size of the whole post body, it is larger than uploaded file size
        //size_t size = (data[0] == ESP_IMAGE_HEADER_MAGIC) ? request->contentLength() : UPDATE_SIZE_UNKNOWN;
	size_t size = UPDATE_SIZE_UNKNOWN;


        ESP_LOGI(TAG, "Updating %s, input size:%u, mode_z:%u, magic: %02X", (type == U_FLASH)? "Firmware" : "Filesystem", request->contentLength(), mode_z, data[0]);

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
            ESP_LOGW(TAG, "Update failed to complete");
            //request->send(503, PGmimetxt, FlashZ::getInstance().errorString());
        }
    }
}

#endif // #ifdef FZ_WITH_ASYNC

#ifndef  FZ_NOHTTPCLIENT
fz_http_err_t FlashZhttp::_http_get(const char* url, int imgtype){
    if (!url)
        return fz_http_err_t::bad_param;

    ESP_LOGI(TAG, "Update from URL:%s", url);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.begin(url);
    int httpCode = http.GET();

    if(httpCode != HTTP_CODE_OK){
        ESP_LOGW(TAG, "http err, reply code:%d", httpCode);
        return fz_http_err_t::httpcode_err;
    }

    int len = http.getSize();
    if ( len <= 0){
        ESP_LOGW(TAG, "http bad file size:%d", len);      // -1 for chunked reply is not supported
        return fz_http_err_t::bad_size;
    }

    WiFiClient *stream = http.getStreamPtr();
    if (!stream){
        http.end();
        ESP_LOGW(TAG, "bad http stream");
        return fz_http_err_t::bad_stream;
    }

    bool mode_z = (stream->peek() == ZLIB_HEADER);          // check if we get a zlib compressed image

    size_t fwsize = mode_z ? UPDATE_SIZE_UNKNOWN : len;     // fw_size is unknown if we have a compressed image
    ESP_LOGI(TAG, "Updating %s, input size:%u, mode_z:%u, magic: %02X", (imgtype == U_FLASH)? "FW" : "FS", len, mode_z, stream->peek());

    if (!(mode_z ? FlashZ::getInstance().beginz(fwsize, imgtype) : FlashZ::getInstance().begin(fwsize, imgtype))){
        FlashZ::getInstance().abortz();
        ESP_LOGW(TAG, "Failed to start Update");
        return fz_http_err_t::bad_start;
    }

    size_t wrt = mode_z ? FlashZ::getInstance().writezStream(*stream, len) : FlashZ::getInstance().writeStream(*stream);
    http.end();
    stream = nullptr;

    if (wrt != len){
        FlashZ::getInstance().abortz();
        ESP_LOGE(TAG, "UPD failed, wrt:%d of %d\n", wrt, len);
        return fz_http_err_t::write_err;
    } else {
        if(FlashZ::getInstance().endz()){
            ESP_LOGI(TAG, "Update Success: %u bytes", wrt);
        } else {
            ESP_LOGW(TAG, "Update failed to complete");
        }
    }

    if (rst_timeout){
        if (!t)
            t = new Ticker;

        t->once_ms(rst_timeout, [](){ ESP.restart(); });
    }

    return fz_http_err_t::ok;
}
#endif  //FZ_NOHTTPCLIENT

#ifndef FZ_NO_WEBSRV
void FlashZhttp::provide_ota_form(WebServer *server, const char* url){
    // Simple OTA-Update Form
    server->on(url, HTTP_GET, [server](){ server->send(200, PGmimehtml, PGotaform ); });
}

void FlashZhttp::handle_ota_form(WebServer *server, const char* url){
    // handler for the /update form POST (once file upload finishes or http-client form)
    server->on(url, HTTP_POST, [server, this](){
        if (server->hasArg(PGurl)){
#ifdef FZ_NOHTTPCLIENT
            return server->send(500, PGmimetxt, "No HTTP Client support");
#else
            // postpone client-OTA
            fetch_async(server->arg(PGurl).c_str(), server->arg(PGimg) == "fs" ? U_SPIFFS : U_FLASH);
            return server->send(200, PGmimetxt, "Attempting OTA from URL in background");
#endif  // FZ_NOHTTPCLIENT
        } else {
            if (FlashZ::getInstance().hasError()) {
                server->send(500, PGmimetxt, "UPDATE FAILED");
            } else {
                if (rst_timeout){
                    if (!t)
                        t = new Ticker;

                    t->once_ms(rst_timeout, [](){ ESP.restart(); });
                }

                server->client().setNoDelay(true);
                server->send(200, PGmimetxt, F("OTA complete, autoreboot in 5 sec..."));
                server->client().stop();
            }
        }
    }, [this, server](){ this->file_upload(server); } );
}

void FlashZhttp::file_upload(WebServer *server){
    HTTPUpload& upload = server->upload();

    switch (upload.status){
/*
        // first chunk of body data
        case HTTPUploadStatus::UPLOAD_FILE_START :
            ESP_LOGI(TAG, "START updating");
            break;
*/
        // file data
        case HTTPUploadStatus::UPLOAD_FILE_WRITE : {
             // if first chunk
            if (!upload.totalSize){
                bool mode_z = (upload.buf[0] == ZLIB_HEADER);    // check if we have a compressed image
                int type;

                if (server->hasArg(PGimg)){
                    // image type is specified in the form data
                    type = server->arg(PGimg) == "fs" ? U_SPIFFS : U_FLASH;
                } else{
                    // no image type specified, try to autodetect
                    if (upload.buf[0] == ESP_IMAGE_HEADER_MAGIC || mode_z)        // can't detect what is insize zlib, so assume it's a fw image (won't owerwrite chip's FS)
                        type = U_FLASH;
                    else
                        type = U_SPIFFS;
                }

                ESP_LOGI(TAG, "Begin updating %s, mode_z:%u, magic: %02X", (type == U_FLASH)? "Firmware" : "Filesystem", mode_z, upload.buf[0]);

                if (!(mode_z ? FlashZ::getInstance().beginz(UPDATE_SIZE_UNKNOWN, type) : FlashZ::getInstance().begin(UPDATE_SIZE_UNKNOWN, type))){
                    return server->send(503, PGmimetxt, FlashZ::getInstance().errorString());
                }
            }

            //deco_stat_t s;
            //FlashZ::getInstance().getstat(s);
            //int bytes_left = upload.totalSize - s.in_bytes - upload.currentSize;
            if(FlashZ::getInstance().writez(upload.buf, upload.currentSize, false) != upload.currentSize){
                ESP_LOGW(TAG, "OTA failed in progress: %s", FlashZ::getInstance().errorString());
                server->send(503, PGmimetxt, FlashZ::getInstance().errorString());
                server->client().stop();
                return FlashZ::getInstance().abortz();
            }
            //ESP_LOGI(TAG, "Updating %s, tsize:%u, len:%u", "Firmware", upload.totalSize, upload.currentSize);
            break;
        }

        case HTTPUploadStatus::UPLOAD_FILE_END : {
            if(FlashZ::getInstance().writez(upload.buf, upload.currentSize, true) != upload.currentSize){
                ESP_LOGW(TAG, "OTA failed in progress: %s", FlashZ::getInstance().errorString());
                //server->send(503, PGmimetxt, FlashZ::getInstance().errorString());
                return FlashZ::getInstance().abortz();
            }
            if(FlashZ::getInstance().endz()){
                ESP_LOGI(TAG, "Update Success: %u bytes", upload.totalSize);
                //server->send(200, PGmimetxt, "Update complete");
            } else {
                ESP_LOGW(TAG, "Update failed to complete");
                //server->send(200, PGmimetxt, "Update failed to complete");
            }
            break;
        }

        //case HTTPUploadStatus::UPLOAD_FILE_ABORTED
        default : {
            FlashZ::getInstance().abortz();
            ESP_LOGW(TAG, "Update aborted");
        }
    }
}
#endif // #ifndef FZ_NO_WEBSRV

unsigned FlashZhttp::autoreboot(unsigned t){
    t = rst_timeout;
    return t;
}

#ifndef FZ_NOHTTPCLIENT
void FlashZhttp::fetch_async(const char* url, int imgtype, int delay){
    if (!cb){ cb = new callback_arg_t(imgtype, url); }
    if (!t) t = new Ticker;
    // have no idea why, but C3 bootloops here, needs investigation
    t->once_ms(delay, FlashZhttp::_fz_http_trigger, this);
    _err = fz_http_err_t::pending;
}
#endif  //FZ_NOHTTPCLIENT
