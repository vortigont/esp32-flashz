# Change Log

## v 1.1.3 (2024-01-26)
 + example for on-the-fly compression via pako.js lib
 * make exaples more CI friendly
 * minor cleanup

## v 1.1.2 (2023-05-20)
 + add `FZ_NOHTTPCLIENT` build-time flag to exclude HTTP Client support for the sake of reduced binary size
 * more safety for dynamic objects
 - disable "HTTP Client" feature for esp32-c3 (need triage)

## v 1.1.1 (2022-10-02)
 * Changed read counter to read timeout (by @tobozo)
 * deallocate inflator's mem on update begin failure
 + post_flashz.py script now relies on 'upload_*" project options
 * ignore POST body size on file upload, assume it is unknown


## v 1.1.0 (2022-07-12)
 + ESP32 WebServer file upload handling
 + integrate with http client download
 + Stream decompression
 * decompressor bug fixes

## v 1.0.0
 Initital release.
 Basic functions for block decompression and update with AsyncWebServer stiching