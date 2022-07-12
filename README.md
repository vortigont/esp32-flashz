ESP32-FlashZ - an arduino library that provides zlib compressed OTA update feature for esp32
======

__[EXAMPLES](/examples/README.md) |__ [![PlatformIO
 CI](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml)


Unlike [esp8266](https://github.com/esp8266/Arduino/pull/6820/commits/67ba90d3eaf01c5400d0b42cdce05ef9295d8c16), esp32's bootloader does not (yet) support compressed image updates. But esp32 has a miniz lib decompressor in ROM, so it can be used to decompress zlib packed stream on the fly during OTA and write decompressed data to flash. This could be done both for firmware and filesystem image.

I wrote this lib just for fun to get the idea of miniz functions hidden in esp32 ROM. The code inspired by [esptool](https://github.com/espressif/esptool) than does the same with it's serial flasher stub. Now it is integrated into [EmbUI](https://github.com/vortigont/EmbUI) framework as OTA update handler.


### Features
 * low code overhead, deco algo is already present in ROM
 * both firmware and filesystem compressed images upload supported
 * compatible with ESP32 [WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), autodetect compressed/non-compressed images
 * compatible with [AsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), autodetect compressed/non-compressed images
 * stream decompression, i.e. via [http client](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient) (fetch and flash compressed image from remote URL)
 * PlatformIO integration via [post_flash.py](/examples/asyncserver-flash/post_flash.py) script that automates on-the-fly compression and OTA upload for your project

### Requirements
 * 32k of heap memory during decompression for dict data
 * derives from Arduino's UpdaterClass to perform flash operation on inflated data
 * firmware images must be compressed in zlib stream (not a gzip format file!). There are many ways to do this, i.e.
    - use [pigz](https://zlib.net/pigz/) tool with -z flag
    - use python's zlib module (check [post_flash.py](/examples/asyncserver-flash/post_flash.py) script for Platformio)
    - use perl's Compress::Zlib module


Check [examples](/examples) to get more idea on how to intergate this lib into platformio projects

### Compatibility
| Platform    | Firmware           | Filesystem         |
|-------------|--------------------|--------------------|
|ESP32        | :heavy_check_mark: | :heavy_check_mark: |
|ESP32-S2     | :heavy_check_mark: | :heavy_check_mark: |
|ESP32-S3     | not tested         | not tested         |
|ESP32-c3     | :heavy_check_mark: | :heavy_check_mark: |


### Using the FlashZ lib
The Lib consists of a low level `FlashZ` singleton that derives from a built-in Arduino's `Updater` class and provides additional methods to handle zlib compressed data. It tries to maintain same API as `Updater`.

`FlashZ::beginz` suposed to be called instead of `Updater::begin` call to intialize update procedure. It allocates zlib Inflator structures and dictionary memory. For uncompressed data `FlashZ::begin` could be used same way.

`FlashZ::writez` is called to uncomress and flash zlib block of data. `bool final` flag is used to signal last piece of data input.

`FlashZ::writezStream` can read a standart `Stream` class objects, decompress and write decompressed stream to flash. NOTE: total stream length must be known in advance, so that Inflator insures a proper end of stream is reached on decompression.

`FlashZ::abortz` or `FlashZ::endz` must be called to end the update and release dynamically allocated Inflator memory.

To stich FlashZ with networking and OTA updates here is a `FlashZhttp` class. This is not a complete OTA updater solution but more of a refence implementation. It integrates [WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer) or [AsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) file upload feature with FLashZ low level methods. Also it can initiate streamed download via [http client](https://github.com/espressif/arduino-esp32/tree/master/libraries/ from a remote URL (only plain http).
FlashZhttp methods includes some heuristic in attemp to autodetect file image format and type, so that it can handle both compressed and uncompressed images transparently. But for compressed file it can't autodetect between firmware and FS image, so it need some metadata to differetiate. Implemented via additional POST data.
See [EXAMPLES](/examples/README.md for more further details.

### Integration with PlatformIO
It is pretty easy to integrate PlatformIO with HTTP OTA update via post build scripting. Python's zlib module could be used to compress firmware image after building and http-client module to upload a compressed image to the ESP32 board Over-the-Air. See a reference implementation in [post_flash.py](/examples/asyncserver-flash/post_flash.py) example. It relies on `FlashZhttp` class methods to process POST form data but could be adjusted easily. Additional platformio.ini variables are used to set remote address of a board. Uploading compressed firmware/FS is done automagicaly via simple `pio run -t upload`. (MCU must be connected to network and reachable).

### Using CLI tools for updates
I'm a linux user and prefer to use cli tools for automating tasks rather than web browser. So here are some oneliners for ESP32-FlashZ updating

 - compress and upload firmware image to ESP
`pigz -9kzc .pio/build/esp32c3/firmware.bin | curl -v http://$ESPHOST/update -F file=@-`

 - compress and upload File System image to ESP
`pigz -9kzc .pio/build/esp32-s2/littlefs.bin | curl -v http://$ESPHOST/update -F "img=fs" -F file=@-`

 - trigger esp32's firmware self-update from a remote host
`curl http://$ESPHOST/update -F "img=fw" -F "url=http://$REMOTE/download/firmware.bin.zz"`

 - trigger esp32's FS self-update from a remote host
`curl http://$ESPHOST/update -F "img=fs" -F "url=http://$REMOTE/download/littlefs.bin.zz"`


### License
Since I get the idea from a [esptool](https://github.com/espressif/esptool) code, this lib inherits esptool's [GNU General Public License v2.0](LICENSE)

