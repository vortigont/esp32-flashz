ESP32-FlashZ - integration with AsynWebServer Example
======

A complete Platformio project that integrates AsyncWebServer with compressed OTA self-updates.
It provides nothing but a very basic HTML form to upload compressed/uncompressed firmware and LittleFS update images.


Use PlatformIO to build the project. Set your WiFi credentianls in main.cpp build and upload the code via serial initialy.

```
pio run -t upload
```

Check the serial output, it should emit something like:
```
Starting flashz test.........
Connected to: MyWiFi, IP-address: 192.168.168.25
Navigate to: http://192.168.168.25/update, and upload raw or zlib-compressed firmware/fs image
```

Now you can open http://192.168.168.25/update in the browser see the web form for fw upload. Both compressed/uncompressed images supported, image format will be autodetected on upload. Compressed images must have .zz extension to be allowed for upload via web form.

Let's upload a compressed FileSystem image now. This project provides a python script for PlatformIO that automates compression and OTA update for itself :)
Open [platformio.ini](platformio.ini) file and find [env:flashz_ota] section. There are two additional parameters there `OTA_compress = true` and `OTA_url = http://192.168.168.25/update`. The first one enables compression for fw/fs images on upload, the second sets OTA URL pointing to your device and replaces serial uploader with OTA. Set `OTA_url` to proper address and upload File system image with `pio run -e flashz_ota -t uploadfs`. Pio builder will try to compress and upload FS image OTA.

```
Building FS image from 'data' directory to .pio/build/flashz_ota/littlefs.bin
/index.html
Looking for upload port...
Auto-detected: /dev/ttyUSB0
Uploading .pio/build/flashz_ota/littlefs.bin
Found OTA_url option, will attempt over-the-air upload
Found OTA_compress option
Compressing littlefs.bin file...
Compress ratio 100%
Uploading file .pio/build/flashz_ota/littlefs.bin.zz to http://192.168.168.25/update 
The firmware has been successfuly uploaded!
```

The `Compress ratio 100%` turns out to be FS image file compressed from ~1.5 MiB to less than 2k. This project contains only one small `index.html` file for filesystem

Serial console confirms flashing and autoreboot in 5 sec.

```
...
[ 24911][I][flashz.cpp:251] flash_cb(): flashed 32768 bytes
[ 24911][I][flashz-async.cpp:120] fz_async_handler(): Update Success: 1936 bytes
[ 29917][W][WiFiGeneric.cpp:852] _eventCallback(): Reason: 8 - ASSOC_LEAVE
ets Jun  8 2016 00:22:57
```

You can also upload images with [curl](https://curl.se/) console tool

for firmware image
`curl -v http://$ESPHOST/update -F "img=fw" -F 'name=@.pio/build/flashz_ota/firmware.bin.zz'`

for fs image
`curl -v http://$ESPHOST/update -F "img=fs" -F 'name=@.pio/build/flashz_ota/littlefs.bin.zz'`

to compress and upload image on-the-fly with [pigz](https://zlib.net/pigz/) tool
`pigz -9kzc .pio/build/flashz/firmware.bin | curl -v http://$ESPHOST/update -F "img=fw" -F file=@-`
