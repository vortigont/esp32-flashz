ESP32-FlashZ - an arduino library that provides zlib compressed OTA update feature for esp32
======

__[EXAMPLES](/examples/README.md) |__ [![PlatformIO
 CI](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml)


Unlike [esp8266](https://github.com/esp8266/Arduino/pull/6820/commits/67ba90d3eaf01c5400d0b42cdce05ef9295d8c16), esp32's bootloader does not (yet) support compressed image updates. But esp32 has a miniz lib decompressor in ROM, so it can be used to decompress zlib packed stream on the fly during OTA and write decompressed data to flash. This could be done both for firmware and filesystem image.

I wrote this lib just for fun to get the idea of miniz functions hidden in esp32 ROM. The code inspired by [esptool](https://github.com/espressif/esptool) than does the same with it's serial flasher stub.

### Features
 * low code overhead, deco algo is already present in ROM
 * both firmware and filesystem compressed images upload supported
 * compatible with AsyncWebServer, autodetect compressed/non-compressed images
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


### License
Since I get the idea from a [esptool](https://github.com/espressif/esptool) code, this lib inherits esptool's [GNU General Public License v2.0](LICENSE)

