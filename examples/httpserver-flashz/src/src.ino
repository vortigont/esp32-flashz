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

/*

This file is just a stub to make Arduino IDE happy

Pls, see main.cpp for sketch code

*/