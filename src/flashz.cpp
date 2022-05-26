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

#include "flashz.hpp"

#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

// ESP32 log tag
static const char *TAG __attribute__((unused)) = "FLASHZ";


bool Inflator::init(){
    rdy = false;

    if (!dictBuff)
        dictBuff = (uint8_t *)malloc(sizeof(uint8_t) * TINFL_LZ_DICT_SIZE);

    if (!dictBuff)
        return false;   // OOM

    if (!m_decomp)
        m_decomp = new tinfl_decompressor;

    if (!m_decomp){
        delete dictBuff;
        dictBuff = nullptr;
        return false;   // OOM
    }

    reset();
    rdy = true;
    return rdy;
}

void Inflator::reset(){
    if (m_decomp)
        tinfl_init(m_decomp);

    dict_free = TINFL_LZ_DICT_SIZE;
    dict_begin = dict_offset = 0;

    avail_in = total_in = total_out = 0;

    decomp_status = TINFL_STATUS_NEEDS_MORE_INPUT;
    decomp_flags = TINFL_FLAG_PARSE_ZLIB_HEADER;          // compressed stream MUST have a proper zlib header
}

void Inflator::end(){
    rdy = false;
    delete m_decomp;
    m_decomp = nullptr;
    delete dictBuff;
    dictBuff = nullptr;
}

int Inflator::inflate(bool final){
    if (!next_in)
        return MZ_STREAM_ERROR;

    if (!dict_free)
        return MZ_NEED_DICT;

    if (final)
        decomp_flags &= ~TINFL_FLAG_HAS_MORE_INPUT;
    else
        decomp_flags |= TINFL_FLAG_HAS_MORE_INPUT;

    size_t in_bytes = avail_in, out_bytes = dict_free;

    // decompress as may input as available or as long as free dict space is available
    decomp_status = tinfl_decompress(m_decomp, next_in, &in_bytes, dictBuff, dictBuff + dict_offset, &out_bytes, decomp_flags);

    next_in += in_bytes;    // advance the input buffer pointer to the number of consumed bytes
    avail_in -= in_bytes;   // decrement input buffer counter
    total_in += in_bytes;   // increment total input cntr
    total_out += out_bytes; // increment total output cntr

    dict_offset = (dict_offset + out_bytes) & (TINFL_LZ_DICT_SIZE - 1);
    dict_free -= out_bytes;

    if (decomp_status < 0)
        return MZ_DATA_ERROR; /* Stream is corrupted (there could be some uncompressed data left in the output dictionary - oh well). */

    if ((decomp_status == TINFL_STATUS_NEEDS_MORE_INPUT) && final )    /* if delator need more input and we demand it's a final call, than something must be wrong with a stream */
        return MZ_STREAM_ERROR;

    if (decomp_status == TINFL_STATUS_HAS_MORE_OUTPUT)    /* if delator can't fit more data to the output buf, than need to flush a buff */
        return MZ_NEED_DICT;

    return ((decomp_status == TINFL_STATUS_DONE) && (final)) ? MZ_STREAM_END : MZ_OK;
};

int Inflator::inflate_block_to_cb(const uint8_t* inBuff, size_t len, inflate_cb_t callback, bool final, size_t chunk_size){
    if (!rdy)
        return MZ_BUF_ERROR;    // inflator not initialized

    next_in = inBuff;
    avail_in = len;

    decomp_flags &= ~( TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );  // use internal ring buffer for decompression

    for (;;){

        int err = inflate(final);                                   // inflate as much in-data as possible
        //ESP_LOGD(TAG, "+inflate chunk - mz_err:%d, dfree:%u, tin:%u, tout:%u", err, dict_free, total_in, total_out);

        if (err < 0){
            ESP_LOGD(TAG, "inflate failure - MZ_ERR: %d, inflate status: %d", err, decomp_status);
            return err;         // exit on any error
        }

        size_t deco_data_len = (dict_offset - dict_begin) & (TINFL_LZ_DICT_SIZE - 1);

        /**
         * call the callback if:
         * - no free space in dict
         * - accumulated data in dict is >= prefered chunk size
         * - it's a final input chunk
         */
        if (!dict_free || deco_data_len >= chunk_size || final){
            //ESP_LOGD(TAG, "dict stat: dfree:%u, ddl:%u, end:%u, tin:%d, tout:%d", dict_free, deco_data_len, final, total_in, total_out);

            /**
             * iterate the callback while:
             * - no space in dict
             * - have data in dict and it's a final chunk
             * - have data in dict >= prefered chunk size
             * 
             */
            while (!dict_free || (final & (bool)deco_data_len) || (deco_data_len >= chunk_size)){
                ESP_LOGD(TAG, "CB - idx:%u, head:%u, dbgn:%u, dend:%u, ddatalen:%u, avin:%u, tin:%u, tout:%u, fin:%d", total_out, dictBuff, dict_begin, dict_offset, deco_data_len, avail_in, total_in, total_out, final && (err == MZ_STREAM_END));

                // callback can consume only a portion of data from dict
                size_t consumed = callback(total_out - deco_data_len, dictBuff + dict_begin, deco_data_len, final && (err == MZ_STREAM_END));

                if (!consumed || consumed > deco_data_len)      // it's an error not to consume or consume too much of dict data
                    return MZ_ERRNO;

                // clear the dict if all the data has been consumed so far
                if (consumed == deco_data_len){
                    dict_free = TINFL_LZ_DICT_SIZE;
                    dict_offset = 0;
                }

                dict_begin = (dict_begin+consumed) & (TINFL_LZ_DICT_SIZE - 1);     // offset deco data pointer in dict
                deco_data_len -= consumed;
            }
        }

        // if we are done with this chunk of input, than quit
        if (!avail_in)
            return err;

        // go another inflate round
    }
}


/**    FlashZ Class implementation    **/

bool FlashZ::beginz(size_t size, int command, int ledPin, uint8_t ledOn, const char *label){
    //stat = {0, 0, 0, 0};

    if (!deco.init())       // allocate Inflator memory
        return false;

    return begin(size, command, ledPin, ledOn, label);
}


size_t FlashZ::writez(uint8_t *data, size_t len, bool final){
    int err = deco.inflate_block_to_cb(data, len, [this](size_t i, const uint8_t* d, size_t s, bool f) int { return flash_cb(i, d, s, f); }, final, FLASH_CHUNK_SIZE);

    if (err == MZ_OK && !final)             // intermediary chunk, ok
        return len;

    if (err == MZ_STREAM_END && final)      // we reached end of the z-stream, good
        return len;

    ESP_LOGD(TAG, "Inflate ERROR: %d", err);

    return 0;                               // deco error, assume that no data has been written, signal to the caller that something is wrong
}

void FlashZ::abortz(){
    abort();
    deco.end();
}

bool FlashZ::endz(bool evenIfRemaining){
    deco.end();
    return end(evenIfRemaining);
}

int FlashZ::flash_cb(size_t index, const uint8_t* data, size_t size, bool final){
    if (!size)
        return 0;

    size_t len;
    if (size >= FLASH_CHUNK_SIZE && !final)
        len = FLASH_CHUNK_SIZE;
    else
        len = size;

    if (write((uint8_t*)data, len) != len){     // this cast to (uint8_t*) is a very dirty hack, but Arduino's Updater lib is missing constness on data pointer
        ESP_LOGD(TAG, "flashz ERROR!");
        return 0;                               // if written size is less than requested, consider it as a fatal error, since I can't determine processed delated size
    }

    ESP_LOGD(TAG, "flashed %u bytes", len);

    return len;
}