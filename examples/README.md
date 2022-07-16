ESP32-FlashZ - Examples
======

[integration with AsynWebServer](/examples/asyncserver-flashz) - A complete Platformio project that integrates AsyncWebServer with compressed OTA self-updates.

[integration with ESP32 WebServer](/examples/httpserver-flashz) - A complete Platformio project that integrates ESP32 WebServer with compressed OTA self-updates.


Provided features:

 - a very basic html form accessible via `http://esphost/update`
 - uploading compressed/uncompressed firmware images
 - uploading compressed/uncompressed file system images
 - trigger downloading image file from a remote URL and initiate self-update from a download stream (both compressed/uncompressed images supported)
 - integrated PlatformIO script for on-the-fly compression and uploading firmware images

### Build
Edit one of the examples provided:

 - set you WiFi creds
 - with PlatformIO build and flash the the code via serial for the first time (`pio run -t upload`, `pio run -t uploadfs`)
 - run console monitor, it will show an ip address once WiFi is connected
 - open the browser and specified URL  (http://esphost/update)
 - upload zlib compressed firmware
 - or specify remote URL to download and update the fw image from
 - or use platformio's env's to do automated build/compress/upload fw tests
    - edit `platformio.ini`'s section `[env:flashz_ota]` and set your controller's IP address
    - build and upload compressed FW image via platformio `pio run -t upload -e flashz_ota`
    - build and upload compressed FS image via platformio `pio run -t uploadfs -e flashz_ota`

In case of issues you can build a pretty much verbose debug version of the lib `pio run -e flashz_debug -t upload`
