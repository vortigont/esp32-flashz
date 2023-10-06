ESP32-FlashZ
an arduino library that provides zlib compressed OTA update feature for esp32
======

__[EXAMPLES](/examples/README.md) | [CHANGELOG](/CHANGELOG.md) |__ [![PlatformIO
 CI](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/esp32-flashz/actions/workflows/pio_build.yml)


Unlike [esp8266](https://github.com/esp8266/Arduino/pull/6820/commits/67ba90d3eaf01c5400d0b42cdce05ef9295d8c16), esp32's bootloader does not (yet) support compressed image updates. But esp32 has a miniz lib decompressor in ROM, so it can be used to decompress zlib packed stream on the fly during OTA update and write decompressed data to SPI flash. This could be done both for firmware and filesystem image.

I wrote this lib just for fun to get the idea of miniz functions hidden in esp32 ROM. `FlashZ` lib is integrated into [EmbUI](https://github.com/vortigont/EmbUI) framework as OTA update handler.
The code inspired by [esptool](https://github.com/espressif/esptool) that does the same with it's serial flasher stub.

### Features
 * low code overhead, deco algo is already present in ROM
 * both firmware and filesystem compressed images upload supported
 * compatible with ESP32 [WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), autodetect compressed/non-compressed images
 * compatible with [AsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), autodetect compressed/non-compressed images
 * stream decompression, i.e. via [http client](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient) (fetch and flash compressed image from remote URL)
 * PlatformIO integration via [post_flash.py](/examples/asyncserver-flash/post_flash.py) script that automates on-the-fly compression and OTA upload for your project
 * on-the-fly compressed upload example via [pako](https://github.com/nodeca/pako) js lib (tnx to @playmiel for contribution)

### Requirements
 * 32k of heap memory during decompression for dict data
 * derives from Arduino's UpdaterClass to perform flash operation on inflated data
 * firmware images must be compressed in zlib stream (not a gzip format file!). There are many ways to do this, i.e.
    - use [pigz](https://zlib.net/pigz/) tool with `-z` flag
    - use python's zlib module (check [post_flash.py](/examples/asyncserver-flash/post_flash.py) script for Platformio)
    - use perl's Compress::Zlib module
    - use [pako](https://github.com/nodeca/pako) js lib


Check [examples](/examples) to get more idea on how to intergate this lib into platformio projects

### Compatibility
| Platform    | Firmware           | Filesystem         |
|-------------|--------------------|--------------------|
|ESP32        | :heavy_check_mark: | :heavy_check_mark: |
|ESP32-S2     | :heavy_check_mark: | :heavy_check_mark: |
|ESP32-S3     | :heavy_check_mark: | :heavy_check_mark: |
|ESP32-c3     | :heavy_check_mark: | :heavy_check_mark: |


### Comparison
some basic tests

| OTA update                           | Origin | zlib |
|-            		               |  -     | -    |
| esp32, fw, ~1MiB                     | 9.8 s  |  10.1 s |
| esp32, fs, ~1.5MiB, 90% sparse       | 9 s    |  6 s   |
| esp32-c3, fw, ~1MiB                  | 9.2 s  |  9.1 s |
| esp32-c3, fs, ~1.5MiB, 90% sparse    | 4.1 s  |  2.5 s |
| esp32-s2, fw, ~1MiB                  | 7.5 s  |  7.5 s |
| esp32-s2, fs, ~1.5MiB, 90% sparse    | 4.7 s  |  1.8 s |



### Using FlashZ lib
FlashZ Lib consists of a low level `FlashZ` singleton that derives from a built-in Arduino's [UpdateClass](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update) class and provides additional methods to handle zlib compressed data. It tries to maintain same API as `Update`.

`FlashZ::beginz` suposed to be called instead of `UpdateClass::begin` call to intialize update procedure. It allocates zlib Inflator structures and dictionary memory. For uncompressed data `FlashZ::begin` could be used as usual with `UpdaterClass`.

`FlashZ::writez` is called to inflate and flash zlib compressed block of data. `bool final` flag is used to signal last piece of data input.

`FlashZ::writezStream` can read a standart `Stream` class objects, decompress and write decompressed stream to flash.
NOTE: total stream length must be known in advance, so that Inflator insures a proper end of stream is reached on decompression.

`FlashZ::abortz` or `FlashZ::endz` must be called to end the update and release dynamically allocated Inflator memory.

To stich `FlashZ` with networking and OTA updates here is a `FlashZhttp` class. This is not a complete OTA updater solution but more of a reference implementation example. Any real-life projects could easily implement something similar with more features, bells and whistles.
`FlashZhttp` class integrates [WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer) or [AsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) file upload feature with `FlashZ` low level methods. Also it can initiate streamed download via [http client](https://github.com/espressif/arduino-esp32/tree/master/libraries/) from a remote URL (only plain http).
`FlashZhttp` methods includes some heuristic in attempt to autodetect file image format and type, so that it can handle both compressed and uncompressed images transparently. But for compressed file it can't autodetect between firmware and FS image, so it need some metadata to differetiate. This is implemented via additional POST data fields.

### Build-time options
By default `AsyncWebServer` support is not build into lib, do not want to intorduce dependency for external lib.
To get `AsyncWebServer` support, `FlashZ` lib **must** be build with `FZ_WITH_ASYNCSRV` flag. This could be done via PlatformIO build_flags. `AsyncWebServer` and `ESP32 WebServer` support options are mutually exclusive due to some definitions clashing.
See [EXAMPLES](/examples/README.md) projects for further details.

By default `FlashZhttp` class within the lib includes support for HTTP Client to be able to download and flash an image from a remote URL. It uses Arduino's `HTTPClient.h` lib for that, which ALWAYS includes SSL support, even if client code does not meant to be using it. SSL support makes a huge impact on resulting firmware image size, it grows in about 100-120k. If you do not need HTTP Client support for the sake of reducing resulting image size you can define `FZ_NOHTTPCLIENT` build flag to completely disable HTTP Client and allow linker exclude SSL-related code from the resulting firmware image. This is here untill I find a better way to workaround it, maybe a flag to exclude FlashZhttp completely?

Also you **should** always specify `NO_GLOBAL_UPDATE` build flag for your project to prevent Arduino's UpdateClass creating it's instance by default. FlashZ uses it's own instance of a derived class and default one just wastes your memory (about 180 bytes). See [arduino-esp32/releases#8500](https://github.com/espressif/arduino-esp32/pull/8500 )

### Integration with PlatformIO
It is pretty easy to integrate PlatformIO with HTTP OTA update via post build scripting. Python's zlib module could be used to compress firmware image after building and http-client module to upload a compressed image to  ESP32 board Over-the-Air. See a reference implementation in [post_flashz.py](/examples/asyncserver-flash/post_flashz.py) example. It relies on `FlashZhttp` class methods to process POST form data but could be adjusted easily. Additional `platformio.ini` variables are used to set remote address of a board. Uploading compressed firmware/FS is done automagicaly via simple `pio run -t upload`. (MCU must be connected to network and reachable).

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

