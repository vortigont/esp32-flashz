ESP32-FlashZ - an arduino library that provides zlib compressed OTA update feature for esp32
======

Unlike [esp8266](https://github.com/esp8266/Arduino/pull/6820/commits/67ba90d3eaf01c5400d0b42cdce05ef9295d8c16), esp32's bootloader does not (yet) support compressed uploads. But esp32 has a miniz lib decompressor in ROM, so it can be used to decompress zlib packed stream on the fly during OTA and write decompressed data to flash. This could be done both for firmware and filesystem image.

I wrote this lib just for fun to get the idea of miniz functions hidden in esp32 ROM. The code inspired by [esptool](https://github.com/espressif/esptool) than does the same with it's serial flasher stub.

### Features
 * low code overhead, deco algo is already present in ROM
 * both firmware and filesystem images supported
 * compatible with AsyncWebServer

### Requirements
 * 32k of heap memory during decompression for dict data
 * derives from Arduino's UpdaterClass to perform flash operation on inflated data
 * firmware images must be compressed in zlib stream (not a gzip format file!). There are many ways to do this, i.e.
    - use [pigz](https://zlib.net/pigz/) tool with -z flag
    - use python's zlib module
    - use perl's Compress::Zlib module



### License
Since I get the idea from a [esptool](https://github.com/espressif/esptool) code, this lib inherits esptool's[GNU General Public License v2.0](LICENSE)

